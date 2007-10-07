/* This is an example of how to use the Allegro GUI routines along with AllegroGL
   It demonstrates how to proceed to :
   - Use the OpenGL double buffer along with the GUI
   - Use the regular Allegro GUI routines "as is"
   - Create a GUI viewport which draws some GL stuff (see 'glviewport_proc()')
 */

#include <alleggl.h>
#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <string.h>
#include <math.h>

#define VIEW_ASPECT 1.


void display(void);
int my_radio_proc(int msg, DIALOG *d, int c);


/* information needed to display the mesh */
struct {
	int polygon_mode;	/* rendering mode of polygons */
	float theta, phi;       /* orientation of object */
	GLuint tex;		/* texture */
} display_info;


/* No need of a 'd_clear_proc' since algl_do_dialog will clear the screen
 * and Z buffer for us.  However note that things like centre_dialog will
 * benefit from having a clear proc at the start, and for non-fullscreen
 * dialogs perhaps you'd want the auto-clear to clear to black, then draw 
 * your small dialog in white with a d_clear_proc.  It's up to you. */
DIALOG my_dialog[] =
{
   /* (dialog proc)		(x)	(y)	(w)	(h)	(fg)	(bg)	(key)	(flags)		(d1)	(d2)	(dp)		(dp2)	(dp3) */
   { d_algl_viewport_proc,	10,	10,	460,	460,	0,	0,	0,	0,		0,	0,	NULL,		NULL,	NULL },
   { my_radio_proc,		500,	10,	120,	20,	0,	0,	0,	0,		1,	0,	"Points",	NULL,	NULL },
   { my_radio_proc,		500,	40,	120,	20,	0,	0,	0,	0,		1,	0,	"Lines",	NULL,	NULL },
   { my_radio_proc,		500,	70,	120,	20,	0,	0,	0,	D_SELECTED,	1,	0,	"Polygons",	NULL,	NULL },
   { d_button_proc,		500,	450,	120,	20,	0,	0,	0,	D_EXIT,		0,	0,	"Exit",		NULL,	NULL },
   { NULL,			0,	0,	0,	0,	0,	0,	0,	0,		0,	0,	NULL,		NULL,	NULL }
};



/* This callback function demonstrates how to use the d_algl_viewport_proc 
 * object.
 */
int glviewport_callback(BITMAP* viewport, int msg, int c)
{
	static int focus = TRUE;
	static float prevx = 0, prevy = 0;	/* position of mouse */
	static float zoom = 30; 		/* field of view in degrees */
	int ret = 0;
	
	/* Determine if the mouse is on the object */
	if (msg == MSG_GOTMOUSE) {
		focus = TRUE;
	}

	if (msg == MSG_LOSTMOUSE) {
		focus = FALSE;
	}

	if (msg == MSG_IDLE) {
		rest(2);
		ret = D_O_K;
	}

	if (msg == MSG_DRAW) {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		
		gluPerspective(zoom, VIEW_ASPECT, 1,100);
		glMatrixMode(GL_MODELVIEW);
		glTranslatef(0,0,-30);

		/* Apply the rotations */
		glRotatef(display_info.phi, 1., 0., 0.);
		glRotatef(display_info.theta, 0., 1., 0.);

		/* Display the 3D object */
		glPolygonMode(GL_FRONT_AND_BACK, display_info.polygon_mode);
		display();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
		/* Display of 2D objects is also possible */
		allegro_gl_set_allegro_mode();
		rect(viewport, 5, 5, viewport->w - 5, viewport->h - 5,
		     focus ? makecol(255,0,0) : makecol(255, 255, 255));
		textprintf_ex(viewport, font, 10, 445, makecol(255, 255, 255), -1,
		           "Click & drag");
		allegro_gl_unset_allegro_mode();

		ret = D_O_K;
	}


	/* drag in progress, simulate trackball */
	if ((mouse_b & 1) && focus) {
		float x,y;
		
		x = mouse_x;
		y = mouse_y;

		display_info.theta += x - prevx;
		display_info.phi += y - prevy;
		
		ret = D_O_K;
	}

	/* zooming drag */
	if ((mouse_b & 2) && focus) {
		zoom += ((mouse_y - prevy) / viewport->h) * 40;
		if (zoom < 5)
			zoom = 5;
		if (zoom > 120)
			zoom = 120;

		ret = D_O_K;
	}

	prevx = mouse_x;
	prevy = mouse_y;

	return ret;
}



/* This function is an ordinary GUI proc (no GL calls)
   It manages the 3 radio buttons and determine which rendering type
   has been selected */
int my_radio_proc(int msg, DIALOG *d, int c)
{
	int ret, i;

	ret = d_radio_proc(msg, d, c);
	
	/* Determine which one is selected... */
	for (i = 1; i < 3; i++) {
		if (my_dialog[i].flags & D_SELECTED) {
			break;
		}
	}
			
	/* ...and change the mode accordingly */
	switch(i) {
		case 1:
			display_info.polygon_mode = GL_POINT;
			break;
		case 2:
			display_info.polygon_mode = GL_LINE;
			break;
		case 3:
		default:
			display_info.polygon_mode = GL_FILL;
	}
	
	return ret;
}



