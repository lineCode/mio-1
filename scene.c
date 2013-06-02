#include "mio.h"

struct scene *new_scene(void)
{
	struct scene *scene = malloc(sizeof(*scene));
	scene->tag = TAG_SCENE;
	LIST_INIT(&scene->armatures);
	LIST_INIT(&scene->objects);
	LIST_INIT(&scene->lights);
	return scene;
}

struct armature *new_armature(struct scene *scene, const char *skelname)
{
	struct skel *skel = load_skel(skelname);
	if (!skel)
		return NULL;

	struct armature *amt = malloc(sizeof(*amt));
	memset(amt, 0, sizeof(*amt));
	amt->tag = TAG_ARMATURE;
	amt->skel = skel;

	amt->dirty = 1;
	vec_init(amt->location, 0, 0, 0);
	quat_init(amt->rotation, 0, 0, 0, 1);
	vec_init(amt->scale, 1, 1, 1);

	LIST_INSERT_HEAD(&scene->armatures, amt, list);
	return amt;
}

struct object *new_object(struct scene *scene, const char *meshname)
{
	struct mesh *mesh = load_mesh(meshname);
	if (!mesh)
		return NULL;

	struct object *obj = malloc(sizeof(*obj));
	memset(obj, 0, sizeof(*obj));
	obj->tag = TAG_OBJECT;

	obj->mesh = mesh;

	obj->dirty = 1;
	vec_init(obj->location, 0, 0, 0);
	quat_init(obj->rotation, 0, 0, 0, 1);
	vec_init(obj->scale, 1, 1, 1);

	LIST_INSERT_HEAD(&scene->objects, obj, list);
	return obj;
}

struct light *new_light(struct scene *scene)
{
	struct light *light = malloc(sizeof(*light));
	memset(light, 0, sizeof(*light));
	light->tag = TAG_LIGHT;

	light->dirty = 1;
	vec_init(light->location, 0, 0, 0);
	quat_init(light->rotation, 0, 0, 0, 1);
	vec_init(light->scale, 1, 1, 1);

	light->type = LIGHT_POINT;
	vec_init(light->color, 1, 1, 1);
	light->energy = 1;
	light->distance = 25;
	light->spot_angle = 45;

	LIST_INSERT_HEAD(&scene->lights, light, list);
	return light;
}

int attach_armature(struct armature *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	int i;

	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;

	for (i = 0; i < skel->count; i++) {
		if (!strcmp(skel->name[i], tagname)) {
			node->parent = parent;
			node->parent_tag = i;
			return 0;
		}
	}

	return -1;
}

int attach_object(struct object *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	struct skel *mskel = node->mesh->skel;
	int i, k;

	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;

	if (mskel) {
		for (i = 0; i < mskel->count; i++) {
			for (k = 0; k < skel->count; k++) {
				if (!strcmp(mskel->name[i], skel->name[k])) {
					node->parent_map[i] = k;
					break;
				}
			}
			if (k == skel->count)
				return -1; /* bone missing from skeleton */
		}
		node->parent = parent;
		return 0;
	}

	else if (tagname) {
		for (i = 0; i < skel->count; i++) {
			if (!strcmp(skel->name[i], tagname)) {
				node->parent = parent;
				node->parent_tag = i;
				return 0;
			}
		}
		return -1; /* tag not found */
	}

	return -1; /* no tag, no skin */
}

int attach_light(struct light *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	int i;

	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;

	for (i = 0; i < skel->count; i++) {
		if (!strcmp(skel->name[i], tagname)) {
			node->parent = parent;
			node->parent_tag = i;
			return 0;
		}
	}

	return -1;
}

