/* Example of AllegroGL's masked texture routines */

#include "allegro.h"
#include "alleggl.h"

#include "running.h"

volatile int chrono = 0;



void the_timer(void) {
	chrono++;
} END_OF_FUNCTION(the_timer);



int render_time;



int main() {

	BITMAP *texture;
	DATAFILE *dat;
	GLuint tex[11];
	int i = 0, j;

	allegro_init();
	install_allegro_gl();
	install_keyboard();
	install_mouse();
	
	allegro_gl_clear_settings();
	allegro_gl_set(AGL_COLOR_DEPTH, 32);
	allegro_gl_set(AGL_DOUBLEBUFFER, 1);
	allegro_gl_set(AGL_Z_DEPTH, 24);
	allegro_gl_set(AGL_WINDOWED, TRUE);
	allegro_gl_set(AGL_RENDERMETHOD, 1);
	allegro_gl_set(AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_DOUBLEBUFFER
	       | AGL_RENDERMETHOD | AGL_Z_DEPTH | AGL_WINDOWED);
	
	if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0)) {	
		allegro_message ("Error setting OpenGL graphics mode:\n%s\n"
		                 "Allegro GL error : %s\n",
		                 allegro_error, allegro_gl_error);
		return -1;
	}
	
	install_timer();
	
	LOCK_FUNCTION(the_timer);
	LOCK_VARIABLE(chrono);
	
	install_int(the_timer, 10);
	
	dat = load_datafile("running.dat");

	if (!dat) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Unable to load running.dat!\n");
		return -2;
	}

	allegro_gl_use_mipmapping(TRUE);

	/* Convert running ball thingie */
	for (j = 0; j < 10; j++) {
		BITMAP *bmp = dat[FRAME_00_BMP + j].dat;
		tex[j] = allegro_gl_make_masked_texture(bmp);
	}
	/* Convert Mysha */
	texture = load_bitmap("mysha.pcx", NULL);
	if (!texture) {
		unload_datafile(dat);
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Unable to load mysha.pcx - did you copy Allegro's "
		                "here?\n");
		return -3;
	}
	tex[10] = allegro_gl_make_texture(texture);
	
	/* Now display everything */

	/* Setup OpenGL like we want */
	glEnable(GL_TEXTURE_2D);

	/* Skip pixels which alpha channel is lower than 0.5*/
	glAlphaFunc(GL_GREATER, 0.5);

	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT, GL_FILL);

	glViewport(0, 0, SCREEN_W, SCREEN_H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 1, 60.0);

	/* Set culling mode - not that we have anything to cull */
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
			
	glMatrixMode(GL_MODELVIEW);
		

	while (keypressed()) readkey();

	render_time = chrono;
	do {
		int screen_w = SCREEN_W;
		int screen_h = SCREEN_H;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
		glTranslatef(-10.0, -10.0, -10.0);
		
		/* Draw BG */
		glBindTexture(GL_TEXTURE_2D, tex[10]);				
		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(0, 0);
			
			glTexCoord2f(1, 0);
			glVertex2f(20, 0);
			
			glTexCoord2f(1, 1);
			glVertex2f(20, 20);
			
			glTexCoord2f(0, 1);
			glVertex2f(0, 20);
		glEnd();
		
		i = (chrono / 10) % 10;

		/* Draw running ball thingie */
		
		/* Enable removing of thingie's transparent pixels */
		glEnable(GL_ALPHA_TEST);
		glLoadIdentity();
		if (screen_w != 0 && screen_h != 0) {
			glTranslatef(mouse_x * 20.0 / screen_w - 10.0,
			             mouse_y * -20.0 / screen_h + 10.0, -9.99);
		}
		glRotatef(chrono, 0, 0, 1.0);
		glBindTexture(GL_TEXTURE_2D, tex[i]);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(-1.0, -1.0);
			
			glTexCoord2f(1, 0);
			glVertex2f(1.0, -1.0);
			
			glTexCoord2f(1, 1);
			glVertex2f(1.0, 1.0);
			
			glTexCoord2f(0, 1);
			glVertex2f(-1.0, 1.0);
		glEnd();
		glDisable(GL_ALPHA_TEST); /* Disable removing of transparent pixels */

		allegro_gl_flip();

		render_time++;
		while (!keypressed() && render_time > chrono) {
			rest(2);
		}
	} while (!keypressed());

	return 0;
}
END_OF_MAIN()

