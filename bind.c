#include "mio.h"

struct scene *scene = NULL;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *L = NULL;

static int ffi_print(lua_State *L)
{
	int i, n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1); /* tostring */
		lua_pushvalue(L, i); /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1); /* get result */
		if (!s) return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1) console_putc('\t');
		console_print(s);
		lua_pop(L, 1); /* pop result */
	}
	console_putc('\n');
	return 0;
}

static int ffi_traceback(lua_State *L)
{
	const char *msg = lua_tostring(L, 1);
	if (msg) {
		fprintf(stderr, "%s\n", msg);
		luaL_traceback(L, L, msg, 1);
	}
	else if (!lua_isnoneornil(L, 1)) { /* is there an error object? */
		if (!luaL_callmeta(L, 1, "__tostring")) /* try its 'tostring' metamethod */
			lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

static int docall(lua_State *L, int narg, int nres)
{
	int status;
	int base = lua_gettop(L) - narg;
	lua_pushcfunction(L, ffi_traceback);
	lua_insert(L, base);
	status = lua_pcall(L, narg, nres, base);
	lua_remove(L, base);
	return status;
}

void run_string(const char *cmd)
{
	if (!L) {
		console_printnl("[no command interpreter]");
		return;
	}

	int status = luaL_loadstring(L, cmd);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
		lua_pop(L, 1);
		return;
	}

	status = docall(L, 0, LUA_MULTRET);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
		lua_pop(L, 1);
		return;
	}

	if (lua_gettop(L) > 0) {
		lua_getglobal(L, "print");
		lua_insert(L, 1);
		lua_pcall(L, lua_gettop(L)-1, 0, 0);
	}
}

void run_file(const char *filename)
{
	if (!L) {
		console_printnl("[no command interpreter]");
		return;
	}

	int status = luaL_loadfile(L, filename);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
		lua_pop(L, 1);
		return;
	}

	status = docall(L, 0, 0);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
		lua_pop(L, 1);
		return;
	}
}

static void *checktag(lua_State *L, int n, int tag)
{
	luaL_checktype(L, n, LUA_TLIGHTUSERDATA);
	int *p = lua_touserdata(L, n);
	if (!p || *p != tag)
		luaL_argerror(L, n, "wrong userdata type");
	return p;
}

/* Misc */

static int ffi_register_archive(lua_State *L)
{
	register_archive(luaL_checkstring(L, 1));
	return 0;
}

static int ffi_register_directory(lua_State *L)
{
	register_directory(luaL_checkstring(L, 1));
	return 0;
}

static int ffi_load_font(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct font *font = load_font(name);
	if (!font)
		return luaL_error(L, "cannot load font: %s", name);
	lua_pushlightuserdata(L, font);
	return 1;
}

/* Model */

static int ffi_load_skel(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct skel *skel = load_skel(name);
	if (!skel)
		return luaL_error(L, "cannot load skel: %s", name);
	lua_pushlightuserdata(L, skel);
	return 1;
}

static int ffi_load_mesh(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct mesh *mesh = load_mesh(name);
	if (!mesh)
		return luaL_error(L, "cannot load mesh: %s", name);
	lua_pushlightuserdata(L, mesh);
	return 1;
}

static int ffi_load_anim(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct anim *anim = load_anim(name);
	if (!anim)
		return luaL_error(L, "cannot load anim: %s", name);
	lua_pushlightuserdata(L, anim);
	return 1;
}

/* Scene */

static int ffi_scn_new(lua_State *L)
{
	struct scene *scn = new_scene();
	lua_pushlightuserdata(L, scn);
	return 1;
}

/* Armature */

static int ffi_amt_new(lua_State *L)
{
	struct skel *skel = checktag(L, 1, TAG_SKEL);
	struct armature *amt = new_armature(scene, skel);
	if (!amt)
		return luaL_error(L, "cannot create armature");
	lua_pushlightuserdata(L, amt);
	return 1;
}