void detach_armature(struct armature *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void detach_object(struct object *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void detach_light(struct light *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void play_anim(struct armature *node, struct anim *anim, float time)
{
	struct skel *skel = node->skel;
	struct skel *askel = anim->skel;
	struct anim_map *map;

	for (map = anim->anim_map_head; map; map = map->next)
		if (map->skel == node->skel)
			break;

	if (!map) {
		int i, k;

		map = malloc(sizeof(*map));
		map->skel = node->skel;
		map->next = anim->anim_map_head;
		anim->anim_map_head = map;

		for (i = 0; i < skel->count; i++) {
			for (k = 0; k < askel->count; k++) {
				if (!strcmp(skel->name[i], askel->name[k])) {
					map->anim_map[i] = k;
					break;
				}
			}
			if (k == askel->count)
				map->anim_map[i] = -1; /* bone missing from skeleton */
		}
	}

	node->anim_map = map->anim_map;
	node->anim = anim;
	node->time = time;
	node->dirty = 1;
}

void stop_anim(struct armature *node)
{
	node->anim_map = NULL;
	node->anim = NULL;
	node->time = 0;
	node->dirty = 1;
}

static int update_armature(struct armature *node)
{
	if (node->anim) {
		node->time += 0.5;
		node->dirty = 1;
	}

	if (node->parent)
		node->dirty |= update_armature(node->parent);

	if (node->dirty) {
		mat4 local_pose[MAXBONE];

		if (node->anim) {
			struct pose *skel_pose = node->skel->pose;
			struct pose anim_pose[MAXBONE];
			int i;

			// TODO: interpolate
			extract_frame(anim_pose, node->anim, (int)node->time % node->anim->frames);

			for (i = 0; i < node->skel->count; i++) {
				int k = node->anim_map[i];
				if (k >= 0)
					mat_from_pose(local_pose[i], anim_pose[k].location, anim_pose[k].rotation, anim_pose[k].scale);
				else
					mat_from_pose(local_pose[i], skel_pose[i].location, skel_pose[i].rotation, skel_pose[i].scale);
			}

			calc_abs_matrix(node->model_pose, local_pose, node->skel->parent, node->skel->count);
		} else {
			calc_matrix_from_pose(local_pose, node->skel->pose, node->skel->count);
			calc_abs_matrix(node->model_pose, local_pose, node->skel->parent, node->skel->count);
		}

		if (node->parent) {
			mat4 parent, local;
			mat_from_pose(local, node->location, node->rotation, node->scale);
			mat_mul(parent, node->parent->transform, node->parent->model_pose[node->parent_tag]);
			mat_mul(node->transform, parent, local);
		} else {
			mat_from_pose(node->transform, node->location, node->rotation, node->scale);
		}

		return 1;
	}

	return 0;
}

static void update_object_skin(struct object *node)
{
	mat4 *pose_matrix = node->parent->model_pose;
	mat4 *inv_bind_matrix = node->mesh->inv_bind_matrix;
	mat4 *skin_matrix = node->skin_matrix;
	unsigned char *parent_map = node->parent_map;
	int i, count = node->mesh->skel->count;
	for (i = 0; i < count; i++)
		mat_mul(skin_matrix[i], pose_matrix[parent_map[i]], inv_bind_matrix[i]);
}

static void update_object(struct object *node)
{
	if (node->parent)
		node->dirty |= update_armature(node->parent);

	if (node->dirty) {
		if (node->parent) {
			if (node->mesh->skel) {
				update_object_skin(node);
				mat_copy(node->transform, node->parent->transform);
			} else {
				mat4 parent, local;
				mat_from_pose(local, node->location, node->rotation, node->scale);
				mat_mul(parent, node->parent->transform, node->parent->model_pose[node->parent_tag]);
				mat_mul(node->transform, parent, local);
			}
		} else {
			mat_from_pose(node->transform, node->location, node->rotation, node->scale);
		}
	}

	node->dirty = 0;
}

static void update_light(struct light *node)
{
	if (node->parent)
		node->dirty |= update_armature(node->parent);

	if (node->dirty) {
		if (node->parent) {
			mat4 parent, local;
			mat_from_pose(local, node->location, node->rotation, node->scale);
			mat_mul(parent, node->parent->transform, node->parent->model_pose[node->parent_tag]);
			mat_mul(node->transform, parent, local);
		} else {
			mat_from_pose(node->transform, node->location, node->rotation, node->scale);
		}
	}

	node->dirty = 0;
}

void update_scene(struct scene *scene)
{
	struct object *obj;
	struct armature *amt;
	struct light *light;

	LIST_FOREACH(amt, &scene->armatures, list)
		update_armature(amt);
	LIST_FOREACH(obj, &scene->objects, list)
		update_object(obj);
	LIST_FOREACH(light, &scene->lights, list)
		update_light(light);

	LIST_FOREACH(amt, &scene->armatures, list)
		amt->dirty = 0;
}

void draw_armature(struct scene *scene, struct armature *node, mat4 projection, mat4 view)
{
	mat4 model_view;

	mat_mul(model_view, view, node->transform);

	draw_begin(projection, model_view);
	draw_set_color(1, 1, 1, 1);
	draw_skel(node->model_pose, node->skel->parent, node->skel->count);
	draw_end();
}

void draw_object(struct scene *scene, struct object *node, mat4 projection, mat4 view)
{
	mat4 model_view;

	mat_mul(model_view, view, node->transform);

	if (node->mesh->skel && node->parent)
		draw_mesh_with_pose(node->mesh, projection, model_view, node->skin_matrix);
	else
		draw_mesh(node->mesh, projection, model_view);
}

void draw_scene(struct scene *scene, mat4 projection, mat4 view)
{
	struct object *obj;

	LIST_FOREACH(obj, &scene->objects, list)
		draw_object(scene, obj, projection, view);
}

void draw_scene_debug(struct scene *scene, mat4 projection, mat4 view)
{
	struct armature *amt;

	glDisable(GL_DEPTH_TEST);
	LIST_FOREACH(amt, &scene->armatures, list) {
		draw_armature(scene, amt, projection, view);
	}
	glEnable(GL_DEPTH_TEST);
}
