/*
 *    Example program for the Allegro library, by Shawn Hargreaves
 *    converted to OpenGL/AllegroGL.
 *
 *    This program demonstrates how to easily manipulate a camera
 *    in OpenGL to view a 3d world from any position and angle.
 *    Quaternions are used, because they're so easy to work with!
 */


#include <stdio.h>
#include <math.h>

#include <allegro.h>
#include <alleggl.h>
#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif


/* Define M_PI in case the compiler doesn't */
#ifndef M_PI
   #define M_PI   3.1415926535897932384626433832795
#endif


/* Define a 3D vector type */
typedef struct VECTOR {
	float x, y, z;
} VECTOR;


/* display a nice 12x12 chessboard grid */
#define GRID_SIZE    12



/* Parameters controlling the camera and projection state */
int viewport_w = 320;  /* Viewport Width  (pixels)  */
int viewport_h = 240;  /* Viewport Height (pixels)  */
int fov = 48;          /* Field of view   (degrees) */
float aspect = 1;      /* Aspect ratio              */

/* Define the camera 
 * We need: One position vector, and one orientation QUAT
 */
struct CAMERA {
	VECTOR position;
	QUAT orientation;
} camera;



/* A simple font to display some info on screen */
FONT *agl_font;



/* Sets up the viewport to designated values */
void set_viewport() {
	glViewport((SCREEN_W - viewport_w) / 2, (SCREEN_H - viewport_h) / 2,
	           viewport_w, viewport_h);
}



