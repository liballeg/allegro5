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


void display (void);


struct camera {
	double xangle, yangle, zangle;
	double dist;
};

struct camera camera = {
	0.0, 0.0, 0.0,
	20.0
};

double angle_speed = 5.0;
double dist_speed = 1.0;

int frames = 0;
volatile int secs;



void secs_timer(void)
{
	secs++;
}
END_OF_FUNCTION(secs_timer);



void set_camera_position (void)
{
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glFrustum (-1.0, 1.0, -1.0, 1.0, 1.0, 40.0);
	glMatrixMode (GL_MODELVIEW);
}



void keyboard (void)
{
	if (key[KEY_LEFT]) camera.yangle += angle_speed;
	if (key[KEY_RIGHT]) camera.yangle -= angle_speed;

	if (key[KEY_UP]) camera.xangle += angle_speed;
	if (key[KEY_DOWN]) camera.xangle -= angle_speed;

	if (key[KEY_PGUP]) camera.dist -= dist_speed;
	if (key[KEY_PGDN]) camera.dist += dist_speed;

	set_camera_position();
	display();
}

void display (void)
{
	// Clear the RGB buffer and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set the modelview matrix to be the identity matrix
	glLoadIdentity();
        
	//Set the camera
	glTranslatef (0, 0, -camera.dist);
	glRotatef (camera.xangle, 1, 0, 0);
	glRotatef (camera.yangle, 0, 1, 0);
	glRotatef (camera.zangle, 0, 0, 1);
	
	// Save the camera matrix
	glPushMatrix();
	
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

	// Get the camera matrix back
	glPopMatrix();
	
	// Translate for the cube
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



int main ()
{
	allegro_init();

	install_allegro_gl();
	
	allegro_gl_clear_settings();
	allegro_gl_set (AGL_COLOR_DEPTH, 32);
	allegro_gl_set (AGL_Z_DEPTH, 24);
	allegro_gl_set (AGL_FULLSCREEN, TRUE);
	allegro_gl_set (AGL_DOUBLEBUFFER, 1);
	allegro_gl_set (AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_Z_DEPTH
	              | AGL_DOUBLEBUFFER | AGL_FULLSCREEN);

	if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0) < 0) {
		allegro_message ("Error setting OpenGL graphics mode:\n%s\n"
		                 "Allegro GL error : %s\n",
		                 allegro_error, allegro_gl_error);
		return -1;
	}

	install_keyboard();
	install_timer();

	LOCK_FUNCTION(secs_timer);
	LOCK_VARIABLE(secs);

	glShadeModel (GL_FLAT);
	glEnable (GL_DEPTH_TEST);
	glCullFace (GL_BACK);
	glEnable (GL_CULL_FACE);

	install_int (secs_timer, 1000);

	do {
		keyboard();
	} while (!key[KEY_ESC]);

	set_gfx_mode (GFX_TEXT, 0, 0, 0, 0);

	allegro_message("Frames: %i, Seconds: %i, FPS: %f\n",
	                frames, secs, (float)frames / (float)secs);

	return 0;
}
END_OF_MAIN()

