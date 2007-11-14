/* exmipmaps.c
 *
 * This example shows how to build and use a mipmap stack in AllegroGL
 * for trilinear filtering. If your GPU also supports setting the maximum
 * degree of anisotropy for filtering, then that is also used.
 */

#include "allegro.h"
#include "alleggl.h"


/* Define a struct to hold our texture information */
typedef struct {
	BITMAP *bitmap;
	GLuint id;
} texture_t;




static void run(texture_t *texture, FONT *agl_font);



int main() {

	int flags;
	texture_t texture;
	FONT *agl_font;

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
	
	if (set_gfx_mode(GFX_OPENGL, 800, 600, 0, 0)) {	
		allegro_message("Error setting OpenGL graphics mode:\n%s\n"
		                "Allegro GL error : %s\n",
		                allegro_error, allegro_gl_error);
		return -1;
	}

	/* Create a checkered bitmap */
	{	int i, j;
		int col[2];
		
		col[0] = makecol(0, 0, 0);
		col[1] = makecol(255, 255, 255);

#define TW 64
#define TH 64
		
		texture.bitmap = create_bitmap(TW, TH);

#define PW 8
#define PH 8
		
		for (j = 0; j < PH; j++) {
			for (i = 0; i < PW; i++) {
				rectfill(texture.bitmap, i * TW / PW, j * TH / PH,
						 (i + 1) * TW / PW, (j + 1) * TH / PH,
						 col[(i ^ j) & 1]);
			}
		}

#undef TW
#undef TH
#undef PW
#undef PH
	}
	
	/* Create a texture from the bitmap and generate mipmaps */
	flags  = AGL_TEXTURE_MIPMAP | AGL_TEXTURE_RESCALE;
	texture.id = allegro_gl_make_texture_ex(flags, texture.bitmap, GL_RGBA8);

	/* Get an AGL font by converting Allegro's */
	agl_font = allegro_gl_convert_allegro_font_ex(font,
	                              AGL_FONT_TYPE_TEXTURED, -1.0, GL_INTENSITY8);
	
	run(&texture, agl_font);

	return 0;
}
END_OF_MAIN()



static volatile int the_time = 0;
#define TIME_SCALE 500


void the_timer(void) {
	the_time++;
	if (the_time > TIME_SCALE) {
		the_time -= TIME_SCALE;
	}
}
END_OF_FUNCTION(the_timer);



void run(texture_t *texture, FONT *agl_font) {

	static const char *text[] = { "Trilinear w/ Aniso",  "Trilinear",
		                          "Bilinear w/ Aniso",   "Bilinear"};
	static const GLenum min_filter[] = { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };
	static const GLfloat max_aniso[] = { 16, 1 };

	int i, j;
	
	int has_aniso = allegro_gl_extensions_GL.EXT_texture_filter_anisotropic;
	
	/* Draw an oblique GL_QUAD in 4 modes:
	 *     Bilinear + Aniso  |     Bilinear
	 *    -----------------------------------------
	 *    Trilinear + Aniso  |    Trilinear
	 */
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_SCISSOR_TEST);

	glMatrixMode(GL_PROJECTION);
	glFrustum(-1, 1, -1, 1, 1, 10);
	glMatrixMode(GL_MODELVIEW);

	install_int(the_timer, 20);
	
	do {
		float t = the_time / (float)TIME_SCALE;

		glClear(GL_COLOR_BUFFER_BIT);

		if (!has_aniso) {
			static const char *text = "EXT_texture_filter_anisotropic not "
				                      "supported!";
			int len = text_length(font, text);

			glViewport(0, 0, SCREEN_W, SCREEN_H);
			
			allegro_gl_set_projection();
			glTranslatef(-0.375, -0.375, 0);
			glTranslatef(5, 5, 0);
			
			for (j = 0; j < 2; j++) {
				rectfill(screen, 0,   j * SCREEN_H/2,
						         len, j * SCREEN_H/2 + 8,
								 makecol(0,0,0));

				allegro_gl_printf(agl_font, 0, j * SCREEN_H/2, 0,
				                  makecol(255, 255, 255),
				                  "%s", text);
			}
			glTranslatef(+0.375, +0.375, 0);
			allegro_gl_unset_projection();
		}
		
		for (j = 0; j < 2; j++) {
			for (i = has_aniso ? 0 : 1; i < 2; i++) {
				int len = text_length(font, text[i + j * 2]);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						        min_filter[j]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
						        max_aniso[i]);

				glViewport(i*SCREEN_W/2, j*SCREEN_H/2, SCREEN_W/2, SCREEN_H/2);
				
				glBegin(GL_TRIANGLES);
					glTexCoord2f(t,    t);  glVertex3f(-1,  1, -1);
					glTexCoord2f(10+t, t);  glVertex3f( 0,  0, -10);
					glTexCoord2f(t,  1+t);  glVertex3f(-1, -1, -1);

					glTexCoord2f(-t,   -t);  glVertex3f(-1,  1, -1);
					glTexCoord2f(10-t, -t);  glVertex3f( 0,  0, -10);
					glTexCoord2f(-t,  1-t);  glVertex3f( 1,  1, -1);

					glTexCoord2f(-t,   -t);  glVertex3f( 1,  1, -1);
					glTexCoord2f(10-t, -t);  glVertex3f( 0,  0, -10);
					glTexCoord2f(-t,  1-t);  glVertex3f( 1, -1, -1);

					glTexCoord2f(-t,   -t);  glVertex3f( 1, -1, -1);
					glTexCoord2f(10-t, -t);  glVertex3f( 0,  0, -10);
					glTexCoord2f(-t,  1-t);  glVertex3f(-1, -1, -1);
				glEnd();

				allegro_gl_set_projection();
				glTranslatef(-0.375, -0.375, 0);
				glTranslatef(5, 5, 0);

				rectfill(screen, i * SCREEN_W/2,       j * SCREEN_H/2,
						         i * SCREEN_W/2 + len, j * SCREEN_H/2 + 8,
								 makecol(0,0,0));

				allegro_gl_printf(agl_font, i * SCREEN_W/2, j * SCREEN_H/2, 0,
				                  makecol(255, 255, 255),
				                  "%s", text[i + j * 2]);

				glTranslatef(+0.375, +0.375, 0);
				allegro_gl_unset_projection();
				
			}
		}
		allegro_gl_flip();
		rest(2);
	} while (!keypressed());
}

