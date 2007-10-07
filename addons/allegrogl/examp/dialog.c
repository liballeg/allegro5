/*----------------------------------------------------------------
 * test.c -- Allegro-GL test program
 *----------------------------------------------------------------
 *  Based on firstogl.c.
 */


#include <allegro.h>

/* Note: Under Windows, if you include any Windows headers (e.g. GL/gl.h),
 * you must include <winalleg.h> first.  To save you working out when this
 * occurs, `alleggl.h' handles this for you, and includes <GL/gl.h>.  If 
 * you need other GL headers, include them afterwards.
 *
 * Also note that user programs, outside of this library distribution,
 * should use <...> notation for `alleggl.h'.
 */
#include "alleggl.h"


static void display (void);


struct camera {
	double xangle, yangle, zangle;
	double dist;
};

static struct camera camera = {
	0.0, 0.0, 0.0,
	20.0
};

static double angle_speed = 5.0;
static double dist_speed = 1.0;

static int frames = 0;
static volatile int secs;

static void secs_timer(void)
{
	secs++;
}
END_OF_STATIC_FUNCTION(secs_timer);

static void set_camera_position (void)
{
	/* Note: Normally, you won't build the camera using the Projection
	 * matrix. See excamera.c on how to build a real camera.
	 * This code is there just to simplify this test program a bit.
	 */
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glFrustum (-1.0, 1.0, -1.0, 1.0, 1.0, 40.0);
	glTranslatef (0, 0, -camera.dist);
	glRotatef (camera.xangle, 1, 0, 0);
	glRotatef (camera.yangle, 0, 1, 0);
	glRotatef (camera.zangle, 0, 0, 1);
	glMatrixMode (GL_MODELVIEW);
}

static void keyboard (void)
{
	if (key[KEY_LEFT]) camera.yangle += angle_speed;
	if (key[KEY_RIGHT]) camera.yangle -= angle_speed;

	if (key[KEY_UP]) camera.xangle += angle_speed;
	if (key[KEY_DOWN]) camera.xangle -= angle_speed;

	if (key[KEY_PGUP]) camera.dist -= dist_speed;
	if (key[KEY_PGDN]) camera.dist += dist_speed;

	if (key[KEY_D]) alert ("You pressed D", NULL, NULL, "OK", NULL, 0, 0);

	set_camera_position();
	display();
}

static void display (void)
{
        // Clear the RGB buffer and the depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set the modelview matrix to be the identity matrix
        glLoadIdentity();
        // Translate and rotate the object
        glTranslatef(-2.5, 0.0, 0.0);
        glRotatef(-30, 1.0, 0.0, 0.0);
        glRotatef(30, 0.0, 1.0, 0.0);
        glRotatef(30, 0.0, 0.0, 1.0);

        glColor3f(1.0, 0.0, 1.0);

        // Draw the sides of the three-sided pyramid
        glBegin(GL_TRIANGLE_FAN);
                glVertex3d(0, 4, 0);
                glVertex3d(0, -4, -4);
                glVertex3d(-4, -4, 4);
                glVertex3d(4, -4, 4);
                glVertex3d(0, -4, -4);
        glEnd();

        glColor3f(0.0, 1.0, 1.0);

        // Draw the base of the pyramid
        glBegin(GL_TRIANGLES);
                glVertex3d(0, -4, -4);
                glVertex3d(4, -4, 4);
                glVertex3d(-4, -4, 4);
        glEnd();

        glLoadIdentity();
        glTranslatef(2.5, 0.0, 0.0);
        glRotatef(45, 1.0, 0.0, 0.0);
        glRotatef(45, 0.0, 1.0, 0.0);
        glRotatef(45, 0.0, 0.0, 1.0);

        glColor3f(0.0, 1.0, 0.0);

        // Draw the sides of the cube
        glBegin(GL_QUAD_STRIP);
                glVertex3d(3, 3, -3);
                glVertex3d(3, -3, -3);
                glVertex3d(-3, 3, -3);
                glVertex3d(-3, -3, -3);
                glVertex3d(-3, 3, 3);
                glVertex3d(-3, -3, 3);
                glVertex3d(3, 3, 3);
                glVertex3d(3, -3, 3);
                glVertex3d(3, 3, -3);
                glVertex3d(3, -3, -3);
        glEnd();

        glColor3f(0.0, 0.0, 1.0);

        // Draw the top and bottom of the cube
        glBegin(GL_QUADS);
                glVertex3d(-3, -3, -3);
                glVertex3d(3, -3, -3);
                glVertex3d(3, -3, 3);
                glVertex3d(-3, -3, 3);

                glVertex3d(-3, 3, -3);
                glVertex3d(-3, 3, 3);
                glVertex3d(3, 3, 3);
                glVertex3d(3, 3, -3);
        glEnd();

	glFlush();

	allegro_gl_flip();

	frames++;
}



