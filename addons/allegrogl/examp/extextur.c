/* Example of AllegroGL's texture routines */

#include "allegro.h"
#include "alleggl.h"


int main() {

	BITMAP *mysha, *texture;
	GLuint tex[18];
	FONT *agl_font;
	int i, j;

	allegro_init();
	install_allegro_gl();
	install_keyboard();
	install_timer();
	
	allegro_gl_clear_settings();
	allegro_gl_set(AGL_COLOR_DEPTH, 32);
	allegro_gl_set(AGL_DOUBLEBUFFER, 1);
	allegro_gl_set(AGL_Z_DEPTH, 24);
	allegro_gl_set(AGL_WINDOWED, TRUE);
	allegro_gl_set(AGL_RENDERMETHOD, 1);
	allegro_gl_set(AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_DOUBLEBUFFER
	                          | AGL_RENDERMETHOD | AGL_Z_DEPTH | AGL_WINDOWED);
	
	TRACE("Swicthing screen modes...\n");
	
	if (set_gfx_mode(GFX_OPENGL, 800, 600, 0, 0)) {	
		allegro_message("Error setting OpenGL graphics mode:\n%s\n"
		                "Allegro GL error : %s\n",
		                allegro_error, allegro_gl_error);
		return -1;
	}
	
	TRACE("Loading texture\n");
	
	/* Load mysha.pcx */
	for (j = 0; j < 3; j++) {
		int flag;
	
		set_color_depth(8 * (j+2));	
		mysha = load_bitmap("mysha.pcx", NULL);
	
		if (!mysha) {
			set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
			allegro_message("Unable to load mysha.pcx - please copy Allegro's "
			                "in this directory.\n");
			return -2;
		}

		texture = create_sub_bitmap(mysha, 64, 64, 64, 64);
	
		TRACE("Converting textures\n");
	
		/* Convert to texture using different modes */
		flag = AGL_TEXTURE_FLIP;		
		tex[j*6 + 0] = allegro_gl_make_texture_ex(flag, texture, -1);
		tex[j*6 + 1] = allegro_gl_make_texture_ex(flag, texture, GL_RGB8);
		tex[j*6 + 2] = allegro_gl_make_texture_ex(flag, texture, GL_RGBA8);
		
		flag |= AGL_TEXTURE_MIPMAP;
		tex[j*6 + 3] = allegro_gl_make_texture_ex(flag, texture, -1);
		tex[j*6 + 4] = allegro_gl_make_texture_ex(flag, texture, GL_RGB8);
		tex[j*6 + 5] = allegro_gl_make_texture_ex(flag, texture, GL_RGBA8);
		
		destroy_bitmap(texture);
		destroy_bitmap(mysha);
	}
	TRACE("Converted all textures\n");

	/* Convert Allegro font */
	agl_font = allegro_gl_convert_allegro_font_ex(font,
	                              AGL_FONT_TYPE_TEXTURED, -1.0, GL_INTENSITY8);
	
	TRACE("Converted font\n");
	
	/* Now display everything */

	/* Setup OpenGL like we want */
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	TRACE("Setup OpenGL\n");

	do {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		/* Draw all quads */
		for (j = 0; j < 3; j++) {
	
			allegro_gl_set_projection();
			
			for (i = 0; i < 6; i++) {
				float x, y, w, h;
				
				glBindTexture(GL_TEXTURE_2D, tex[j * 6 + i]);
				glBegin(GL_QUADS);
				
					x = i * SCREEN_W / 6.0;
					y = j * SCREEN_H / 6.0;
					w = SCREEN_W / 6.0;
					h = w;
					
					glTexCoord2f(0, 1);
					glVertex2f(x, y);
					
					glTexCoord2f(1, 1);
					glVertex2f(x + w - 1, y);
				
					glTexCoord2f(1, 0);
					glVertex2f(x + w - 1, y + h - 1);
				
					glTexCoord2f(0, 0);
					glVertex2f(x, y + h - 1);
					
					x = i * SCREEN_W / 6.0;
					y = j * SCREEN_H / 6.0;
					w = SCREEN_W / 6.0 / 2;
					h = w;
				
					glTexCoord2f(0, 1);
					glVertex2f(x, y);
					
					glTexCoord2f(1, 1);
					glVertex2f(x + w - 1, y);
				
					glTexCoord2f(1, 0);
					glVertex2f(x + w - 1, y + h - 1);
				
					glTexCoord2f(0, 0);
					glVertex2f(x, y + h - 1);
				glEnd();
			}

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glTranslatef(-0.375, -0.375, 0);

			for (i = 0; i < 6; i++) {
				const char *text = (i % 3 == 0 ? "default" : i % 3 == 1
				     ? "RGB8" : "RGBA8");

				allegro_gl_printf(agl_font, i * SCREEN_W/6,
				                  j * SCREEN_H/6, 0,
				         makecol(255, 255, 255), "%ibpp %s", (j+2) * 8, text);
				allegro_gl_printf(agl_font, i * SCREEN_W/6,
				                  j * SCREEN_H/6 + 8, 0,
				         makecol(255, 255, 255), "%s", i < 3 ? "" : "mipmaped");
			}
			allegro_gl_unset_projection();
			glBlendFunc(GL_ONE, GL_ZERO);		
		}

		glViewport(0, 0, SCREEN_W, SCREEN_H);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-1.0, 1.0, -1.0, 1.0, 1, 1000.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		/* Draw a large mysha */
		glBindTexture(GL_TEXTURE_2D, tex[0]);
		glTranslatef(-10.0, -10.0, -10.0);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(0, 0);
					
			glTexCoord2f(1, 0);
			glVertex2f(20, 0);
				
			glTexCoord2f(1, 1);
			glVertex2f(20, 8);
				
			glTexCoord2f(0, 1);
			glVertex2f(0, 8);
		glEnd();

		allegro_gl_flip();
		rest(2);

	} while (!keypressed());

	return 0;
}
END_OF_MAIN()