/* Displays the 3D object */
void display (void)
{
	/* Translate and rotate the object */
	glTranslatef(-2.5, 0.0, 0.0);
	glRotatef(-30, 1.0, 0.0, 0.0);
	glRotatef(30, 0.0, 1.0, 0.0);
	glRotatef(30, 0.0, 0.0, 1.0);

	glColor3f(1.0, 0.0, 1.0);

	/* Draw the sides of the three-sided pyramid */
	glBindTexture (GL_TEXTURE_2D, display_info.tex);
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f (0, 0); glVertex3d(0, 4, 0);
		glTexCoord2f (1, 0); glVertex3d(0, -4, -4);
		glTexCoord2f (1, 1); glVertex3d(-4, -4, 4);
		glTexCoord2f (0, 1); glVertex3d(4, -4, 4);
		glTexCoord2f (1, 0); glVertex3d(0, -4, -4);
	glEnd();

	glColor3f(0.0, 1.0, 1.0);

	/* Draw the base of the pyramid */
	glBegin(GL_TRIANGLES);
		glTexCoord2f (1, 0); glVertex3d(0, -4, -4);
		glTexCoord2f (0, 1); glVertex3d(4, -4, 4);
		glTexCoord2f (1, 1); glVertex3d(-4, -4, 4);
	glEnd();


	glTranslatef(2.5, 0.0, 0.0);
	glRotatef(45, 1.0, 0.0, 0.0);
	glRotatef(45, 0.0, 1.0, 0.0);
	glRotatef(45, 0.0, 0.0, 1.0);

	glColor3f(0.0, 1.0, 0.0);

	glBindTexture (GL_TEXTURE_2D, 0);

	/* Draw the sides of the cube */
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

	/* Draw the top of the cube */
	glBegin(GL_QUADS);
		glVertex3d(-3, -3, -3);
		glVertex3d(3, -3, -3);
		glVertex3d(3, -3, 3);
		glVertex3d(-3, -3, 3);
	glEnd();

	/* Bottom is texture-mapped */
	glBindTexture (GL_TEXTURE_2D, display_info.tex);
	glBegin (GL_QUADS);
		glTexCoord2f (0, 0); glVertex3d(-3, 3, -3);
		glTexCoord2f (1, 0); glVertex3d(-3, 3, 3);
		glTexCoord2f (1, 1); glVertex3d(3, 3, 3);
		glTexCoord2f (0, 1); glVertex3d(3, 3, -3);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}



/* Load the texture */
void setup_texture (void)
{
	PALETTE pal;
	BITMAP *bmp, *bmp2;
	int w = 128, h = 128;

	bmp = load_bitmap ("mysha.pcx", pal);
	if (!bmp) {
		allegro_message ("Error loading `mysha.pcx'");
		exit (1);
	}
	bmp2 = create_bitmap (w, h);
	stretch_blit (bmp, bmp2, 0, 0, bmp->w, bmp->h, 0, 0, w, h);
	destroy_bitmap (bmp);

	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

	display_info.tex = allegro_gl_make_texture (bmp2);
	destroy_bitmap (bmp2);
}



int main (void)
{
	allegro_init();
	install_allegro_gl();

	allegro_gl_clear_settings();
	allegro_gl_set (AGL_Z_DEPTH, 16);
	allegro_gl_set (AGL_COLOR_DEPTH, 16);
	allegro_gl_set (AGL_DOUBLEBUFFER, 1);
	allegro_gl_set (AGL_WINDOWED, TRUE);	
	allegro_gl_set (AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_Z_DEPTH
	              | AGL_DOUBLEBUFFER | AGL_WINDOWED);

	if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0) < 0) {
		allegro_message ("Error setting OpenGL graphics mode:\n%s\n"
		                 "Allegro GL error : %s\n",
		                 allegro_error, allegro_gl_error);
		return -1;
	}
	
	install_keyboard();
	install_mouse();
	
	/* Set up OpenGL */
	/* remove back faces */
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
  
	/* speedups */
	glShadeModel(GL_FLAT);

	glPointSize(2.);
	glEnable(GL_TEXTURE_2D);
	setup_texture();

	/* Set colours of dialog components */
	set_dialog_color (my_dialog, makecol(0, 0, 0), makecol(255, 255, 255));
	
	/* Set up the callback function for d_algl_viewport_proc */
	my_dialog[0].dp = glviewport_callback;
	my_dialog[0].bg = makecol(77, 102, 153);

	/* Set colour for automatic dialog clearing */
	glClearColor(1.,1.,1.,1.);

	/* Same as Allegro's do_dialog, but works with OpenGL -- the only
	 * caveat is that the whole screen is cleared and the dialog redrawn
	 * once per update (i.e. pretty much all the time).
	 */
	algl_do_dialog (my_dialog, -1);

	return 0;
}
END_OF_MAIN()