static int mode = AGL_FULLSCREEN, width, height;



struct resolution_entry {
	int w,h;
	char *s;
} resolutions[] = {
	{  320,  240, " 320x240 "  },
	{  640,  480, " 640x480 "  },
	{  800,  600, " 800x600 "  },
	{ 1024,  768, "1024x768 "  },
	{ 1280, 1024, "1280x1024" },
	{ 1600, 1200, "1600x1200" }
};

static char *resolution_lister (int i, int *size)
{
	if (i < 0) {
		*size = sizeof resolutions / sizeof *resolutions;
		return NULL;
	}
	return resolutions[i].s;
}



struct colour_depth_entry {
	int depth;
	char *s;
} colour_depths[] = {
	{  8, " 8 bpp" },
	{ 15, "15 bpp" },
	{ 16, "16 bpp" },
	{ 24, "24 bpp" },
	{ 32, "32 bpp" }
};

static char *colour_depth_lister (int i, int *size)
{
	if (i < 0) {
		*size = sizeof colour_depths / sizeof *colour_depths;
		return NULL;
	}
	return colour_depths[i].s;
}

struct zbuffer_depth_entry {
	int depth;
	char *s;
} zbuffer_depths[] = {
	{  8, " 8 bits" },
	{ 16, "16 bits" },
	{ 24, "24 bits" },
	{ 32, "32 bits" }
};

static char *zbuffer_depth_lister (int i, int *size)
{
	if (i < 0) {
		*size = sizeof zbuffer_depths / sizeof *zbuffer_depths;
		return NULL;
	}
	return zbuffer_depths[i].s;
}


