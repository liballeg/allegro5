
#include <time.h>
#include <math.h>
#include <string.h>

#include "allegro.h"
#include "alleggl.h"


#ifndef M_PI
#define M_PI 3.1415
#endif

#define NUM_STRINGS 3
#define STRING "Hello World!"

#define STRING0 ("AllegroGL  v" AGL_VERSION_STR)
#define STRING1 "Text Drawing"
#define STRING2 "And Font Loading"
#define STRING3 "Example Program"
#define STRING4 "See Our Webpage At:"


#define URL "http://allegrogl.sourceforge.net/"

volatile int chrono = 0;
volatile double angle = 0.0;



void the_timer(void) {
	chrono++;
	angle += 1.0 / 400.0;

	if (angle > M_PI)
		angle -= 2 * M_PI;


} END_OF_FUNCTION(the_timer);



int main() {

	FONT *lucidia_fnt;
	FONT *allegro_fnt;
	DATAFILE *dat;
	
	struct {
		float x, y, z;
		int c;
	} coord[NUM_STRINGS];
	char *string = STRING;
	int i;

	allegro_init();
	install_allegro_gl();
	install_timer();
	
	LOCK_FUNCTION(the_timer);
	LOCK_VARIABLE(chrono);
	
	install_int(the_timer, 2);

	allegro_gl_clear_settings();
	allegro_gl_set(AGL_COLOR_DEPTH, 32);
	allegro_gl_set(AGL_Z_DEPTH, 24);
	allegro_gl_set(AGL_WINDOWED, TRUE);
	allegro_gl_set(AGL_DOUBLEBUFFER, 1);
	allegro_gl_set(AGL_RENDERMETHOD, 1);
	allegro_gl_set(AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_DOUBLEBUFFER
	             | AGL_RENDERMETHOD | AGL_Z_DEPTH | AGL_WINDOWED);

	if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0)) {
		allegro_message("Error initializing OpenGL!\n");
		return -1;
	}

	install_keyboard();


	/* Load 2 copies of the datafile */	
	dat = load_datafile("lucidia.dat");

	if (!dat) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Unable to load lucidia.dat\n");
		return -2;
	}

	lucidia_fnt = allegro_gl_convert_allegro_font((FONT*)dat[0].dat,
	                                              AGL_FONT_TYPE_TEXTURED, 16.0);

	if (!lucidia_fnt) {
		unload_datafile(dat);
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Unable to convert font!\n");
		return -3;
	}

	allegro_fnt = allegro_gl_convert_allegro_font(font,
	                                              AGL_FONT_TYPE_TEXTURED, 16.0);

	if (!allegro_fnt) {
		allegro_gl_destroy_font(lucidia_fnt);
		unload_datafile(dat);
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Unable to convert font!\n");
		return -4;
	}

	
	srand(time(NULL));


	for (i = 0; i < NUM_STRINGS; i++) {
		coord[i].z = 0;
		coord[i].c = 0;
	}
	i = 0;

	/* Start nice text demo */

	/* Setup OpenGL like we want */
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glViewport(0, 0, SCREEN_W, SCREEN_H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 1, 60.0);

	/* Set culling mode - not that we have anything to cull */
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
			
	glMatrixMode(GL_MODELVIEW);

	do {
		int c;

		/* Update rotation angle of text */	
		glLoadIdentity();
		
		glTranslatef(0, 0, -30);
		glRotatef(-45, 1, 0, 0);
		glRotatef(-30, 0, 1, 0);

		/* Clear the screen and set a nice blender mode */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);						
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

		/* Print the text wheel */
		for (i = 0; i < NUM_STRINGS; i++) {			
			allegro_gl_printf(lucidia_fnt, coord[i].x, coord[i].y, coord[i].z,
			                  coord[i].c, string);
		}
		/* Update the text wheel's position */
		for (i = 0; i < NUM_STRINGS; i++) {
			int r, g, b;
			float angle2 = angle + i * M_PI * 2 / NUM_STRINGS;
			coord[i].x = 10 * cos(angle2) - 10;
			coord[i].y = 10 * sin(angle2);			
			angle2 = angle2 * 180.0 / M_PI;
#if ((((ALLEGRO_VERSION) << 16) | ((ALLEGRO_SUB_VERSION) << 8) | (ALLEGRO_WIP_VERSION)) == 0x40200)
			/* Work around an Allegro bug that's only in 4.2.0 */
			while (angle2 > 360.0f) angle2 -= 360.0f;
			while (angle2 < 0)      angle2 += 360.0f;
#endif			
			hsv_to_rgb(angle2, 1, 1, &r, &g, &b);
			coord[i].c = makeacol(r, g, b, 255);
		}
		/* Draw "AllegroGL" */
		glLoadIdentity();
		c = MID(0, (int)(255 - chrono / 100), 255);
		allegro_gl_printf(allegro_fnt, -(strlen(STRING0) * 1 / 2.0f) / 2,
		                -3.0f, -chrono / 200.0 + 0, makecol(c, c, c), STRING0);
		c = MID(0, (int)(255 - chrono / 100) + 24, 255);
		allegro_gl_printf(allegro_fnt, -(strlen(STRING1) * 1 / 2.0f) / 2,
		                -3.0f, -chrono / 200.0 + 12, makecol(c, c, c), STRING1);
		c = MID(0, (int)(255 - chrono / 100) + 48, 255);
		allegro_gl_printf(allegro_fnt, -(strlen(STRING2) * 1 / 2.0f) / 2,
		                -3.0f, -chrono / 200.0 + 24, makecol(c, c, c), STRING2);
		c = MID(0, (int)(255 - chrono / 100) + 72, 255);
		allegro_gl_printf(allegro_fnt, -(strlen(STRING3) * 1 / 2.0f) / 2,
		                -3.0f, -chrono / 200.0 + 36, makecol(c, c, c), STRING3);
		c = MID(0, (int)(255 - chrono / 100) + 96, 255);
		allegro_gl_printf(allegro_fnt, -(strlen(STRING4) * 1 / 2.0f) / 2,
		                -3.0f, -chrono / 200.0 + 48, makecol(c, c, c), STRING4);
		c = MID(0, (int)(255 - chrono / 100) + 120, 255);
		allegro_gl_printf(allegro_fnt, -(strlen(URL) * 1 / 2.0f) / 2,
		                -3.0f, -chrono / 200.0 + 60, makecol(c, c, c), URL);

		allegro_gl_flip();

		rest(2);

	} while (!key[KEY_ESC]);

	return 0;
}
END_OF_MAIN()

