/* Allegro-AllegroGL interface example */
#include <stdio.h>
#include <math.h>

#include "allegro.h"
#include "alleggl.h"

#include "running.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif


volatile int chrono = 0;


void the_timer(void) {
	chrono++;
} END_OF_FUNCTION(the_timer);



int main() {

	BITMAP *texture;
	GLuint tex[6];
	int i = 0;
	int frame_count = 0, frame_count_time = 0;
	float fps_rate = 0.0;
	
	FONT *f;
	int old_depth;

	/* Init Allegro */
	allegro_init();
	install_allegro_gl();
	install_keyboard();
	install_mouse();
	
	/* Set up an OpenGL screen mode */
	allegro_gl_clear_settings();
	allegro_gl_set(AGL_COLOR_DEPTH, 32);
	allegro_gl_set(AGL_Z_DEPTH, 24);
	allegro_gl_set(AGL_WINDOWED, TRUE);
	allegro_gl_set(AGL_DOUBLEBUFFER, 1);
	allegro_gl_set(AGL_RENDERMETHOD, 1);
	allegro_gl_set(AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_DOUBLEBUFFER
	             | AGL_RENDERMETHOD | AGL_Z_DEPTH | AGL_WINDOWED);
	
	if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0)) {	
		allegro_message("Error setting OpenGL graphics mode:\n%s\n"
		                "Allegro GL error : %s\n",
		                allegro_error, allegro_gl_error);
		return -1;
	}

	/* Set up a timer that ticks every 5 ms */	
	LOCK_FUNCTION(the_timer);
	LOCK_VARIABLE(chrono);
	
	install_int(the_timer, 5);
	
	/* Let's load the mysha image in various color depths.
	 */
	old_depth = bitmap_color_depth(screen);
	 
	for (i = 0; i < 3; i++) {
		char name[256];
		
		/* Load mysha */
		set_color_depth((i + 2) * 8);
		texture = load_bitmap("mysha.pcx", NULL);
				
		if (!texture) {
			set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
			allegro_message("Unable to load mysha.pcx - did you copy Allegro's here?\n");
			return -3;
		}
		
		tex[i * 2]     = allegro_gl_make_masked_texture(texture);
		tex[i * 2 + 1] = allegro_gl_make_texture(texture);
		
		/* Just to verify that the load was successful, save the images to disk
		 * so they can be checked manually.
		 */
		uszprintf(name, sizeof(name), "mysha%i.bmp", i);
		save_bitmap(name, texture, NULL);
		
		destroy_bitmap(texture);
	}
	set_color_depth(old_depth);
	
	/* Convert the Allegro font */
	f = allegro_gl_convert_allegro_font(font, AGL_FONT_TYPE_TEXTURED, 16.0);
	
	/* Now display everything */

	/* Setup OpenGL the way we want */
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

	/* Flush keyboard buffer */
	while (keypressed()) readkey();

	/* Main rendering loop */
	do {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
		
		/* Draw all quads */
		for (i = 0; i < 6; i++) {
			glLoadIdentity();
			glTranslatef(-10.0 + 4 * (i & 1), -4.0 * (i / 2 - 1), -10.0);
	
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, tex[i]);
			glBegin(GL_QUADS);
				
				glTexCoord2f(0, 0);
				glVertex2f(0, 0);
					
				glTexCoord2f(1, 0);
				glVertex2f(4, 0);
				
				glTexCoord2f(1, 1);
				glVertex2f(4, 4);
				
				glTexCoord2f(0, 1);
				glVertex2f(0, 4);
				
			glEnd();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			allegro_gl_printf(f, 0, 3.9, 0.1, makecol(255, 255, 255),
			                  "%i bpp", (i/2+2) * 8);
			glBlendFunc(GL_ONE, GL_ZERO);
		}
		
		/* Draw some lines and triangles using Allegro */
		allegro_gl_set_allegro_mode();
		
		/* Vertices of triangles should be entered carefully : OpenGL culls
		 * back faces...
		 * Therefore, according to glFrontFace(GL_CCW), vertices are given
		 * in counterclockwise order.
		 */
		triangle(screen, 470 + (int)(sin(-chrono / 100.0) * 150),
		                 250 - (int)(cos(-chrono / 100.0) * 150),
		                 470 + (int)(sin(-chrono / 100.0 + M_PI * 4 / 3) * 150),
		                 250 - (int)(cos(-chrono / 100.0 + M_PI * 4 / 3) * 150),
						 470 + (int)(sin(-chrono / 100.0 + M_PI * 2 / 3) * 150),
		                 250 - (int)(cos(-chrono / 100.0 + M_PI * 2 / 3) * 150),
		         makecol(0, 0, 255));
		triangle(screen, 470 + (int)(sin(chrono / 100.0) * 150),
		                 250 - (int)(cos(chrono / 100.0) * 150),
		                 470 + (int)(sin(chrono / 100.0 + M_PI * 4 / 3) * 150),
		                 250 - (int)(cos(chrono / 100.0 + M_PI * 4 / 3) * 150),
		                 470 + (int)(sin(chrono / 100.0 + M_PI * 2 / 3) * 150),
		                 250 - (int)(cos(chrono / 100.0 + M_PI * 2 / 3) * 150),
		         makecol(0, 255, 0));
						
		for (i = 0; i < 6; i++) {
			hline(screen, 320, 100 + (chrono % 50) + 50 * i, 620,
			      makecol(255, 0, 0));
		}
		for (i = 0; i < 6; i++) {
			vline(screen, 320 + (chrono % 50) + 50 * i, 100, 400,
			      makecol(255, 0, 0));
		}
		for (i = 0; i < 6; i++) {
			line(screen, 620 - (chrono % 50) - 50 * i, 400,
			     620, 400 - (chrono % 50) - 50 * i, makecol(255, 0, 0));
			line(screen, 320, 400 - (chrono % 50) - 50 * i,
			     620 - (chrono % 50) - 50 * i, 100, makecol(255, 0, 0));
		}
		rect(screen, 320, 100, 620, 400, makecol(255, 255, 0));
		
		/* draw mouse */
		triangle(screen,
		         mouse_x + 20 + (int)(sin(chrono / 100.0) * 15),
		         mouse_y - (int)(cos(chrono / 100.0) * 15) + 7,
		         mouse_x + 20 + (int)(sin(chrono / 100.0 + M_PI * 4 / 3) * 15),
		         mouse_y - (int)(cos(chrono / 100.0 + M_PI * 4 / 3) * 15) + 7,
		         mouse_x + 20 + (int)(sin(chrono / 100.0 + M_PI * 2 / 3) * 15),
		         mouse_y - (int)(cos(chrono / 100.0 + M_PI * 2 / 3) * 15) + 7,
		         makecol(255, 255, 0));
		triangle(screen, mouse_x, mouse_y, mouse_x + 25, mouse_y + 15,
		         mouse_x + 35, mouse_y,	makecol(255, 0, 0));

		allegro_gl_unset_allegro_mode();

		/* Compute and display the framerate */
		frame_count++;
		
		if (frame_count >= 20) {
			if (chrono > frame_count_time) {
				fps_rate = frame_count * 200.0 / (chrono - frame_count_time);
			}
			else {
				fps_rate = 200;
			}
			frame_count -= 20;
			frame_count_time = chrono;
		}
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		allegro_gl_printf(f, 0, 0, 0, makecol(255, 255, 255),
		                  "FPS: %.2f", fps_rate);
		glBlendFunc(GL_ONE, GL_ZERO);
				
		/* Resets the current color to its initial value
		 * That must be done in order to reset alpha to 255
		 */
		glColor4ub(255, 255, 255, 255);
		
		allegro_gl_flip();

		rest(2);

	} while (!keypressed());

	destroy_font(f);

	return 0;
}
END_OF_MAIN()