static int setup (void)
{
	#define RESOLUTION_LIST   4
	#define COLOUR_LIST       6
	#define ZBUFFER_LIST      8
	#define WINDOWED_BOX      9
	#define DOUBLEBUFFER_BOX 10
	#define BUTTON_OK        11
	
	DIALOG dlg[] = {
    /*	proc                 x    y    w    h  fg bg  key    flags d1 d2  dp */
    {	d_shadow_box_proc,   0,   0, 320, 200,  0, 0,   0,         0, 0, 0, NULL, NULL, NULL },
    {	d_ctext_proc,      160,  10,   0,   0,  0, 0,   0,         0, 0, 0, "____________", NULL, NULL },
    {	d_ctext_proc,      160,   8,   0,   0,  0, 0,   0,         0, 0, 0, "OpenGL Setup", NULL, NULL },
    {	d_text_proc,        10,  30,   0,   0,  0, 0,   0,         0, 0, 0, "Resolution", NULL, NULL },
    {	d_list_proc,        10,  40,  96,  52,  0, 0,   0,         0, 0, 0, resolution_lister, NULL, NULL },
    {	d_text_proc,       120,  30,   0,   0,  0, 0,   0,         0, 0, 0, "Colour depth", NULL, NULL },
    {	d_list_proc,       120,  40,  96,  48,  0, 0,   0,         0, 0, 0, colour_depth_lister, NULL, NULL },
    {	d_text_proc,        10, 104,  96,  48,  0, 0,   0,         0, 0, 0, "Z buffer", NULL, NULL },
    {	d_list_proc,        10, 114,  96,  48,  0, 0,   0,         0, 0, 0, zbuffer_depth_lister, NULL, NULL },
    {	d_check_proc,       10, 170,  96,   8,  0, 0,   0,         0, 1, 0, "Windowed", NULL, NULL },
    {	d_check_proc,       10, 180, 128,   8,  0, 0,   0,D_SELECTED, 1, 0, "Double Buffered", NULL, NULL },
    {	d_button_proc,     220, 150,  96,  18,  0, 0,   0,    D_EXIT, 0, 0, "Ok", NULL, NULL },
    {	d_button_proc,     220, 174,  96,  18,  0, 0,   0,    D_EXIT, 0, 0, "Exit", NULL, NULL },
    {	d_yield_proc,        0,   0,   0,   0,  0, 0,   0,         0, 0, 0, NULL, NULL, NULL },
    {	NULL,                0,   0,   0,   0,  0, 0,   0,         0, 0, 0, NULL, NULL, NULL }
	};

	int x;

	if (mode == AGL_WINDOWED)
		dlg[WINDOWED_BOX].flags |= D_SELECTED;

	centre_dialog (dlg);
	set_dialog_color (dlg, makecol(0, 0, 0), makecol(255, 255, 255));
		
	if (secs) {
		textprintf_centre_ex(screen, font, SCREEN_W / 2, SCREEN_H / 2 + 110,
		                  makecol(255, 255, 255), 0,
		                  "Frames: %i, Seconds: %i, FPS: %f",
		                  frames, secs, (float)frames / (float)secs);
	}
	
	x = do_dialog (dlg, 4);
	
	allegro_gl_clear_settings();
	allegro_gl_set(AGL_COLOR_DEPTH, colour_depths[dlg[COLOUR_LIST].d1].depth);
	allegro_gl_set(AGL_Z_DEPTH, zbuffer_depths[dlg[ZBUFFER_LIST].d1].depth);
	allegro_gl_set(AGL_DOUBLEBUFFER,
	              (dlg[DOUBLEBUFFER_BOX].flags & D_SELECTED) ? 1 : 0);
	allegro_gl_set(AGL_RENDERMETHOD, 1);
	
	mode = ((dlg[WINDOWED_BOX].flags & D_SELECTED)
	       ? AGL_WINDOWED : AGL_FULLSCREEN);
	
	allegro_gl_set(mode, TRUE);
	allegro_gl_set(AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_Z_DEPTH
	                          | AGL_DOUBLEBUFFER | AGL_RENDERMETHOD | mode);
	
	width  = resolutions[dlg[RESOLUTION_LIST].d1].w;
	height = resolutions[dlg[RESOLUTION_LIST].d1].h;

	return (x == BUTTON_OK);
}


static void run_demo (void)
{
	set_color_depth (16);
	if (set_gfx_mode(GFX_OPENGL, width, height, 0, 0) < 0) {
		allegro_message ("Error setting OpenGL graphics mode:\n%s\nAllegro GL error : %s\n", allegro_error, allegro_gl_error);
		return;
	}

	install_keyboard();

	LOCK_FUNCTION(secs_timer);
	LOCK_VARIABLE(secs);

	glClearColor (0, 0, 0, 0);
	glShadeModel (GL_FLAT);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glPolygonMode (GL_BACK, GL_POINTS);
	glEnable (GL_DEPTH_TEST);
	glCullFace (GL_BACK);
	glEnable (GL_CULL_FACE);

	install_int (secs_timer, 1000);

	do {
		keyboard();
		rest(2);
	} while (!key[KEY_ESC]);

	remove_int (secs_timer);

	remove_keyboard();
}



int main()
{
	int ok;
	allegro_init();
	install_timer();

	do {
		set_color_depth(8);
		if (set_gfx_mode (GFX_AUTODETECT, 640, 480, 0, 0) < 0) {
			allegro_message ("Error setting plain graphics mode:\n%s\n", allegro_error);
			return -1;
		}
		
		install_allegro_gl();

		install_keyboard();
		install_mouse();
		ok = setup();
		remove_keyboard();
		remove_mouse();

		if (ok) run_demo();

		remove_allegro_gl();
	} while (ok);

	return 0;
}
END_OF_MAIN()

