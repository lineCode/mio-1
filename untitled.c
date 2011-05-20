#include "mio.h"

#include "syshook.h"
#include "sysevent.h"

static struct md2_model *model1;
static struct model *model2;
static struct model *model3;
static struct model *village;
static struct model *tree;
static struct model *skydome1;
static struct model *skydome2;
static struct font *font;
// static int grasstex[4];

static float camera_dir[3] = { 0, 0, 0 };
static float camera_pos[3] = { 0, -5, 1.8 };
static float camera_rot[3] = { 0, 0, 0 };
static int action_forward = 0;
static int action_backward = 0;
static int action_left = 0;
static int action_right = 0;

float sunpos[] = { -5, -5, 10, 0 };
float suncolor[] = { 1, 1, 1, 1 };
float torchpos[] = { 0, 0, 1, 1 };
float torchcolor[] = { 1, 1, 0.5, 1 };
float ambientcolor[] = { 0.1, 0.1, 0.1, 1 };
float fogcolor[4] = { 73.0/255, 149.0/255, 204.0/255, 1 };

void perspective(float fov, float aspect, float near, float far)
{
	fov = fov * 3.14159 / 360.0;
	fov = tan(fov) * near;
	glFrustum(-fov * aspect, fov * aspect, -fov, fov, near, far);
}

void rotvec(float a, float b, float c, float *in, float *out)
{
	float xp, yp, zp;
	float x, y, z;
	x = in[0];
	y = in[1];
	z = in[2];

	// - x -
	yp = y * cos(a) - z * sin(a);
	zp = y * sin(a) + z * cos(a);
	y = yp;
	z = zp;

	// - y -
	xp = x * cos(b) + z * sin(b);
	zp = -x * sin(b) + z * cos(b);
	x = xp;
	z = zp;

	// - z -
	xp = x * cos(c) - y * sin(c);
	yp = x * sin(c) + y * cos(c);
	x = xp;
	y = yp;

	out[0] = x;
	out[1] = y;
	out[2] = z;
}

void updatecamera(void)
{
	float dir[3];
	float speed = 0.1;
	rotvec(camera_rot[0]*M_PI/180, camera_rot[1]*M_PI/180, camera_rot[2]*M_PI/180, camera_dir, dir);
	camera_pos[0] += dir[0] * speed;
	camera_pos[1] += dir[1] * speed;
	camera_pos[2] += dir[2] * speed;
}

void sys_hook_init(int argc, char **argv)
{
	char buf[100];
	float one = 1;
	int i;

	printf("loading data files...\n");

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glShadeModel(GL_SMOOTH);
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, fogcolor);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, &one);

	font = load_font("data/CharterBT-Roman.ttf");
	model1 = md2_load_model("data/soldier.md2");
	//model2 = load_obj_model("data/ca_hom/ca_hom_armor01.obj");
	//model2 = load_obj_model("data/tr_mo_kami_caster/tr_mo_kami_caster.obj");
	model2 = load_obj_model("data/tr_mo_cute/tr_mo_cute.obj");
	load_obj_animation(model2, "data/tr_mo_cute/tr_mo_cute_idle_occupation1.pc2");
	model3 = load_obj_model("data/ca_hom/ca_hom_armor01.obj");
	village = load_obj_model("data/village-a/villagea.obj");
	tree = load_obj_model("data/vegetation/ju_s2_big_tree.obj");

#if 0
	printf("loading terrain textures\n");
	for (i = 0; i < nelem(grasstex); i++) {
		sprintf(buf, "data/terrain/y-bassesiles-256-a-%02d.png", i+1);
		grasstex[i] = load_texture(buf, NULL, NULL, NULL, 0);
	}
#endif

	// init_sky_dome(15, 15);
	// skytex = load_texture("data/DuskStormonHorizon.jpg", 0, 0, 0, 0);
	skydome1 = load_obj_model("data/sky/sky_fog_tryker.obj");
	skydome2 = load_obj_model("data/sky/canope_tryker.obj");

	printf("all done\n");

	sys_start_idle_loop();
}