static int ffi_amt_set_parent(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	struct armature *parent = checktag(L, 2, TAG_ARMATURE);
	const char *tagname = luaL_checkstring(L, 3);
	if (armature_set_parent(amt, parent, tagname))
		return luaL_error(L, "cannot find bone: %s", tagname);
	return 0;
}

static int ffi_amt_clear_parent(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	armature_clear_parent(amt);
	return 0;
}

static int ffi_amt_set_position(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->position[0] = luaL_checknumber(L, 2);
	amt->position[1] = luaL_checknumber(L, 3);
	amt->position[2] = luaL_checknumber(L, 4);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_set_rotation(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->rotation[0] = luaL_checknumber(L, 2);
	amt->rotation[1] = luaL_checknumber(L, 3);
	amt->rotation[2] = luaL_checknumber(L, 4);
	amt->rotation[3] = luaL_checknumber(L, 5);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_set_scale(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->scale[0] = luaL_checknumber(L, 2);
	amt->scale[1] = luaL_checknumber(L, 3);
	amt->scale[2] = luaL_checknumber(L, 4);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_position(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	lua_pushnumber(L, amt->position[0]);
	lua_pushnumber(L, amt->position[1]);
	lua_pushnumber(L, amt->position[2]);
	return 3;
}

static int ffi_amt_rotation(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	lua_pushnumber(L, amt->rotation[0]);
	lua_pushnumber(L, amt->rotation[1]);
	lua_pushnumber(L, amt->rotation[2]);
	lua_pushnumber(L, amt->rotation[3]);
	return 4;
}

static int ffi_amt_scale(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	lua_pushnumber(L, amt->scale[0]);
	lua_pushnumber(L, amt->scale[1]);
	lua_pushnumber(L, amt->scale[2]);
	return 3;
}

static int ffi_amt_play_anim(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	struct anim *anim = checktag(L, 2, TAG_ANIM);
	float transition = luaL_checknumber(L, 3);
	play_anim(amt, anim, transition);
	return 0;
}

static int ffi_amt_stop_anim(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	stop_anim(amt);
	return 0;
}

/* Object */

static int ffi_obj_new(lua_State *L)
{
	struct mesh *mesh = checktag(L, 1, TAG_MESH);
	struct object *obj = new_object(scene, mesh);
	if (!obj)
		return luaL_error(L, "cannot create object");
	lua_pushlightuserdata(L, obj);
	return 1;
}

static int ffi_obj_set_parent(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	struct armature *parent = checktag(L, 2, TAG_ARMATURE);
	const char *tagname = lua_tostring(L, 3);
	if (object_set_parent(obj, parent, tagname)) {
		if (tagname)
			return luaL_error(L, "cannot find bone: %s", tagname);
		else
			return luaL_error(L, "skeleton mismatch");
	}
	return 0;
}

static int ffi_obj_clear_parent(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	object_clear_parent(obj);
	return 0;
}

static int ffi_obj_set_position(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->position[0] = luaL_checknumber(L, 2);
	obj->position[1] = luaL_checknumber(L, 3);
	obj->position[2] = luaL_checknumber(L, 4);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_rotation(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->rotation[0] = luaL_checknumber(L, 2);
	obj->rotation[1] = luaL_checknumber(L, 3);
	obj->rotation[2] = luaL_checknumber(L, 4);
	obj->rotation[3] = luaL_checknumber(L, 5);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_scale(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->scale[0] = luaL_checknumber(L, 2);
	obj->scale[1] = luaL_checknumber(L, 3);
	obj->scale[2] = luaL_checknumber(L, 4);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_color(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->color[0] = luaL_checknumber(L, 2);
	obj->color[1] = luaL_checknumber(L, 3);
	obj->color[2] = luaL_checknumber(L, 4);
	return 0;
}

static int ffi_obj_position(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->position[0]);
	lua_pushnumber(L, obj->position[1]);
	lua_pushnumber(L, obj->position[2]);
	return 3;
}

static int ffi_obj_rotation(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->rotation[0]);
	lua_pushnumber(L, obj->rotation[1]);
	lua_pushnumber(L, obj->rotation[2]);
	lua_pushnumber(L, obj->rotation[3]);
	return 4;
}

static int ffi_obj_scale(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->scale[0]);
	lua_pushnumber(L, obj->scale[1]);
	lua_pushnumber(L, obj->scale[2]);
	return 3;
}

static int ffi_obj_color(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->color[0]);
	lua_pushnumber(L, obj->color[1]);
	lua_pushnumber(L, obj->color[2]);
	return 3;
}

/* Light */

static const char *lamp_type_enum[] = { "POINT", "SPOT", "SUN", 0 };

static int ffi_lamp_new(lua_State *L)
{
	struct lamp *lamp = new_lamp(scene);
	if (!lamp)
		return luaL_error(L, "cannot create lamp");
	lua_pushlightuserdata(L, lamp);
	return 1;
}

static int ffi_lamp_set_parent(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	struct armature *parent = checktag(L, 2, TAG_ARMATURE);
	const char *tagname = luaL_checkstring(L, 3);
	if (lamp_set_parent(lamp, parent, tagname))
		return luaL_error(L, "cannot find bone: %s", tagname);
	return 0;
}

static int ffi_lamp_clear_parent(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp_clear_parent(lamp);
	return 0;
}

static int ffi_lamp_set_position(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->position[0] = luaL_checknumber(L, 2);
	lamp->position[1] = luaL_checknumber(L, 3);
	lamp->position[2] = luaL_checknumber(L, 4);
	lamp->dirty = 1;
	return 0;
}

static int ffi_lamp_set_rotation(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->rotation[0] = luaL_checknumber(L, 2);
	lamp->rotation[1] = luaL_checknumber(L, 3);
	lamp->rotation[2] = luaL_checknumber(L, 4);
	lamp->rotation[3] = luaL_checknumber(L, 5);
	lamp->dirty = 1;
	return 0;
}

static int ffi_lamp_set_color(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->color[0] = luaL_checknumber(L, 2);
	lamp->color[1] = luaL_checknumber(L, 3);
	lamp->color[2] = luaL_checknumber(L, 4);
	return 0;
}

static int ffi_lamp_set_type(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->type = luaL_checkoption(L, 2, NULL, lamp_type_enum);
	return 0;
}

static int ffi_lamp_set_energy(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->energy = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_distance(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->distance = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_spot_angle(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->spot_angle = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_spot_blend(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->spot_blend = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_use_sphere(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->use_sphere = lua_toboolean(L, 2);
	return 0;
}

static int ffi_lamp_set_use_shadow(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->use_shadow = lua_toboolean(L, 2);
	return 0;
}

static int ffi_lamp_position(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->position[0]);
	lua_pushnumber(L, lamp->position[1]);
	lua_pushnumber(L, lamp->position[2]);
	return 3;
}

static int ffi_lamp_rotation(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->rotation[0]);
	lua_pushnumber(L, lamp->rotation[1]);
	lua_pushnumber(L, lamp->rotation[2]);
	lua_pushnumber(L, lamp->rotation[3]);
	return 4;
}

static int ffi_lamp_color(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->color[0]);
	lua_pushnumber(L, lamp->color[1]);
	lua_pushnumber(L, lamp->color[2]);
	return 3;
}

static int ffi_lamp_type(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushstring(L, lamp_type_enum[lamp->type]);
	return 1;
}

static int ffi_lamp_energy(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->energy);
	return 1;
}

static int ffi_lamp_distance(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->distance);
	return 1;
}

static int ffi_lamp_spot_angle(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->spot_angle);
	return 1;
}