/* Sets up the camera for displaying the world */
void set_camera() {
	float theta;

	/* First, we set up the projection matrix.
	 * Note that SCREEN_W / SCREEN_H = 1.333333, so we need to multiply the
	 * aspect ratio by that value so that the display doesn't get distorted.
	 */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective((float)fov, aspect * 1.333333, 1.0, 120.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Macro to convert radians to degrees */
	#define RAD_2_DEG(x) ((x) * 180 / M_PI)

	/* Convert the QUAT to something OpenGL can understand 
	 * We can use allegro_gl_apply_quat() here, but I'd just like
	 * to show how it can be done with regular GL code.
	 *
	 * Since we're working with the camera, we have to rotate first,
	 * and then translate. Objects are done the other way around.
	 */
	theta = RAD_2_DEG(2 * acos(camera.orientation.w));
	if (camera.orientation.w < 1.0f && camera.orientation.w > -1.0f) {
		glRotatef(theta, camera.orientation.x, camera.orientation.y,
			camera.orientation.z);
	}

	glTranslatef(-camera.position.x, -camera.position.y, -camera.position.z);

	#undef RAD_2_DEG
}



/* Draw the (simple) world 
 * Notice how the camera doesn't affect the positioning.
 */
void draw_field() {

	int i, j;
	
	for (j = 0; j < GRID_SIZE; j++) {
		for (i = 0; i < GRID_SIZE; i++) {
			glPushMatrix();
			glTranslatef(i * 2 - GRID_SIZE + 1, -2, j * 2 - GRID_SIZE + 1);
			
			if ((i + j) & 1) {
				glColor3ub(255, 255, 0);
			}
			else {
				glColor3ub(0, 255, 0);
			}

			glBegin(GL_QUADS);
				glVertex3f(-1, 0, -1);
				glVertex3f(-1, 0,  1);
				glVertex3f( 1, 0,  1);
				glVertex3f( 1, 0, -1);
			glEnd();
			glPopMatrix();
		}
	}
}



/* For display, we'd like to convert the QUAT back to heading, pitch and roll
 * These don't serve any purpose but to make it look human readable.
 * Note: Produces incorrect results.
 */
void convert_quat(QUAT *q, float *heading, float *pitch, float *roll) {
	MATRIX_f matrix;
	quat_to_matrix(q, &matrix);
	
	*heading = atan2(matrix.v[0][2], matrix.v[0][0]);
	*pitch   = asin(matrix.v[0][1]);
	*roll    = atan2(matrix.v[2][1], matrix.v[2][0]);
}



/* Draws the overlay over the field. The position of the overlay is
 * independent of the camera.
 */
void draw_overlay() {
	float heading, pitch, roll;
	int color;
	VECTOR v;

	/* Set up the viewport so that it takes up the whole screen */
	glViewport(0, 0, SCREEN_W, SCREEN_H);

	/* Draw a line around the viewport */
	allegro_gl_set_projection();

	glColor3ub(255, 0, 0);
	glDisable(GL_DEPTH_TEST);
	
	glBegin(GL_LINE_LOOP);
		glVertex2i((SCREEN_W - viewport_w) / 2, (SCREEN_H - viewport_h) / 2);
		glVertex2i((SCREEN_W + viewport_w) / 2 - 1,
		           (SCREEN_H - viewport_h) / 2);
		glVertex2i((SCREEN_W + viewport_w) / 2 - 1,
		           (SCREEN_H + viewport_h) / 2 - 1);
		glVertex2i((SCREEN_W - viewport_w) / 2,
		           (SCREEN_H + viewport_h) / 2 - 1);
	glEnd();

	/* Overlay some text describing the current situation */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	color = 0;
	glTranslatef(-0.375, -0.375, 0);
	allegro_gl_printf(agl_font, 0,  0, 0, color,
	                  "Viewport width:  %03d pix (w/W changes)", viewport_w);
	allegro_gl_printf(agl_font, 0,  8, 0, color,
	                  "Viewport height: %03d pix (h/H changes)", viewport_h);
	allegro_gl_printf(agl_font, 0, 16, 0, color,
	                  "Field Of View:    %02d deg (f/F changes)", fov);
	allegro_gl_printf(agl_font, 0, 24, 0, color,
	                  "Aspect Ratio:    %.2f   (a/A changes)", aspect);
	allegro_gl_printf(agl_font, 0, 32, 0, color,
	                  "X position:     %+.2f (x/X changes)", camera.position.x);
	allegro_gl_printf(agl_font, 0, 40, 0, color,
	                  "Y position:     %+.2f (y/Y changes)", camera.position.y);
	allegro_gl_printf(agl_font, 0, 48, 0, color,
	                  "Z position:     %+.2f (z/Z changes)", camera.position.z);

	/* Convert the orientation QUAT into heading, pitch and roll to display */
	convert_quat(&camera.orientation, &heading, &pitch, &roll);

	allegro_gl_printf(agl_font, 0, 56, 0, color,
	    "Heading:        %+.2f deg (left/right changes)", heading * 180 / M_PI);
	allegro_gl_printf(agl_font, 0, 64, 0, color,
	    "Pitch:          %+.2f deg (pgup/pgdn changes)", pitch * 180 / M_PI);
	allegro_gl_printf(agl_font, 0, 72, 0, color,
	    "Roll:           %+.2f deg (r/R changes)", roll * 180 / M_PI);
	
	apply_quat(&camera.orientation, 0, 0, -1, &v.x, &v.y, &v.z);
	
	allegro_gl_printf(agl_font, 0, 80, 0, color,
	                 "Front Vector:    %.2f, %.2f, %.2f", v.x, v.y, v.z);
	
	apply_quat(&camera.orientation, 0, 1, 0, &v.x, &v.y, &v.z);	
	
	allegro_gl_printf(agl_font, 0, 88, 0, color,
	                  "Up Vector:       %.2f, %.2f, %.2f", v.x, v.y, v.z);
	allegro_gl_printf(agl_font, 0, 96, 0, color,
	          "QUAT:  %f, %f, %f, %f ", camera.orientation.w,
	          camera.orientation.x, camera.orientation.y, camera.orientation.z);
	
	allegro_gl_unset_projection();
	
	glBlendFunc(GL_ONE, GL_ZERO);
	glEnable(GL_DEPTH_TEST);
}



/* draw everything */
void render()
{
	set_viewport();
	
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	set_camera();
	
	draw_field();
	
	draw_overlay();
	
	glFlush();
	
	allegro_gl_flip();
}



/* deal with user input */
void process_input(void) {

	QUAT q;

	poll_keyboard();

	if (key[KEY_W]) {
		if (key_shifts & KB_SHIFT_FLAG) {
			if (viewport_w < SCREEN_W)
				viewport_w += 8;
		}
		else { 
			if (viewport_w > 16)
				viewport_w -= 8;
		}
	}

	if (key[KEY_H]) {
		if (key_shifts & KB_SHIFT_FLAG) {
			if (viewport_h < SCREEN_H)
				viewport_h += 8;
		}
		else {
			if (viewport_h > 16)
				viewport_h -= 8;
		}
	}

	if (key[KEY_X]) {
		if (key_shifts & KB_SHIFT_FLAG)
			camera.position.x += 0.05;
		else
			camera.position.x -= 0.05;
	}

	if (key[KEY_Y]) {
		if (key_shifts & KB_SHIFT_FLAG)
			camera.position.y += 0.05;
		else
			camera.position.y -= 0.05;
	}

	if (key[KEY_Z]) {
		if (key_shifts & KB_SHIFT_FLAG)
			camera.position.z += 0.05;
		else
			camera.position.z -= 0.05;
	}
	
	if (key[KEY_UP]) {
		VECTOR front;
		/* Note: We use -1 here because Allegro's coordinate system
		 * is slightly different than OpenGL's.
		 */
		apply_quat(&camera.orientation, 0, 0, -1, &front.x, &front.y, &front.z);
		camera.position.x += front.x / 10;
		camera.position.y += front.y / 10;
		camera.position.z += front.z / 10;
	}
	if (key[KEY_DOWN]) {
		VECTOR front;
		apply_quat(&camera.orientation, 0, 0, -1, &front.x, &front.y, &front.z);
		camera.position.x -= front.x / 10;
		camera.position.y -= front.y / 10;
		camera.position.z -= front.z / 10;
	}


	/* When turning right or left, we only want to change the heading.
	 * That is, we only want to rotate around the absolute Y axis
	 */
	if (key[KEY_LEFT]) {
		get_y_rotate_quat(&q, -1);
		quat_mul(&camera.orientation, &q, &camera.orientation);
	}
	if (key[KEY_RIGHT]) {
		get_y_rotate_quat(&q, 1);
		quat_mul(&camera.orientation, &q, &camera.orientation);
	}

	/* However, when rolling or changing pitch, we do a rotation relative to
	 * the current orientation of the camera. This is why we extract the
	 * 'right' and 'front' vectors of the camera and apply a rotation on
	 * those.
	 */
	if (key[KEY_PGUP]) {
		VECTOR right;
		apply_quat(&camera.orientation, 1, 0, 0, &right.x, &right.y, &right.z);
		get_vector_rotation_quat(&q, right.x, right.y, right.z, -1);
		quat_mul(&camera.orientation, &q, &camera.orientation);
	}
	if (key[KEY_PGDN]) {
		VECTOR right;
		apply_quat(&camera.orientation, 1, 0, 0, &right.x, &right.y, &right.z);
		get_vector_rotation_quat(&q, right.x, right.y, right.z, 1);
		quat_mul(&camera.orientation, &q, &camera.orientation);
	}

	if (key[KEY_R]) {
		VECTOR front;
		apply_quat(&camera.orientation, 0, 0, 1, &front.x, &front.y, &front.z);

		if (key_shifts & KB_SHIFT_FLAG)
			get_vector_rotation_quat(&q, front.x, front.y, front.z, -1);
		else
			get_vector_rotation_quat(&q, front.x, front.y, front.z, 1);

		quat_mul(&camera.orientation, &q, &camera.orientation);
	}
	
	if (key[KEY_F]) {
		if (key_shifts & KB_SHIFT_FLAG) {
			if (fov < 96)
				fov++;
		}
		else {
			if (fov > 16)
				fov--;
		}
	}

	if (key[KEY_A]) {
		if (key_shifts & KB_SHIFT_FLAG) {
			aspect += 0.05;
			if (aspect > 2)
				aspect = 2;
		}
		else {
			aspect -= 0.05;
			if (aspect < .1)
				aspect = .1;
		}
	}
}



int main(void) {

	allegro_init();
	install_allegro_gl();
	install_keyboard();
	install_timer();

	/* Initialise the camera */
	camera.orientation = identity_quat;
	camera.position.x = 0;
	camera.position.y = 0;
	camera.position.z = 4;

	/* Set up AllegroGL */
	allegro_gl_clear_settings();
	allegro_gl_set (AGL_COLOR_DEPTH, 16);
	allegro_gl_set (AGL_Z_DEPTH, 16);
	allegro_gl_set (AGL_DOUBLEBUFFER, 1);
	allegro_gl_set (AGL_RENDERMETHOD, 1);
	allegro_gl_set (AGL_WINDOWED, TRUE);	
	allegro_gl_set (AGL_SUGGEST, AGL_Z_DEPTH | AGL_DOUBLEBUFFER
	        | AGL_RENDERMETHOD | AGL_WINDOWED | AGL_COLOR_DEPTH);
   
	if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0) != 0) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message ("Error setting OpenGL graphics mode:\n%s\n"
		                 "Allegro GL error : %s\n",
		                 allegro_error, allegro_gl_error);
		return 1;
	}

	/* Set up OpenGL */
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glShadeModel(GL_SMOOTH);

	/* Build the font we'll use to display info */
	agl_font = allegro_gl_convert_allegro_font_ex(font,
                                       AGL_FONT_TYPE_TEXTURED, -1.0, GL_ALPHA8);

	glBindTexture(GL_TEXTURE_2D, 0);

	/* Run the example program */
	while (!key[KEY_ESC]) {
		render();

		process_input();
		rest(2);
	}
	
	allegro_gl_destroy_font(agl_font);

	return 0;
}
END_OF_MAIN()

