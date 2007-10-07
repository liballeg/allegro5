/* font test -- based on extext */

/* Specifically, this tests alpha fonts blending into backgrounds compared
 * to solid text with transparent background.
 */


#include <time.h>
#include <math.h>
#include <string.h>

#include "allegro.h"
#include "alleggl.h"


int main() {

	FONT *demofont;
	DATAFILE *dat;
	
	allegro_init();
	install_allegro_gl();

	allegro_gl_clear_settings();
	allegro_gl_set(AGL_COLOR_DEPTH, 32);
	allegro_gl_set(AGL_WINDOWED, TRUE);
	allegro_gl_set(AGL_DOUBLEBUFFER, 1);
	allegro_gl_set(AGL_SUGGEST, AGL_DOUBLEBUFFER | AGL_COLOR_DEPTH
	             | AGL_WINDOWED);

	if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0)) {
		allegro_message("Error initializing OpenGL!\n");
		return -1;
	}

	install_keyboard();
	install_timer();

	/* Load font palette first, then font itself */
	dat = load_datafile_object ("demofont.dat", "PAL");
	if (!dat) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Error loading demofont.dat#PAL\n");
		return -1;
	}
	set_palette ((RGB *)dat->dat);
	unload_datafile_object (dat);

	dat = load_datafile_object ("demofont.dat", "FONT");
	if (!dat) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Error loading demofont.dat#FONT\n");
		return -1;
	}
	demofont = allegro_gl_convert_allegro_font_ex((FONT*)dat->dat,
	                                   AGL_FONT_TYPE_TEXTURED, 16.0, GL_ALPHA8);
	unload_datafile_object(dat);

	if (!demofont) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Unable to convert font!\n");
		return -1;
	}


	/* Setup OpenGL the way we want */
	glEnable(GL_TEXTURE_2D);

	glViewport(0, 0, SCREEN_W, SCREEN_H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	glMatrixMode(GL_MODELVIEW);

	do {
		int x, y;
		glLoadIdentity();
		
		/* Draw the background */
		for (y = 0; y < 4; y++) {
			for (x = 0; x < 4; x++) {
				glColor3f ((y&1), (x&1), ((y&2)^(x&2))/2);
				glBegin (GL_QUADS);
					glVertex2f (x*0.25, y*0.25);
					glVertex2f (x*0.25, (y+1)*0.25);
					glVertex2f ((x+1)*0.25, (y+1)*0.25);
					glVertex2f ((x+1)*0.25, y*0.25);
				glEnd();
			}
		}

		/* Draw the text on top */
		glScalef (0.1, 0.1, 0.1);

		/* First, draw the text alpha blended */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		allegro_gl_printf(demofont, 1, 8, 0, makeacol(255,255,255,255),
		                 "Hello World!");
		glBlendFunc(GL_ONE, GL_ZERO);
		glDisable(GL_BLEND);

		/* Now just draw it solid with transparent background */
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
		allegro_gl_printf(demofont, 1, 5, 0, makeacol(255,255,255,255),
		                 "Hello World!");
		glDisable(GL_ALPHA_TEST);

		allegro_gl_flip();
		rest(2);

	} while (!keypressed());
	
	/* Clean up and exit */
	allegro_gl_destroy_font(demofont);
	TRACE("Font Destroyed");

	return 0;
}
END_OF_MAIN()