static int ffi_lamp_spot_blend(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->spot_blend);
	return 1;
}

static int ffi_lamp_use_sphere(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushboolean(L, lamp->use_sphere);
	return 1;
}

static int ffi_lamp_use_shadow(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushboolean(L, lamp->use_shadow);
	return 1;
}

void init_lua(void)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "print", ffi_print);

	lua_register(L, "register_archive", ffi_register_archive);
	lua_register(L, "register_directory", ffi_register_directory);

	/* draw */
	lua_register(L, "load_font", ffi_load_font);

	/* model */
	lua_register(L, "load_skel", ffi_load_skel);
	lua_register(L, "load_mesh", ffi_load_mesh);
	lua_register(L, "load_anim", ffi_load_anim);

	/* scene */
	lua_register(L, "scn_new", ffi_scn_new);

	lua_register(L, "amt_new", ffi_amt_new);
	lua_register(L, "amt_set_parent", ffi_amt_set_parent);
	lua_register(L, "amt_clear_parent", ffi_amt_clear_parent);
	lua_register(L, "amt_set_position", ffi_amt_set_position);
	lua_register(L, "amt_set_rotation", ffi_amt_set_rotation);
	lua_register(L, "amt_set_scale", ffi_amt_set_scale);
	lua_register(L, "amt_position", ffi_amt_position);
	lua_register(L, "amt_rotation", ffi_amt_rotation);
	lua_register(L, "amt_scale", ffi_amt_scale);
	lua_register(L, "amt_play_anim", ffi_amt_play_anim);
	lua_register(L, "amt_stop_anim", ffi_amt_stop_anim);

	lua_register(L, "obj_new", ffi_obj_new);
	lua_register(L, "obj_set_parent", ffi_obj_set_parent);
	lua_register(L, "obj_clear_parent", ffi_obj_clear_parent);
	lua_register(L, "obj_set_position", ffi_obj_set_position);
	lua_register(L, "obj_set_rotation", ffi_obj_set_rotation);
	lua_register(L, "obj_set_scale", ffi_obj_set_scale);
	lua_register(L, "obj_set_color", ffi_obj_set_color);
	lua_register(L, "obj_position", ffi_obj_position);
	lua_register(L, "obj_rotation", ffi_obj_rotation);
	lua_register(L, "obj_scale", ffi_obj_scale);
	lua_register(L, "obj_color", ffi_obj_color);

	lua_register(L, "lamp_new", ffi_lamp_new);
	lua_register(L, "lamp_set_parent", ffi_lamp_set_parent);
	lua_register(L, "lamp_clear_parent", ffi_lamp_clear_parent);
	lua_register(L, "lamp_set_position", ffi_lamp_set_position);
	lua_register(L, "lamp_set_rotation", ffi_lamp_set_rotation);
	lua_register(L, "lamp_set_type", ffi_lamp_set_type);
	lua_register(L, "lamp_set_color", ffi_lamp_set_color);
	lua_register(L, "lamp_set_energy", ffi_lamp_set_energy);
	lua_register(L, "lamp_set_distance", ffi_lamp_set_distance);
	lua_register(L, "lamp_set_spot_angle", ffi_lamp_set_spot_angle);
	lua_register(L, "lamp_set_spot_blend", ffi_lamp_set_spot_blend);
	lua_register(L, "lamp_set_use_sphere", ffi_lamp_set_use_sphere);
	lua_register(L, "lamp_set_use_shadow", ffi_lamp_set_use_shadow);
	lua_register(L, "lamp_position", ffi_lamp_position);
	lua_register(L, "lamp_rotation", ffi_lamp_rotation);
	lua_register(L, "lamp_type", ffi_lamp_type);
	lua_register(L, "lamp_color", ffi_lamp_color);
	lua_register(L, "lamp_energy", ffi_lamp_energy);
	lua_register(L, "lamp_distance", ffi_lamp_distance);
	lua_register(L, "lamp_spot_angle", ffi_lamp_spot_angle);
	lua_register(L, "lamp_spot_blend", ffi_lamp_spot_blend);
	lua_register(L, "lamp_use_sphere", ffi_lamp_use_sphere);
	lua_register(L, "lamp_use_shadow", ffi_lamp_use_shadow);
}