void sys_hook_draw(int w, int h)
{
	struct event *evt;
	static int mousex = 0, mousey = 0, mousez = 0;

	while ((evt = sys_read_event()))
	{
		if (evt->type == SYS_EVENT_KEY_CHAR)
		{
			if (evt->key == ' ')
				sys_stop_idle_loop();
			if (evt->key == 'r')
				sys_start_idle_loop();
			if (evt->key == '\r' && (evt->mod & SYS_MOD_ALT)) {
				if (sys_is_fullscreen())
					sys_leave_fullscreen();
				else
					sys_enter_fullscreen();
			}
			if (evt->key == 27)
				exit(0);
		}
		if (evt->type == SYS_EVENT_KEY_DOWN) {
			switch (evt->key) {
			case SYS_KEY_UP: action_forward = 1; break;
			case SYS_KEY_DOWN: action_backward = 1; break;
			case SYS_KEY_LEFT: action_left = 1; break;
			case SYS_KEY_RIGHT: action_right = 1; break;
			}
		}
		if (evt->type == SYS_EVENT_KEY_UP) {
			switch (evt->key) {
			case SYS_KEY_UP: action_forward = 0; break;
			case SYS_KEY_DOWN: action_backward = 0; break;
			case SYS_KEY_LEFT: action_left = 0; break;
			case SYS_KEY_RIGHT: action_right = 0; break;
			}
		}
		if (evt->type == SYS_EVENT_MOUSE_MOVE)
		{
			mousex = evt->x;
			mousey = evt->y;
		}
		if (evt->type == SYS_EVENT_MOUSE_DOWN)
		{
			if (evt->btn == 5)
				mousez++;
			else if (evt->btn == 4)
				mousez--;
		}
	}

	glViewport(0, 0, w, h);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static float angle = 0.0;
	static float lerp = 0.0;
	static int idx = 0;
	angle += 1.0;
	if (angle > 360)
		angle = 0;
	lerp += 15.0/60.0;
	if (lerp > 1.0)
	{
		lerp = 0.0;
		idx ++;
	}
	if (idx == 261) idx = 0;
	if (idx == md2_get_frame_count(model1) - 1)
		idx = 0;

	/*
	 * Update camera
	 */

	float dir[3] = {0, 1, 0};
	if (action_forward) camera_dir[1] = 1;
	else if (action_backward) camera_dir[1] = -1;
	else camera_dir[1] = 0;
	if (action_left) camera_rot[2] += 3;
	if (action_right) camera_rot[2] -= 3;
	updatecamera();

	 /*
	 * Draw rotating models
	 */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspective(60, (float) w / h, 0.05, 6000); /* 5cm to 6km clip planes */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(-90, 1, 0, 0); /* Z-up */
	glRotatef(-camera_rot[2], 0, 0, 1);
	glRotatef(-camera_rot[0], 1, 0, 0);
	glRotatef(-camera_rot[1], 0, 1, 0);
	glTranslatef(-camera_pos[0], -camera_pos[1], -camera_pos[2]);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_LIGHTING);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, sunpos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, suncolor);

	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, torchpos);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, torchcolor);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 1.0/15);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fogcolor);
	glFogf(GL_FOG_DENSITY, 0.30f);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_START, 1.0f);
	glFogf(GL_FOG_END, 1000.0f);
	glEnable(GL_FOG);

	glPushMatrix();
	glTranslatef(-1, 0, 0);
	md2_draw_frame_lerp(model1, 0, idx, idx + 1, lerp);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1, 0, 0);
	draw_obj_model_frame(model2, idx);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 1, 0);
	draw_obj_model(model3);
	glPopMatrix();

	glPushMatrix();
	draw_obj_model(village);
	glPopMatrix();

#if 0
	glPushMatrix();
	glScalef(2, 2, 1);
	{
		int row, col, tex = 0;
		for (row = -40; row <= 40; row++)
		{
			for (col = -40; col <= 40; col++)
			{
				glBindTexture(GL_TEXTURE_2D, grasstex[tex]);
				glBegin(GL_QUADS);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(row + 0, col + 0, 0);
				glTexCoord2f(1, 0); glVertex3f(row + 1, col + 0, 0);
				glTexCoord2f(1, 1); glVertex3f(row + 1, col + 1, 0);
				glTexCoord2f(0, 1); glVertex3f(row + 0, col + 1, 0);
				glEnd();
				tex = (tex + 1) % nelem(grasstex);
			}
		}
	}
	glPopMatrix();
#endif

	glPushMatrix();
	glScalef(5, 5, 5);
	draw_obj_model(skydome2);
	glPopMatrix();

	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glPushMatrix();
	glScalef(20, 20, 20);
	draw_obj_model(skydome1);
	glPopMatrix();

	/* draw tree twice, once with alpha testing and depth write */
	/* then again with blending and no depth write (or test) */
	glDisable(GL_CULL_FACE);

	glEnable(GL_FOG);
	glEnable(GL_LIGHTING);
	glPushMatrix();
	glTranslatef(20, 40, 0);

	glAlphaFunc(GL_GREATER, 0.9);
	glEnable(GL_ALPHA_TEST);
	draw_obj_model(tree);
	glDisable(GL_ALPHA_TEST);

	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	draw_obj_model(tree);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glPopMatrix();
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);

	glEnable(GL_CULL_FACE);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	/* Draw x-y-z-axes */
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glColor4f(1, 0, 0, 1);
	glVertex3f(0, 0, 0); glVertex3f(1, 0, 0);
	glColor4f(0, 1, 0, 1);
	glVertex3f(0, 0, 0); glVertex3f(0, 1, 0);
	glColor4f(0, 0, 1, 1);
	glVertex3f(0, 0, 0); glVertex3f(0, 0, 1);
	glEnd();

	/*
	* Draw text overlay
	*/

	char status[100];

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor3f(1, 0.70, 0.7);
	float x;
	sprintf(status, "Angle: %g", angle);
	draw_string(font, 20, 20, 30, status);
	sprintf(status, "Frame: %d", idx);
	x = measure_string(font, 20, status);
	draw_string(font, 20, w - x - 20, 30, status);

	#if 0
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(10, 50);
	glTexCoord2f(0, 1); glVertex2f(10, 50 + 512);
	glTexCoord2f(1, 1); glVertex2f(10 + 512, 50 + 512);
	glTexCoord2f(1, 0); glVertex2f(10 + 512, 50);
	glEnd();
	#endif

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}
