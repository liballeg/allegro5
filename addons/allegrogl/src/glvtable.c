/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file glvtable.c
  * \brief Allegro->OpenGL conversion vtable.
  */

#include <string.h>

#include <allegro.h>

#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#include "alleggl.h"
#include "allglint.h"
#include "glvtable.h"
#include <allegro/internal/aintern.h>
#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif


static GFX_VTABLE allegro_gl_screen_vtable;
static GLuint __allegro_gl_pool_texture = 0;

static GLuint __allegro_gl_dummy_texture = 0; /* For ATI Rage Pro */

static int __agl_owning_drawing_pattern_tex = FALSE;
GLuint __agl_drawing_pattern_tex = 0;
BITMAP *__agl_drawing_pattern_bmp = 0;
static int __agl_drawing_mode = DRAW_MODE_SOLID;


/** \defgroup glvtable Allegro Graphics Driver
 *  These functions are used to implement Allegro function over an
 *  OpenGL screen.
 *
 *  Keep in mind that some restictions mentioned in Allegro docs about video
 *  bitmaps and the screen do not apply when using AllegroGL GFX driver. Namely:
 *  - Reading from video bitmaps is just as fast as reading from memory bitmaps.
 *  - Doing translucent/transparent drawing on the screen can be very fast if
 *    the source is a video bitmap.
 *  - Drawing from video bitmap to a video bitmap will be pretty fast on modern
 *    GFX cards with appropiate drivers installed.
 *  - There are dozen of places in the Allegro docs where it says that a memory
 *    bitmap must be used (drawing_mode(), polygon3d(), stretch_sprite() etc.).
 *    This doesn't apply when AllegroGL is used. You can also use video bitmaps
 *    and it will be quite fast.
 *
 *  \{
 */


/* Computes the next power of two if the number wasn't a power of two to start
 * with. Ref: http://bob.allegronetwork.com/prog/tricks.html#roundtonextpowerof2
 */
int __allegro_gl_make_power_of_2(int x) {
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x++;
	return x;
}



/* allegro_gl_drawing_mode (GFX_DRIVER vtable entry):
 * Sets the drawing mode. Same implementation to all GFX vtables.
 */
void allegro_gl_drawing_mode(void) {
	if (__agl_drawing_mode == _drawing_mode)
		return;

	switch (__agl_drawing_mode) {
		case DRAW_MODE_TRANS:
			glDisable(GL_BLEND);
		break;
		case DRAW_MODE_XOR:
			glDisable(GL_COLOR_LOGIC_OP);
		break;
		case DRAW_MODE_COPY_PATTERN:
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			if (__agl_owning_drawing_pattern_tex && __agl_drawing_pattern_tex)
				glDeleteTextures(1, &__agl_drawing_pattern_tex);
			__agl_drawing_pattern_tex = 0;
			__agl_drawing_pattern_bmp = 0;
		break;
	}

	__agl_drawing_mode = _drawing_mode;

	switch (_drawing_mode) {
		case DRAW_MODE_TRANS:
			glEnable(GL_BLEND);
		break;

		case DRAW_MODE_XOR:
			glEnable(GL_COLOR_LOGIC_OP);
			glLogicOp(GL_XOR);
		break;

		case DRAW_MODE_COPY_PATTERN:
			if (is_memory_bitmap(_drawing_pattern)) {
				__agl_drawing_pattern_tex =
									allegro_gl_make_texture(_drawing_pattern);
				__agl_drawing_pattern_bmp = _drawing_pattern;
				__agl_owning_drawing_pattern_tex = TRUE;
			}
			else if (is_video_bitmap(_drawing_pattern)) {
				AGL_VIDEO_BITMAP *bmp = _drawing_pattern->extra;
				__agl_drawing_pattern_tex = bmp->tex;
				__agl_drawing_pattern_bmp = bmp->memory_copy;
				__agl_owning_drawing_pattern_tex = FALSE;
			}

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, __agl_drawing_pattern_tex);

			break;
	}
}


void split_color(int color, GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *a,
						int color_depth)
{
	AGL_LOG(2, "glvtable.c:split_color\n");
	*r = getr_depth(color_depth, color);
	*g = getg_depth(color_depth, color);
	*b = getb_depth(color_depth, color);
	if (color_depth == 32)
		*a = geta_depth(color_depth, color);
	else
		*a = 255;
}


/* allegro_gl_created_sub_bitmap:
 */
void allegro_gl_created_sub_bitmap(BITMAP *bmp, BITMAP *parent)
{
   bmp->extra = parent;
}


/* static void allegro_gl_screen_acquire(struct BITMAP *bmp) */
/** acquire_bitmap(screen) overload.
  * This doesn't do anything, since OpenGL rendering context
  * doesn't need locking. You don't need to call this function
  * in your program.
  */
static void allegro_gl_screen_acquire(struct BITMAP *bmp) {}



/* static void allegro_gl_screen_release(struct BITMAP *bmp) */
/** release_bitmap(screen) overload.
  * This doesn't do anything, since OpenGL rendering context
  * doesn't need locking. You don't need to call this function
  * in your program.
  */
static void allegro_gl_screen_release(struct BITMAP *bmp) {}



static int allegro_gl_screen_getpixel(struct BITMAP *bmp, int x, int y)
{
	GLubyte pixel[3];
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_getpixel\n");
	if (bmp->clip && (x < bmp->cl || x >= bmp->cr
	                                          || y < bmp->ct || y >= bmp->cb)) {
		return -1;
	}
	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}
	glReadPixels(x, bmp->h - y - 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

	return makecol_depth(bitmap_color_depth(screen),
	                                              pixel[0], pixel[1], pixel[2]);
}



static void allegro_gl_screen_putpixel(struct BITMAP *bmp, int x, int y,
                                                                      int color)
{
	GLubyte r, g, b, a;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_putpixel\n");
	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
	if (bmp->clip && (x < bmp->cl || x >= bmp->cr
	                                          || y < bmp->ct || y >= bmp->cb)) {
		return;
	}

	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}

	glColor4ub(r, g, b, a);
	glBegin(GL_POINTS);
		glVertex2f(x, y);
	glEnd();
}



static void allegro_gl_screen_vline(struct BITMAP *bmp, int x, int y1, int y2,
                                                                      int color)
{
	GLubyte r, g, b, a;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_vline\n");

	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}

	if (bmp->clip) {
		if ((x < bmp->cl) || (x >= bmp->cr)) {
			return;
		}
		if ((y1 >= bmp->cb) || (y2 < bmp->ct)) {
			return;
		}
		if (y1 < bmp->ct) {
			y1 = bmp->ct;
		}
		if (y2 >= bmp->cb) {
			y2 = bmp->cb - 1;
		}
	}
	
	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y1 += bmp->y_ofs;
		y2 += bmp->y_ofs;
	}

	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));

	glColor4ub(r, g, b, a);
	glBegin(GL_LINES);
		glVertex2f(x, y1);
		glVertex2f(x, y2 + 0.325 * 3);
	glEnd();

	return;
}



static void allegro_gl_screen_hline(struct BITMAP *bmp, int x1, int y, int x2,
                                                                      int color)
{
	GLubyte r, g, b, a;
	AGL_LOG(2, "glvtable.c:allegro_gl_hline\n");
	
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (bmp->clip) {
		if ((y < bmp->ct) || (y >= bmp->cb)) {
			return;
		}
		if ((x1 >= bmp->cr) || (x2 < bmp->cl)) {
			return;
		}
		if (x1 < bmp->cl) {
			x1 = bmp->cl;
		}
		if (x2 >= bmp->cr) {
			x2 = bmp->cr - 1;
		}
	}
	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		x2 += bmp->x_ofs;
		y += bmp->y_ofs;
	}
	
	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
	
	glColor4ub(r, g, b, a);
	glBegin(GL_LINES);
		glVertex2f(x1 - 0.325, y);
		glVertex2f(x2 + 0.325 * 2, y);
	glEnd();

	return;
}



static void allegro_gl_screen_line(struct BITMAP *bmp, int x1, int y1, int x2,
                                                              int y2, int color)
{
	GLubyte r, g, b, a;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_line\n");

	if (bmp->clip) {
		glPushAttrib(GL_SCISSOR_BIT);
		glEnable(GL_SCISSOR_TEST);
		glScissor(bmp->x_ofs + bmp->cl, bmp->h + bmp->y_ofs - bmp->cb,
		                                  bmp->cr - bmp->cl, bmp->cb - bmp->ct);
	}
	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		x2 += bmp->x_ofs;
		y1 += bmp->y_ofs;
		y2 += bmp->y_ofs;
	}
	
	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));

	glColor4ub(r, g, b, a);
	glBegin(GL_LINES);
		glVertex2f(x1 + 0.1625, y1 + 0.1625);
		glVertex2f(x2 + 0.1625, y2 + 0.1625);
	glEnd();

	/* OpenGL skips the endpoint when drawing lines */
	glBegin(GL_POINTS);
		glVertex2f(x2 + 0.1625, y2 + 0.1625);
	glEnd();
	
	if (bmp->clip) {
		glPopAttrib();
	}

	return;
}


#define SET_TEX_COORDS(x, y)  \
	do {                      \
		if (__agl_drawing_pattern_tex) {  \
			glTexCoord2f (                \
				(x - _drawing_x_anchor) / (float)__agl_drawing_pattern_bmp->w,\
				(y - _drawing_y_anchor) / (float)__agl_drawing_pattern_bmp->h \
			);                                                                \
		}                                                                     \
	} while(0)


void allegro_gl_screen_rectfill(struct BITMAP *bmp, int x1, int y1,
                                                      int x2, int y2, int color)
{
	GLubyte r, g, b, a;
	GLfloat old_col[4];
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_rectfill\n");
	
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}
	
	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}
	
	if (bmp->clip) {
		if ((x1 > bmp->cr) || (x2 < bmp->cl)) {
			return;
		}
		if (x1 < bmp->cl) {
			x1 = bmp->cl;
		}
		if (x2 > bmp->cr) {
			x2 = bmp->cr;
		}
		if ((y1 > bmp->cb) || (y2 < bmp->ct)) {
			return;
		}
		if (y1 < bmp->ct) {
			y1 = bmp->ct;
		}
		if (y2 > bmp->cb) {
			y2 = bmp->cb;
		}
	}
	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		x2 += bmp->x_ofs;
		y1 += bmp->y_ofs;
		y2 += bmp->y_ofs;
	}
	
	glGetFloatv(GL_CURRENT_COLOR, old_col);
	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
	glColor4ub(r, g, b, a);

	glBegin(GL_QUADS);
		SET_TEX_COORDS(x1, y1);
		glVertex2f(x1, y1);
		SET_TEX_COORDS(x2, y1);
		glVertex2f(x2, y1);
		SET_TEX_COORDS(x2, y2);
		glVertex2f(x2, y2);
		SET_TEX_COORDS(x1, y2);
		glVertex2f(x1, y2);
	glEnd();

	glColor4fv(old_col);

	return;
}



static void allegro_gl_screen_triangle(struct BITMAP *bmp, int x1, int y1,
                                      int x2, int y2, int x3, int y3, int color)
{
	GLubyte r, g, b, a;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_triangle\n");
	
	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
	
	if (bmp->clip) {
		glPushAttrib(GL_SCISSOR_BIT);
		glEnable(GL_SCISSOR_TEST);
		glScissor(bmp->x_ofs + bmp->cl, bmp->h + bmp->y_ofs - bmp->cb,
		                                 bmp->cr - bmp->cl, bmp->cb - bmp->ct);
	}
	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		y1 += bmp->y_ofs;
		x2 += bmp->x_ofs;
		y2 += bmp->y_ofs;
		x3 += bmp->x_ofs;
		y3 += bmp->y_ofs;
	}
	
	glColor4ub(r, g, b, a);
	glBegin(GL_TRIANGLES);
		SET_TEX_COORDS(x1, y1);
		glVertex2f(x1, y1);
		SET_TEX_COORDS(x2, y2);
		glVertex2f(x2, y2);
		SET_TEX_COORDS(x3, y3);
		glVertex2f(x3, y3);
	glEnd();

	if (bmp->clip) {
		glPopAttrib();
	}
}



#define BITMAP_BLIT_CLIP(source, dest, source_x, source_y, dest_x, dest_y, \
                                                          width, height) { \
	if (dest->clip) {                                                      \
		if ((dest_x >= dest->cr) || (dest_y >= dest->cb)                   \
		 || (dest_x + width < dest->cl) || (dest_y + height < dest->ct)) { \
			width = 0;                                                     \
		}                                                                  \
		if (dest_x < dest->cl) {                                           \
			width += dest_x - dest->cl;                                    \
			source_x -= dest_x - dest->cl;                                 \
			dest_x = dest->cl;                                             \
		}                                                                  \
		if (dest_y < dest->ct) {                                           \
			height += dest_y - dest->ct;                                   \
			source_y -= dest_y - dest->ct;                                 \
			dest_y = dest->ct;                                             \
		}                                                                  \
		if (dest_x + width > dest->cr) {                                   \
			width = dest->cr - dest_x;                                     \
		}                                                                  \
		if (dest_y + height > dest->cb) {                                  \
			height = dest->cb - dest_y;                                    \
		}                                                                  \
	}                                                                      \
	if (source->clip) {                                                    \
		if ((source_x >= source->cr) || (source_y >= source->cb)           \
		 || (source_x + width < source->cl)                                \
		 || (source_y + height < source->ct)) {                            \
			width = 0;                                                     \
		}                                                                  \
		if (source_x < source->cl) {                                       \
			width += source_x - source->cl;                                \
			dest_x -= source_x - source->cl;                               \
			source_x = source->cl;                                         \
		}                                                                  \
		if (source_y < source->ct) {                                       \
			height += source_y - source->ct;                               \
			dest_y -= source_y - source->ct;                               \
			source_y = source->ct;                                         \
		}                                                                  \
		if (source_x + width > source->cr) {                               \
			width = source->cr - source_x;                                 \
		}                                                                  \
		if (source_y + height > source->cb) {                              \
			height = source->cb - source_y;                                \
		}                                                                  \
	}                                                                      \
}
	



static void allegro_gl_screen_blit_from_memory(
    struct BITMAP *source, struct BITMAP *dest,
    int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
	GLfloat saved_zoom_x, saved_zoom_y;
	GLint saved_row_length;
	BITMAP *temp = NULL;
	void *data;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_blit_from_memory\n");

	BITMAP_BLIT_CLIP(source, dest, source_x, source_y, dest_x, dest_y,
	                                                             width, height);

	if (width <= 0 || height <= 0) {
		return;
	}
	

	if (is_sub_bitmap(dest)) {
		dest_x += dest->x_ofs;
		dest_y += dest->y_ofs;
	}

	/* Note: We don't need to offset the source bitmap coordinates
	 * because we use source->line[] directly, which is already offsetted for
	 * us.
	 */
	data = source->line[source_y]
	     + source_x * BYTES_PER_PIXEL(bitmap_color_depth(source));

	/* If packed pixels (or GL 1.2) isn't supported, then we need to convert
	 * the bitmap into something GL can understand - 24-bpp should do it.
	 */
	if (!allegro_gl_extensions_GL.EXT_packed_pixels
	                                       && bitmap_color_depth(source) < 24) {
		temp = create_bitmap_ex(24, width, height);

		if (temp) {
			blit(source, temp, source_x, source_y, 0, 0, width, height);
			source_x = 0;
			source_y = 0;
			data = temp->line[0];
		}
		else {
			/* XXX <rohannessian> Report error? */
			return;
		}
		source = temp;
	}
		

	/* Save state */
	glGetFloatv(GL_ZOOM_X, &saved_zoom_x);
	glGetFloatv(GL_ZOOM_Y, &saved_zoom_y);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &saved_row_length);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glRasterPos2i(dest_x, dest_y);

	/* XXX <rohannessian> I wonder if it would be faster to use glDrawPixels()
	 * one line at a time instead of playing with the Zoom factor.
	 */
	glPixelZoom (1.0, -1.0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,
	                (source->line[1] - source->line[0])
	                / BYTES_PER_PIXEL(source->vtable->color_depth));

	glDrawPixels(width, height, __allegro_gl_get_bitmap_color_format(source, 0),
		__allegro_gl_get_bitmap_type(source, 0), data);

	/* Restore state */
	glPixelZoom(saved_zoom_x, saved_zoom_y);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, saved_row_length);

	if (temp) {
		destroy_bitmap(temp);
	}
	return;
}



static void allegro_gl_screen_blit_to_memory(
      struct BITMAP *source, struct BITMAP *dest,
      int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
	GLint saved_row_length;
	GLint saved_alignment;
	GLint saved_pack_invert;

	BITMAP *bmp = NULL;

	AGL_LOG(2, "glvtable.c:allegro_gl_screen_blit_to_memory\n");

	BITMAP_BLIT_CLIP(source, dest, source_x, source_y, dest_x, dest_y,
	                                                             width, height);
	
	if (is_sub_bitmap(source)) {
		source_x += source->x_ofs;
		source_y += source->y_ofs;
	}
	if (is_sub_bitmap(dest)) {
		dest_x += dest->x_ofs;
		dest_y += dest->y_ofs;
	}

	if (width <= 0 || height <= 0) {
		return;
	}
	
	/* Note that glPixelZoom doesn't affect reads -- so we have to do a flip.
	 * We can do this by reading into a temporary bitmap then flipping that to
	 * the destination, -OR- use the GL_MESA_pack_invert extension to do it
	 * for us.
	 *
	 * If GL_EXT_packed_pixels isn't supported, then we can't use
	 * MESA_pack_invert on 16-bpp bitmaps or less.
	 */
	
	if ( !allegro_gl_extensions_GL.MESA_pack_invert
	 || (!allegro_gl_extensions_GL.EXT_packed_pixels
	   && bitmap_color_depth(dest) < 24)) {
	
		/* XXX <rohannessian> Bitmap format should be the same as the source
		 * dest bitmap!
		 */
		if ((!allegro_gl_extensions_GL.EXT_packed_pixels
		   && bitmap_color_depth(dest) < 24)) {
			bmp = create_bitmap_ex(24, width, height);
		}
		else {
			bmp = create_bitmap_ex(bitmap_color_depth(dest), width, height);
		}
		if (!bmp)
			return;
	}

	glGetIntegerv(GL_PACK_ROW_LENGTH, &saved_row_length);
	glGetIntegerv(GL_PACK_ALIGNMENT,  &saved_alignment);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_ALIGNMENT,  1);

	if (!allegro_gl_extensions_GL.MESA_pack_invert) {

		glReadPixels(source_x, source->h - source_y - height, width, height,
			__allegro_gl_get_bitmap_color_format(bmp, 0),
			__allegro_gl_get_bitmap_type(bmp, 0), bmp->dat);
	}
	else {
		glGetIntegerv(GL_PACK_INVERT_MESA, &saved_pack_invert);
		glPixelStorei(GL_PACK_INVERT_MESA, TRUE);
		glPixelStorei(GL_PACK_ROW_LENGTH,
		              (dest->line[1] - dest->line[0])
		               / BYTES_PER_PIXEL(dest->vtable->color_depth));
		
		glReadPixels(source_x, source->h - source_y - height, width, height,
			__allegro_gl_get_bitmap_color_format(dest, 0),
			__allegro_gl_get_bitmap_type(dest, 0), dest->line[0]);
		
		glPixelStorei(GL_PACK_INVERT_MESA, saved_pack_invert);
	}

	glPixelStorei(GL_PACK_ROW_LENGTH, saved_row_length);
	glPixelStorei(GL_PACK_ALIGNMENT,  saved_alignment);

	/* Flip image if needed (glPixelZoom doesn't affect reads) */
	if (bmp) {
		
		int y, dy;
		
		for (y = 0, dy = dest_y + height - 1; y < height; y++, dy--) {
			blit(bmp, dest, 0, y, dest_x, dy, width, 1);
		}

		destroy_bitmap(bmp);
	}

	return;
}




void allegro_gl_screen_blit_to_self (
     struct BITMAP *source, struct BITMAP *dest,
     int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_blit_to_self\n");

	BITMAP_BLIT_CLIP(source, dest, source_x, source_y, dest_x, dest_y,
	                                                             width, height);

	if (is_sub_bitmap(source)) {
		source_x += source->x_ofs;
		source_y += source->y_ofs;
	}
	if (is_sub_bitmap(dest)) {
		dest_x += dest->x_ofs;
		dest_y += dest->y_ofs;
	}

	if (width <= 0 || height <= 0) {
		return;
	}

	/* screen -> screen */
	if (is_screen_bitmap(source) && is_screen_bitmap(dest)) {
		glRasterPos2i(dest_x, dest_y + height - 1);
		glCopyPixels(source_x, SCREEN_H - source_y - height, width, height,
		                                                              GL_COLOR);
	}
	/* video -> screen */
	else if (is_screen_bitmap(dest) && is_video_bitmap(source)) {
		AGL_VIDEO_BITMAP *vid;
		BITMAP *source_parent = source;
		GLfloat current_color[4];

		while (source_parent->id & BMP_ID_SUB) {
			source_parent = (BITMAP *)source_parent->extra;
		}
		vid = source_parent->extra;

		glGetFloatv(GL_CURRENT_COLOR, current_color);
		glColor4ub(255, 255, 255, 255);

		while (vid) {
			int sx, sy;           /* source coordinates */
			int dx, dy;           /* destination coordinates */
			int w, h;

			if (source_x >= vid->x_ofs + vid->memory_copy->w ||
				source_y >= vid->y_ofs + vid->memory_copy->h ||
				vid->x_ofs >= source_x + width ||
				vid->y_ofs >= source_y + height) {
				vid = vid->next;
				continue;
			}

			sx = MAX(vid->x_ofs, source_x) - vid->x_ofs;
			w = MIN(vid->x_ofs + vid->memory_copy->w, source_x + width)
			  - vid->x_ofs - sx;
			sy = MAX(vid->y_ofs, source_y) - vid->y_ofs;
			h = MIN(vid->y_ofs + vid->memory_copy->h, source_y + height)
			  - vid->y_ofs - sy;
	
			dx = dest_x + vid->x_ofs + sx - source_x;
			dy = dest_y + vid->y_ofs + sy - source_y;

			glEnable(vid->target);
			glBindTexture(vid->target, vid->tex);

			if (vid->target == GL_TEXTURE_2D) {
				float tx = sx / (float)vid->memory_copy->w;
				float ty = sy / (float)vid->memory_copy->h;
				float tw = w / (float)vid->memory_copy->w;
				float th = h / (float)vid->memory_copy->h;

				glBegin(GL_QUADS);
					glTexCoord2f(tx, ty);
					glVertex2f(dx, dy);
					glTexCoord2f(tx, ty + th);
					glVertex2f(dx, dy + h);
					glTexCoord2f(tx + tw, ty + th);
					glVertex2f(dx + w, dy + h);
					glTexCoord2f(tx + tw, ty);
					glVertex2f(dx + w, dy);
				glEnd();
			}
			else {
				glBegin(GL_QUADS);
					glTexCoord2i(sx, sy);
					glVertex2f(dx, dy);
					glTexCoord2i(sx, sy + h);
					glVertex2f(dx, dy + h);
					glTexCoord2i(sx + w, sy + h);
					glVertex2f(dx + w, dy + h);
					glTexCoord2i(sx + w, sy);
					glVertex2f(dx + w, dy);
				glEnd();
			}

			glBindTexture(vid->target, 0);
			glDisable(vid->target);

			vid = vid->next;
		}
		
		glColor4fv(current_color);
	}
	/* screen -> video */
	else if (is_screen_bitmap(source) && is_video_bitmap(dest)) {
	
		AGL_VIDEO_BITMAP *vid;
		BITMAP *source_parent = source;

		while (source_parent->id & BMP_ID_SUB) {
			source_parent = (BITMAP *)source_parent->extra;
		}

		vid = dest->extra;

		while (vid) {
			int sx, sy;           /* source coordinates */
			int dx, dy;           /* destination coordinates */
			int w, h;

			if (dest_x >= vid->x_ofs + vid->memory_copy->w ||
				dest_y >= vid->y_ofs + vid->memory_copy->h ||
				vid->x_ofs >= dest_x + width ||
				vid->y_ofs >= dest_y + height) {
				vid = vid->next;
				continue;
			}

			dx = MAX(vid->x_ofs, dest_x) - vid->x_ofs;
			w = MIN(vid->x_ofs + vid->memory_copy->w, dest_x + width)
			  - vid->x_ofs - dx;
			dy = MAX(vid->y_ofs, dest_y) - vid->y_ofs;
			h = MIN(vid->y_ofs + vid->memory_copy->h, dest_y + height)
			  - vid->y_ofs - dy;
	
			sx = source_x + vid->x_ofs + dx - dest_x;
			sy = source_y + vid->y_ofs + dy - dest_y;

			/* We cannot use glCopyTexSubImage2D() here because it will flip the image. */
			allegro_gl_screen_blit_to_memory(source, vid->memory_copy,
			                                 sx, sy, dx, dy, w, h);

			allegro_gl_video_blit_from_memory(vid->memory_copy, dest, 0, 0,
				vid->x_ofs, vid->y_ofs, vid->memory_copy->w, vid->memory_copy->h);

			vid = vid->next;
		}
	}
	else if (is_video_bitmap(source) && is_video_bitmap(dest)) {
		allegro_gl_video_blit_to_self(source, dest, source_x, source_y,
									  dest_x, dest_y, width, height);
	}
}



void allegro_gl_upload_and_display_texture(struct BITMAP *source,
     int source_x, int source_y, int dest_x, int dest_y, int width, int height,
     int flip_dir, GLint format, GLint type)
{
	float tx, ty;
	GLint saved_row_length;
	int bytes_per_pixel = BYTES_PER_PIXEL(bitmap_color_depth(source));
	int i, j;
	
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, __allegro_gl_pool_texture);

	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &saved_row_length);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glPixelStorei(GL_UNPACK_ROW_LENGTH,
	                     (source->line[1] - source->line[0]) / bytes_per_pixel);
	
	for (i = 0; i <= abs(width) / 256; i++) {
		for (j = 0; j <= abs(height) / 256; j++) {

			void *data = source->line[source_y + j * 256]
			                           + (source_x + i * 256) * bytes_per_pixel;
			int w = abs(width)  - i * 256;
			int h = abs(height) - j * 256;
			int dx = dest_x + i * 256;
			int dy = dest_y + j * 256;

			w = (w & -256) ? 256 : w;
			h = (h & -256) ? 256 : h;

			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, type, data);

			tx = (float)w / 256.;
			ty = (float)h / 256.;

			if (flip_dir & AGL_H_FLIP) {
				dx = 2*dest_x + width - dx;
				w = -w;
			}

			if (flip_dir & AGL_V_FLIP) {
				dy = 2*dest_y + height - dy;
				h = -h;
			}

			if (width < 0)  w = -w;
			if (height < 0) h = -h;

			glBegin(GL_QUADS);
				glTexCoord2f(0., 0.);
				glVertex2i(dx, dy);
				glTexCoord2f(0., ty);
				glVertex2i(dx, dy + h);
				glTexCoord2f(tx, ty);
				glVertex2i(dx + w, dy + h);
				glTexCoord2f(tx, 0.);
				glVertex2i(dx + w, dy);
			glEnd();
		}
	}

	/* Restore state */
	glPixelStorei(GL_UNPACK_ROW_LENGTH, saved_row_length);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);

	return;
}



static void do_screen_masked_blit_standard(GLint format, GLint type, struct BITMAP *temp, int source_x, int source_y, int dest_x, int dest_y, int width, int height, int flip_dir, int blit_type)
{
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

	if (blit_type & AGL_NO_ROTATION) {
		GLint saved_row_length;
		float dx = dest_x, dy = dest_y;
		GLfloat zoom_x, zoom_y, old_zoom_x, old_zoom_y;

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);

		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &saved_row_length);
		glGetFloatv(GL_ZOOM_X, &old_zoom_x);
		glGetFloatv(GL_ZOOM_Y, &old_zoom_y);

		if (flip_dir & AGL_H_FLIP) {
			zoom_x = -1.0f;   
			/* Without the -0.5 below, we get an invalid position,
			 * and the operation is ignored by OpenGL. */
			dx += abs(width) - 0.5;
		}
		else {
			zoom_x = (float) width / abs(width);
		}

		if (flip_dir & AGL_V_FLIP) {
			zoom_y = 1.0f;
			dy += abs(height) - 0.5;
		}
		else {
			zoom_y = -1.0f * width / abs(width);
		}

		glRasterPos2f(dx, dy);
		glPixelZoom(zoom_x, zoom_y);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH,
		        (temp->line[1] - temp->line[0])
		        / BYTES_PER_PIXEL(bitmap_color_depth(temp)));

		glDrawPixels(abs(width), abs(height), format, type, temp->line[0]);
		
		glPixelStorei(GL_UNPACK_ROW_LENGTH, saved_row_length);
		glPixelZoom(old_zoom_x, old_zoom_y);
	}
	else {
		allegro_gl_upload_and_display_texture(temp, 0, 0, dest_x, dest_y, width, height,
		                           flip_dir, format, type);
	}

	glPopAttrib();
}



static void screen_masked_blit_standard(struct BITMAP *source,
    int source_x, int source_y, int dest_x, int dest_y, int width, int height,
    int flip_dir, int blit_type)
{
	BITMAP *temp = NULL;

	GLint format, type;
	
	format = __allegro_gl_get_bitmap_color_format(source, AGL_TEXTURE_MASKED);
	type   = __allegro_gl_get_bitmap_type(source, AGL_TEXTURE_MASKED);

	temp = __allegro_gl_munge_bitmap(AGL_TEXTURE_MASKED, source,
	                        source_x, source_y, abs(width), abs(height),
	                        &type, &format);

	if (temp) {
		source = temp;
	}

	do_screen_masked_blit_standard(format, type, source, source_x, source_y,
		dest_x, dest_y, width, height, flip_dir, blit_type);

	if (temp) {
		destroy_bitmap(temp);
	}

	return;
}



static void __allegro_gl_init_nv_register_combiners(BITMAP *bmp)
{
	GLfloat mask_color[4];
	int depth = bitmap_color_depth(bmp);
	int color = bitmap_mask_color(bmp);

	mask_color[0] = getr_depth(depth, color) / 255.;
	mask_color[1] = getg_depth(depth, color) / 255.;
	mask_color[2] = getb_depth(depth, color) / 255.;
	mask_color[3] = 0.;

	glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, mask_color);
	glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 2);
	glEnable(GL_REGISTER_COMBINERS_NV);

	glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
		GL_TEXTURE0_ARB, GL_SIGNED_IDENTITY_NV, GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
		GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV,
		GL_CONSTANT_COLOR0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB);
	glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
		GL_ZERO, GL_EXPAND_NORMAL_NV, GL_RGB);
	glCombinerOutputNV(GL_COMBINER0_NV, GL_RGB, GL_DISCARD_NV,
		GL_DISCARD_NV, GL_SPARE0_NV, GL_NONE, GL_NONE,
		GL_FALSE, GL_FALSE, GL_FALSE);

	glCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV,
		GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB);
	glCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV,
		GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB);
	glCombinerOutputNV(GL_COMBINER1_NV, GL_RGB, GL_SPARE1_NV,
		GL_DISCARD_NV, GL_DISCARD_NV, GL_NONE, GL_NONE,
		GL_TRUE, GL_FALSE, GL_FALSE);

	glFinalCombinerInputNV(GL_VARIABLE_A_NV, GL_TEXTURE0_ARB,
		GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_ZERO,
		GL_UNSIGNED_INVERT_NV, GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_C_NV, GL_ZERO,
		GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_D_NV, GL_ZERO,
		GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	glFinalCombinerInputNV(GL_VARIABLE_G_NV, GL_SPARE1_NV,
		GL_UNSIGNED_IDENTITY_NV, GL_BLUE);

	return;
}



static void screen_masked_blit_nv_register(struct BITMAP *source,
    int source_x, int source_y, int dest_x, int dest_y, int width, int height,
    int flip_dir, int blit_type)
{
	BITMAP *temp = NULL;
	GLint type   = __allegro_gl_get_bitmap_type(source, 0);
	GLint format = __allegro_gl_get_bitmap_color_format(source, 0);

	if (type == -1) {
		temp = create_bitmap_ex(24, width, height);
		if (!temp) {
			return;
		}
		blit(source, temp, source_x, source_y, 0, 0, width, height);
		source = temp;
		source_x = 0;
		source_y = 0;

		type   = __allegro_gl_get_bitmap_type(source, 0);
		format = __allegro_gl_get_bitmap_color_format(source, 0);
	}

	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
	__allegro_gl_init_nv_register_combiners(source);

	allegro_gl_upload_and_display_texture(source, source_x, source_y, dest_x, dest_y,
	                           width, height, flip_dir, format, type);

	glPopAttrib();

	if (temp) {
		destroy_bitmap(temp);
	}
	return;
}



static void __allegro_gl_init_combine_textures(BITMAP *bmp)
{
	GLubyte mask_color[4];

	split_color(bitmap_mask_color(bmp), &mask_color[0], &mask_color[1],
	    &mask_color[2], &mask_color[3], bitmap_color_depth(bmp));
	glColor4ubv(mask_color);

	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);

	/* Dot the result of the subtract with itself. Store it in the alpha 
	 * component. The alpha should then be 0 if the color fragment was equal to 
	 * the mask color, or >0 otherwise.
	 */
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

	/* Put the original RGB value in its place */

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);

	glActiveTexture(GL_TEXTURE0);

	return;
}



static void screen_masked_blit_combine_tex(struct BITMAP *source,
    int source_x, int source_y, int dest_x, int dest_y, int width, int height,
    int flip_dir, int blit_type)
{
	float tx, ty;
	BITMAP *temp = NULL;
	GLint saved_row_length;
	GLint type   = __allegro_gl_get_bitmap_type(source, 0);
	GLint format = __allegro_gl_get_bitmap_color_format(source, 0);
	int bytes_per_pixel;
	int i, j;
	GLfloat current_color[4];

	if (type == -1) {
		temp = create_bitmap_ex(24, width, height);
		if (!temp)
			return;
		blit(source, temp, source_x, source_y, 0, 0, width, height);
		source = temp;
		source_x = 0;
		source_y = 0;

		type   = __allegro_gl_get_bitmap_type(source, 0);
		format = __allegro_gl_get_bitmap_color_format(source, 0);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, __allegro_gl_pool_texture);
	
	glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
	glGetFloatv(GL_CURRENT_COLOR, current_color);
	__allegro_gl_init_combine_textures(source);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, __allegro_gl_pool_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, __allegro_gl_pool_texture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, __allegro_gl_pool_texture);
	glActiveTexture(GL_TEXTURE0);
	
	bytes_per_pixel = BYTES_PER_PIXEL(bitmap_color_depth(source));

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);

	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &saved_row_length);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glPixelStorei(GL_UNPACK_ROW_LENGTH,
	                     (source->line[1] - source->line[0]) / bytes_per_pixel);

	for (i = 0; i <= width / 256; i++) {
		for (j = 0; j <= height / 256; j++) {
				
			void *data = source->line[source_y + j * 256]
			                           + (source_x + i * 256) * bytes_per_pixel;
			int w = width - i * 256;
			int h = height - j * 256;
			int dx = dest_x + i * 256;
			int dy = dest_y + j * 256;

			w = (w & -256) ? 256 : w;
			h = (h & -256) ? 256 : h;

			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, type, data);

			tx = (float)w / 256.;
			ty = (float)h / 256.;

			if (flip_dir & AGL_H_FLIP) {
				dx = 2*dest_x + width - dx;
				w = -w;
			}

			if (flip_dir & AGL_V_FLIP) {
				dy = 2*dest_y + height - dy;
				h = -h;
			}

			glBegin(GL_QUADS);
				glMultiTexCoord2f(GL_TEXTURE0, 0., 0.);
				glMultiTexCoord2f(GL_TEXTURE1, 0., 0.);
				glMultiTexCoord2f(GL_TEXTURE2, 0., 0.);
				glVertex2f(dx, dy);
				glMultiTexCoord2f(GL_TEXTURE0, 0., ty);
				glMultiTexCoord2f(GL_TEXTURE1, 0., ty);
				glMultiTexCoord2f(GL_TEXTURE2, 0., ty);
				glVertex2f(dx, dy + h);
				glMultiTexCoord2f(GL_TEXTURE0, tx, ty);
				glMultiTexCoord2f(GL_TEXTURE1, tx, ty);
				glMultiTexCoord2f(GL_TEXTURE2, tx, ty);
				glVertex2f(dx + w, dy + h);
				glMultiTexCoord2f(GL_TEXTURE0, tx, 0.);
				glMultiTexCoord2f(GL_TEXTURE1, tx, 0.);
				glMultiTexCoord2f(GL_TEXTURE2, tx, 0.);
				glVertex2f(dx + w, dy);
			glEnd();
		}
	}

	/* Restore state */
	glPixelStorei(GL_UNPACK_ROW_LENGTH, saved_row_length);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glPopAttrib();
	glColor4fv(current_color);

	if (temp) {
		destroy_bitmap(temp);
	}

	return;
}



void do_masked_blit_screen(struct BITMAP *source, struct BITMAP *dest,
     int source_x, int source_y, int dest_x, int dest_y, int width, int height,
     int flip_dir, int blit_type)
{
	
	/* XXX <rohannessian> We should merge this clip code with the
	 * BITMAP_BLIT_CLIP macro
	 */

	/* Clipping of destination bitmap */
	if (dest->clip && (blit_type & AGL_NO_ROTATION)) {
		if ((dest_x >= dest->cr) || (dest_y >= dest->cb)
		 || (dest_x + width < dest->cl) || (dest_y + height < dest->ct)) {
			return;
		}
		if (flip_dir & AGL_H_FLIP) {
			if (dest_x < dest->cl) {
				width += dest_x - dest->cl;
				dest_x = dest->cl;
			}
			if (dest_x + width > dest->cr) {
				source_x += dest_x + width - dest->cr;
				width = dest->cr - dest_x;
			}
		}
		else {
			if (dest_x < dest->cl) {
				width += dest_x - dest->cl;
				source_x -= dest_x - dest->cl;
				dest_x = dest->cl;
			}
			if (dest_x + width > dest->cr) {
				width = dest->cr - dest_x;
			}
		}
		if (flip_dir & AGL_V_FLIP) {
			if (dest_y < dest->ct) {
				height += dest_y - dest->ct;
				dest_y = dest->ct;
			}
			if (dest_y + height > dest->cb) {
				source_y += dest_y + height - dest->cb;
				height = dest->cb - dest_y;
			}
		}
		else {
			if (dest_y < dest->ct) {
				height += dest_y - dest->ct;
				source_y -= dest_y - dest->ct;
				dest_y = dest->ct;
			}
			if (dest_y + height > dest->cb) {
				height = dest->cb - dest_y;
			}
		}
	}

	/* Clipping of source bitmap */
	if (source->clip && (blit_type & AGL_REGULAR_BMP)) {
		if ((source_x >= source->cr) || (source_y >= source->cb)
		 || (source_x + width < source->cl)
		 || (source_y + height < source->ct)) {
			return;
		}
		if (source_x < source->cl) {
			width += source_x - source->cl;
			dest_x -= source_x - source->cl;
			source_x = source->cl;
		}
		if (source_y < source->ct) {
			height += source_y - source->ct;
			dest_y -= source_y - source->ct;
			source_y = source->ct;
		}
		if (source_x + width > source->cr) {
			width = source->cr - source_x;
		}
		if (source_y + height > source->cb) {
			height = source->cb - source_y;
		}
	}
	if (is_sub_bitmap(dest)) {
		dest_x += dest->x_ofs;
		dest_y += dest->y_ofs;
	}
	if (width <= 0 || height <= 0)
		return;

	/* memory -> screen */
	if (!is_video_bitmap(source) && !is_screen_bitmap(source)) {

		__allegro_gl_driver->screen_masked_blit(source, source_x, source_y,
		                    dest_x, dest_y, width, height, flip_dir, blit_type);
	}
	/* video -> screen */
	else if (is_video_bitmap(source)) {
		AGL_VIDEO_BITMAP *vid;
		BITMAP *source_parent = source;

		int use_combiners = 0;

		/* Special combiner paths */
		if (allegro_gl_extensions_GL.NV_register_combiners
		 || allegro_gl_info.num_texture_units >= 3) {

			use_combiners = 1;

			glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);

			if (allegro_gl_extensions_GL.NV_register_combiners) {
				__allegro_gl_init_nv_register_combiners(source);
			}
			else {
				__allegro_gl_init_combine_textures(source);
			}

			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.0f);
		}

		while (source_parent->id & BMP_ID_SUB) {
			source_parent = (BITMAP *)source_parent->extra;
		}
		vid = source_parent->extra;

		while (vid) {
			int sx, sy;           /* source coordinates */
			int dx, dy;           /* destination coordinates */
			int w, h;

			if (source_x >= vid->x_ofs + vid->memory_copy->w ||
				source_y >= vid->y_ofs + vid->memory_copy->h ||
				vid->x_ofs >= source_x + width ||
				vid->y_ofs >= source_y + height) {
				vid = vid->next;
				continue;
			}

			sx = MAX (vid->x_ofs, source_x) - vid->x_ofs;
			w = MIN (vid->x_ofs + vid->memory_copy->w, source_x + width)
			  - vid->x_ofs - sx;
			sy = MAX (vid->y_ofs, source_y) - vid->y_ofs;
			h = MIN (vid->y_ofs + vid->memory_copy->h, source_y + height)
			  - vid->y_ofs - sy;

			dx = dest_x + vid->x_ofs + sx - source_x;
			dy = dest_y + vid->y_ofs + sy - source_y;

			if (flip_dir & AGL_H_FLIP) {
				dx = 2*dest_x + width - dx;
				w = -w;
			}

			if (flip_dir & AGL_V_FLIP) {
				dy = 2*dest_y + height - dy;
				h = -h;
			}

			if (use_combiners) {
				if (allegro_gl_extensions_GL.NV_register_combiners) {
					glEnable(vid->target);
					glBindTexture(vid->target, vid->tex);
					glTexParameteri(vid->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(vid->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

					if (vid->target == GL_TEXTURE_2D) {
						float tx = sx / (float)vid->memory_copy->w;
						float ty = sy / (float)vid->memory_copy->h;
						float tw = abs(w) / (float)vid->memory_copy->w;
						float th = abs(h) / (float)vid->memory_copy->h;

						glBegin(GL_QUADS);
							glTexCoord2f(tx, ty);
							glVertex2f(dx, dy);
							glTexCoord2f(tx, ty + th);
							glVertex2f(dx, dy + h);
							glTexCoord2f(tx + tw, ty + th);
							glVertex2f(dx + w, dy + h);
							glTexCoord2f(tx + tw, ty);
							glVertex2f(dx + w, dy);
						glEnd();
					}
					else {
						glBegin(GL_QUADS);
							glTexCoord2i(sx, sy);
							glVertex2f(dx, dy);
							glTexCoord2i(sx, sy + h);
							glVertex2f(dx, dy + h);
							glTexCoord2i(sx + w, sy + h);
							glVertex2f(dx + w, dy + h);
							glTexCoord2i(sx + w, sy);
							glVertex2f(dx + w, dy);
						glEnd();
					}

					glBindTexture(vid->target, 0);
					glDisable(vid->target);
				}
				else {
					glEnable(vid->target);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(vid->target, vid->tex);
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(vid->target, vid->tex);
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(vid->target, vid->tex);
					glActiveTexture(GL_TEXTURE0);
					glTexParameteri(vid->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(vid->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

					if (vid->target == GL_TEXTURE_2D) {
						float tx, ty, tw, th; /* texture coordinates */
						tx = sx / (float)vid->memory_copy->w;
						ty = sy / (float)vid->memory_copy->h;
						tw = abs(w) / (float)vid->memory_copy->w;
						th = abs(h) / (float)vid->memory_copy->h;

						glBegin(GL_QUADS);
							glMultiTexCoord2f(GL_TEXTURE0, tx, ty);
							glMultiTexCoord2f(GL_TEXTURE1, tx, ty);
							glMultiTexCoord2f(GL_TEXTURE2, tx, ty);
							glVertex2f(dx, dy);
							glMultiTexCoord2f(GL_TEXTURE0, tx, ty + th);
							glMultiTexCoord2f(GL_TEXTURE1, tx, ty + th);
							glMultiTexCoord2f(GL_TEXTURE2, tx, ty + th);
							glVertex2f(dx, dy + h);
							glMultiTexCoord2f(GL_TEXTURE0, tx + tw, ty + th);
							glMultiTexCoord2f(GL_TEXTURE1, tx + tw, ty + th);
							glMultiTexCoord2f(GL_TEXTURE2, tx + tw, ty + th);
							glVertex2f(dx + w, dy + h);
							glMultiTexCoord2f(GL_TEXTURE0, tx + tw, ty);
							glMultiTexCoord2f(GL_TEXTURE1, tx + tw, ty);
							glMultiTexCoord2f(GL_TEXTURE2, tx + tw, ty);
							glVertex2f(dx + w, dy);
						glEnd();
					}
					else {
						glBegin(GL_QUADS);
							glMultiTexCoord2i(GL_TEXTURE0, dx, dy);
							glMultiTexCoord2i(GL_TEXTURE1, dx, dy);
							glMultiTexCoord2i(GL_TEXTURE2, dx, dy);
							glVertex2f(dx, dy);
							glMultiTexCoord2i(GL_TEXTURE0, dx, dy + h);
							glMultiTexCoord2i(GL_TEXTURE1, dx, dy + h);
							glMultiTexCoord2i(GL_TEXTURE2, dx, dy + h);
							glVertex2f(dx, dy + h);
							glMultiTexCoord2i(GL_TEXTURE0, dx + w, dy + h);
							glMultiTexCoord2i(GL_TEXTURE1, dx + w, dy + h);
							glMultiTexCoord2i(GL_TEXTURE2, dx + w, dy + h);
							glVertex2f(dx + w, dy + h);
							glMultiTexCoord2i(GL_TEXTURE0, dx + w, dy);
							glMultiTexCoord2i(GL_TEXTURE1, dx + w, dy);
							glMultiTexCoord2i(GL_TEXTURE2, dx + w, dy);
							glVertex2f(dx + w, dy);
						glEnd();
					}

					glBindTexture(vid->target, 0);
					glDisable(vid->target);
				}
			}
			else {
				screen_masked_blit_standard(vid->memory_copy, sx, sy, dx, dy,
				                            w, h, FALSE, blit_type);
			}

			vid = vid->next;
		}

		if (use_combiners) {
			glPopAttrib();
		}
	}
	return;
}



static BITMAP* __allegro_gl_convert_rle_sprite(AL_CONST struct RLE_SPRITE *sprite, int trans)
{
	BITMAP *temp = NULL;
	int y, x, src_depth;
	signed long src_mask;

	#define DRAW_RLE_8888(bits)					\
	{								\
		for (y = 0; y < sprite->h; y++) {			\
			signed long c = *s++;				\
			for (x = 0; x < sprite->w;) {			\
				if (c == src_mask)			\
					break;				\
				if (c > 0) {				\
					/* Run of solid pixels */	\
					for (c--; c>=0; c--) {		\
						unsigned long col = *s++;		\
						if (bits == 32 && trans)		\
							_putpixel32(temp, x++, y, makeacol32(getr32(col), getg32(col), getb32(col), geta32(col))); \
						else			\
							_putpixel32(temp, x++, y, makeacol32(getr##bits(col), getg##bits(col), getb##bits(col), 255)); \
					}				\
				}					\
				else {					\
					/* Run of transparent pixels */	\
					hline(temp, x, y, x-c+1, 0);	\
					x -= c;				\
				}					\
				c = *s++;				\
			}						\
		}							\
	}

	src_depth = sprite->color_depth;
	if (src_depth == 8)
		src_mask = 0;
	else
		src_mask = makecol_depth(src_depth, 255, 0, 255);

	temp = create_bitmap_ex(32, sprite->w, sprite->h);
	if (!temp) return NULL;

	/* RGBA 8888 */
	switch(src_depth) {
		case 8:
		{
			signed char *s = (signed char*)sprite->dat;
			DRAW_RLE_8888(8);
			break;
		}
		case 15:
		{
			int16_t *s = (int16_t*)sprite->dat;
			DRAW_RLE_8888(15);
			break;
		}
		case 16:
		{
			int16_t *s = (int16_t*)sprite->dat;
			DRAW_RLE_8888(16);
			break;
		}
		case 24:
		{
			int32_t *s = (int32_t*)sprite->dat;
			DRAW_RLE_8888(24);
			break;
		}
		case 32:
		{
			int32_t *s = (int32_t*)sprite->dat;
			DRAW_RLE_8888(32);
			break;
		}
	}

	return temp;
}



void allegro_gl_screen_draw_rle_sprite(struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y)
{
	BITMAP *temp = NULL, *temp2 = NULL;
	int source_x = 0, source_y = 0;
	int width = sprite->w, height = sprite->h;

	temp = __allegro_gl_convert_rle_sprite(sprite, FALSE);
	if (!temp)
		return;

	BITMAP_BLIT_CLIP(temp, bmp, source_x, source_y, x, y, width, height);
	
	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}

	if (width <= 0 || height <= 0) {
		destroy_bitmap(temp);
		return;
	}

	temp2 = create_sub_bitmap(temp, source_x, source_y, width, height);
	if (!temp2) {
		destroy_bitmap(temp);
		return;
	}

	do_screen_masked_blit_standard(GL_RGBA, 
		__allegro_gl_get_bitmap_type(temp2, AGL_TEXTURE_MASKED), temp2,
		0, 0, x, y, width, height, FALSE, AGL_NO_ROTATION);

	destroy_bitmap(temp2);
	destroy_bitmap(temp);
}


static void allegro_gl_screen_draw_trans_rgba_rle_sprite(struct BITMAP *bmp,
							AL_CONST struct RLE_SPRITE *sprite, int x, int y) {
	BITMAP *temp = NULL, *temp2 = NULL;
	int source_x = 0, source_y = 0;
	int width = sprite->w, height = sprite->h;

	temp = __allegro_gl_convert_rle_sprite(sprite, TRUE);
	if (!temp)
		return;

	BITMAP_BLIT_CLIP(temp, bmp, source_x, source_y, x, y, width, height);
	
	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}

	if (width <= 0 || height <= 0) {
		destroy_bitmap(temp);
		return;
	}

	temp2 = create_sub_bitmap(temp, source_x, source_y, width, height);
	if (!temp2) {
		destroy_bitmap(temp);
		return;
	}
	
	if (__allegro_gl_blit_operation == AGL_OP_LOGIC_OP)
		glEnable(GL_COLOR_LOGIC_OP);
	else
		glEnable(GL_BLEND);

	allegro_gl_upload_and_display_texture(temp2, 0, 0, x, y, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE);
	
	if (__allegro_gl_blit_operation == AGL_OP_LOGIC_OP)
		glDisable(GL_COLOR_LOGIC_OP);
	else
		glDisable(GL_BLEND);

	destroy_bitmap(temp2);
	destroy_bitmap(temp);
}



static void allegro_gl_screen_masked_blit(struct BITMAP *source,
    struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y,
    int width, int height)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_masked_blit\n");
	do_masked_blit_screen(source, dest, source_x, source_y, dest_x, dest_y,
	                      width, height, FALSE, AGL_REGULAR_BMP | AGL_NO_ROTATION);
}



static void allegro_gl_screen_draw_sprite(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_sprite\n");
	do_masked_blit_screen(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
	                      FALSE, AGL_NO_ROTATION);
}



static void allegro_gl_screen_draw_sprite_v_flip(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_sprite_v_flip\n");
	do_masked_blit_screen(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
	                      AGL_V_FLIP, AGL_NO_ROTATION);
}



static void allegro_gl_screen_draw_sprite_h_flip(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_sprite_h_flip\n");
	do_masked_blit_screen(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
	                      AGL_H_FLIP, AGL_NO_ROTATION);
}



static void allegro_gl_screen_draw_sprite_vh_flip(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_sprite_vh_flip\n");
	do_masked_blit_screen(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
	                      AGL_V_FLIP | AGL_H_FLIP, AGL_NO_ROTATION);
}



static void allegro_gl_screen_pivot_scaled_sprite_flip(struct BITMAP *bmp,
    struct BITMAP *sprite, fixed x, fixed y, fixed cx, fixed cy, fixed angle,
    fixed scale, int v_flip)
{
	double dscale = fixtof(scale);
	GLint matrix_mode;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_pivot_scaled_sprite_flip\n");
	
#define BIN_2_DEG(x) (-(x) * 180.0 / 128)
	
	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(fixtof(x), fixtof(y), 0.);
	glRotated(BIN_2_DEG(fixtof(angle)), 0., 0., -1.);
	glScaled(dscale, dscale, dscale);
	glTranslated(-fixtof(x+cx), -fixtof(y+cy), 0.);
	
	do_masked_blit_screen(sprite, bmp, 0, 0, fixtoi(x), fixtoi(y),
	                      sprite->w, sprite->h, v_flip ? AGL_V_FLIP : FALSE, FALSE);
	glPopMatrix();
	glMatrixMode(matrix_mode);

#undef BIN_2_DEG

	return;
}



static void allegro_gl_screen_draw_trans_rgba_sprite(struct BITMAP *bmp,
	struct BITMAP *sprite, int x, int y) {

	if (__allegro_gl_blit_operation == AGL_OP_LOGIC_OP)
		glEnable(GL_COLOR_LOGIC_OP);
	else
		glEnable(GL_BLEND);

	/* video -> screen */
	if (is_video_bitmap(sprite)) {
		allegro_gl_screen_blit_to_self(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h);
	}
	/* memory -> screen */
	else if (is_memory_bitmap(sprite)) {
		GLint format = __allegro_gl_get_bitmap_color_format(sprite, AGL_TEXTURE_HAS_ALPHA);
		GLint type = __allegro_gl_get_bitmap_type(sprite, 0);
		allegro_gl_upload_and_display_texture(sprite, 0, 0, x, y, sprite->w, sprite->h, 0, format, type);
	}
	
	if (__allegro_gl_blit_operation == AGL_OP_LOGIC_OP)
		glDisable(GL_COLOR_LOGIC_OP);
	else
		glDisable(GL_BLEND);
	
	return;
}



static void allegro_gl_screen_draw_sprite_ex(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y, int mode, int flip)
{
	int lflip = 0;
	int matrix_mode;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_sprite_ex\n");

	/* convert allegro's flipping flags to AGL's flags */
	switch (flip) {
		case DRAW_SPRITE_NO_FLIP:
			lflip = FALSE;
		break;
		case DRAW_SPRITE_V_FLIP:
			lflip = AGL_V_FLIP;
		break;
		case DRAW_SPRITE_H_FLIP:
			lflip = AGL_H_FLIP;
		break;
		case DRAW_SPRITE_VH_FLIP:
			lflip = AGL_V_FLIP | AGL_H_FLIP;
		break;
	}

	switch (mode) {
		case DRAW_SPRITE_NORMAL:
			do_masked_blit_screen(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
				lflip, AGL_NO_ROTATION);
		break;
		case DRAW_SPRITE_TRANS:
			if (lflip) {
				glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();

				glTranslatef(x, y, 0.f);
				glScalef((lflip&AGL_H_FLIP) ? -1 : 1, (lflip&AGL_V_FLIP)? -1 : 1, 1);
				glTranslatef(-x, -y, 0);
				glTranslatef((lflip&AGL_H_FLIP) ? -sprite->w : 0,
						     (lflip&AGL_V_FLIP) ? -sprite->h : 0, 0);
			}

			allegro_gl_screen_draw_trans_rgba_sprite(bmp, sprite, x, y);

			if (lflip) {
				glPopMatrix();
				glMatrixMode(matrix_mode);
			}
		break;
		case DRAW_SPRITE_LIT:
		/* unsupported */
		break;
	}
}



void allegro_gl_screen_draw_glyph_ex(struct BITMAP *bmp,
                                  AL_CONST struct FONT_GLYPH *glyph, int x, int y,
								  int color, int bg, int flip)
{
	GLubyte r, g, b, a;
	int x_offs = 0;
	int i;

	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_glyph_ex\n");
	
	if (bmp->clip) {
		glPushAttrib(GL_SCISSOR_BIT);
		glEnable(GL_SCISSOR_TEST);
		glScissor(bmp->x_ofs + bmp->cl, bmp->h + bmp->y_ofs - bmp->cb,
		          bmp->cr - bmp->cl, bmp->cb - bmp->ct);

		if (x < bmp->cl) {
			x_offs -= x - bmp->cl;
			x = bmp->cl;
		}
	}
	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}
	
	if (bg != -1) {
		split_color(bg, &r, &g, &b, &a, bitmap_color_depth(bmp));
		glColor4ub(r, g, b, a);
		glRecti(x, y, x + glyph->w, y + glyph->h);				
	}

	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
	glColor4ub(r, g, b, a);
	glRasterPos2i(x, y);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

	if (flip) {
		for (i = 0; i < glyph->h; i++) {
			glBitmap(glyph->w, 1, x_offs, i, 0, 2,
		                                 glyph->dat + i * ((glyph->w + 7) / 8));
		}
	}
	else {
		for (i = 0; i < glyph->h; i++) {
			glBitmap(glyph->w, 1, x_offs, i, 0, 0,
		                                 glyph->dat + i * ((glyph->w + 7) / 8));
		}
	}
	
	if (bmp->clip) {
		glPopAttrib();
	}

	return;
}



static void allegro_gl_screen_draw_glyph(struct BITMAP *bmp,
                                  AL_CONST struct FONT_GLYPH *glyph, int x, int y,
								  int color, int bg) {
	allegro_gl_screen_draw_glyph_ex(bmp, glyph, x, y, color, bg, 0);
}



void allegro_gl_screen_draw_color_glyph_ex(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y, int color, int bg, int flip)
{

	/* Implementation note: we should try building textures and see how well
	 * those work instead of of DrawPixels with a weird I_TO_RGBA mapping.
	 */
	static GLfloat red_map[256];
	static GLfloat green_map[256];
	static GLfloat blue_map[256];
	static GLfloat alpha_map[256];
	GLubyte r, g, b, a;
	int i;
	GLint saved_row_length;
	GLint width, height;
	int sprite_x = 0, sprite_y = 0;
	void *data;
	int *table;

	width = sprite->w;
	height = sprite->h;

	if (bmp->clip) {
		if ((x >= bmp->cr) || (y >= bmp->cb) || (x + width < bmp->cl)
		 || (y + height < bmp->ct)) {
			return;
		}
		if (x < bmp->cl) {
			width += x - bmp->cl;
			sprite_x -= (x - bmp->cl);
			x = bmp->cl;
		}
		if (y < bmp->ct) {
			height += y - bmp->ct;
			sprite_y -= (y - bmp->ct);
			y = bmp->ct;
		}
		if (x + width > bmp->cr) {
			width = bmp->cr - x;
		}
		if (y + height > bmp->cb) {
			height = bmp->cb - y;
		}
	}
	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}

	data = sprite->line[sprite_y]
	     + sprite_x * BYTES_PER_PIXEL(bitmap_color_depth(sprite));

	if (bg < 0) {
		glAlphaFunc(GL_GREATER, 0.0f);
		glEnable(GL_ALPHA_TEST);
		alpha_map[0] = 0.;
	}
	else {
		split_color(bg, &r, &g, &b, &a, bitmap_color_depth(bmp));
		red_map[0] = r / 255.;
		green_map[0] = g / 255.;
		blue_map[0] = b / 255.;
		alpha_map[0] = 1.;
	}

	if (color < 0) {
		table = _palette_expansion_table(bitmap_color_depth(bmp));

		for(i = 1; i < 255; i++) {
			split_color(table[i], &r, &g, &b, &a, bitmap_color_depth(bmp));
			red_map[i] = r / 255.;
			green_map[i] = g / 255.;
			blue_map[i] = b / 255.;
			alpha_map[i] = 1.;
		}
	}
	else {
		split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));

		for(i = 1; i < 255; i++) {
			red_map[i] = r / 255.;
			green_map[i] = g / 255.;
			blue_map[i] = b / 255.;
			alpha_map[i] = 1.;
		}
	}
	
	glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, red_map);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, green_map);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, blue_map);
	glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, alpha_map);
	
	glRasterPos2i(x, y);
	glPushAttrib(GL_PIXEL_MODE_BIT);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &saved_row_length);
	
	glPixelZoom(1.0, flip ? -1.0 : 1.0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, sprite->w);
	glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

	glDrawPixels(width, height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, saved_row_length);
	glPopAttrib();
	if (bg < 0) {
		glDisable(GL_ALPHA_TEST);
	}

	return;
}



static void allegro_gl_screen_draw_color_glyph(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y, int color, int bg) {
	allegro_gl_screen_draw_color_glyph_ex(bmp, sprite, x, y, color, bg, 1);
}



static void allegro_gl_screen_draw_character(struct BITMAP *bmp,
                         struct BITMAP *sprite, int x, int y, int color, int bg)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_character\n");
	allegro_gl_screen_draw_color_glyph(bmp, sprite, x, y, color, bg);
}



static void allegro_gl_screen_draw_256_sprite(struct BITMAP *bmp,
                                            struct BITMAP *sprite, int x, int y)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_draw_256_sprite\n");
	allegro_gl_screen_draw_color_glyph(bmp, sprite, x, y, -1, -1);
}



void allegro_gl_screen_clear_to_color(struct BITMAP *bmp, int color)
{
	if (__agl_drawing_pattern_tex || bmp->clip) {
		allegro_gl_screen_rectfill(bmp, 0, 0, bmp->w, bmp->h, color);
	}
	else {
		GLubyte r, g, b, a;
		GLfloat old_col[4];
	
		split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
		
		glGetFloatv(GL_COLOR_CLEAR_VALUE, old_col);
		glClearColor(((float) r / 255), ((float) g / 255), ((float) b / 255),
					 ((float) a / 255));

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(old_col[0], old_col[1], old_col[2], old_col[3]);
	}

	return;
}



/* TODO: Handle concave and self-intersecting. */
static void allegro_gl_screen_polygon(struct BITMAP *bmp, int vertices,
									  AL_CONST int *points, int color) {
	GLubyte r, g, b, a;
	int i;

	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
	glColor4ub(r, g, b, a);
	
	glPushAttrib(GL_SCISSOR_BIT);

	if (bmp->clip) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(bmp->x_ofs + bmp->cl, bmp->h + bmp->y_ofs - bmp->cb,
				  bmp->cr - bmp->cl, bmp->cb - bmp->ct);
	}
	else {
		glScissor(0, 0, bmp->w, bmp->h);
	}

	glBegin(GL_POLYGON);
		for (i = 0; i < vertices*2-1; i+=2) {
			SET_TEX_COORDS(points[i], points[i+1]);
			if (is_sub_bitmap(bmp)) {
				glVertex2f(points[i] + bmp->x_ofs, points[i+1] + bmp->y_ofs);
			}
			else {
				glVertex2f(points[i], points[i+1]);
			}
		}
	glEnd();

	glPopAttrib();
}



static void allegro_gl_screen_rect(struct BITMAP *bmp,
								   int x1, int y1, int x2, int y2, int color) {
	GLubyte r, g, b, a;

	split_color(color, &r, &g, &b, &a, bitmap_color_depth(bmp));
	glColor4ub(r, g, b, a);
	
	glPushAttrib(GL_SCISSOR_BIT);

	if (bmp->clip) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(bmp->x_ofs + bmp->cl, bmp->h + bmp->y_ofs - bmp->cb,
				  bmp->cr - bmp->cl, bmp->cb - bmp->ct);
	}
	else {
		glScissor(0, 0, bmp->w, bmp->h);
	}
	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		x2 += bmp->x_ofs;
		y1 += bmp->y_ofs;
		y2 += bmp->y_ofs;
	}

	glBegin(GL_LINE_STRIP);
		glVertex2f(x1, y1);
		glVertex2f(x2, y1);
		glVertex2f(x2, y2);
		glVertex2f(x1, y2);
		glVertex2f(x1, y1);
	glEnd();

	glPopAttrib();
}



void allegro_gl_screen_polygon3d_f(struct BITMAP *bmp, int type,
								   struct BITMAP *texture, int vc,
								   V3D_f *vtx[]) {
	int i;
	int use_z = FALSE;

	if (type & POLYTYPE_ZBUF) {
		use_z = TRUE;
		type &= ~POLYTYPE_ZBUF;
	}

	if (type == POLYTYPE_PTEX || type == POLYTYPE_PTEX_TRANS)
		use_z = TRUE;

	if (bmp->clip) {
		glPushAttrib(GL_SCISSOR_BIT);
		glEnable(GL_SCISSOR_TEST);
		glScissor(bmp->x_ofs + bmp->cl, bmp->h + bmp->y_ofs - bmp->cb,
				  bmp->cr - bmp->cl, bmp->cb - bmp->ct);
	}
	if (is_sub_bitmap(bmp)) {
		for (i = 0; i < vc*2-1; i+=2) {
			vtx[i] += bmp->x_ofs;
			vtx[i+1] += bmp->y_ofs;
		}
	}

	if (use_z) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	}

	glColor4ub(255, 255, 255, 255);

	if (type == POLYTYPE_ATEX || type == POLYTYPE_PTEX
	 || type == POLYTYPE_ATEX_TRANS || type == POLYTYPE_PTEX_TRANS) {
		drawing_mode(DRAW_MODE_COPY_PATTERN, texture, 0, 0);
	}

	if (type == POLYTYPE_ATEX_TRANS || type == POLYTYPE_PTEX_TRANS) {
		glEnable(GL_BLEND);
	}

	glBegin(GL_POLYGON);
		for (i = 0; i < vc; i++) {
			if (type == POLYTYPE_FLAT)
				glColor3ub(getr(vtx[0]->c), getg(vtx[0]->c), getb(vtx[0]->c));
			else if (type == POLYTYPE_GRGB)
				glColor3ub(getr24(vtx[i]->c), getg24(vtx[i]->c), getb24(vtx[i]->c));
			else if (type == POLYTYPE_GCOL)
				glColor3ub(getr(vtx[i]->c), getg(vtx[i]->c), getb(vtx[i]->c));
			else if (type == POLYTYPE_ATEX || type == POLYTYPE_PTEX
				  || type == POLYTYPE_ATEX_TRANS || type == POLYTYPE_PTEX_TRANS) {
					SET_TEX_COORDS(vtx[i]->u, vtx[i]->v);
			}

			if (use_z)
				glVertex3f(vtx[i]->x, vtx[i]->y, 1.f / vtx[i]->z);
			else
				glVertex2f(vtx[i]->x, vtx[i]->y);
		}
	glEnd();

	if (bmp->clip)
		glPopAttrib();

	if (use_z) {
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}

	if (type == POLYTYPE_ATEX || type == POLYTYPE_PTEX
	 || type == POLYTYPE_ATEX_TRANS || type == POLYTYPE_PTEX_TRANS) {
		solid_mode();
	}

	if (type == POLYTYPE_ATEX_TRANS || type == POLYTYPE_PTEX_TRANS)
		glDisable(GL_BLEND);
}



static void allegro_gl_screen_polygon3d(struct BITMAP *bmp, int type,
										struct BITMAP *texture, int vc,
										V3D *vtx[]) {
	int i;
	V3D_f **vtx_f = malloc(vc * sizeof(struct V3D_f*));
	if (!vtx_f)
		return;

	for (i = 0; i < vc; i++) {
		vtx_f[i] = malloc(sizeof(struct V3D_f));
		if (!vtx_f[i]) {
			int k;
			for (k = 0; k < i; k++)
				free(vtx_f[k]);
			free(vtx_f);
			return;
		}
		vtx_f[i]->c = vtx[i]->c;
		vtx_f[i]->u = fixtof(vtx[i]->u);
		vtx_f[i]->v = fixtof(vtx[i]->v);
		vtx_f[i]->x = fixtof(vtx[i]->x);
		vtx_f[i]->y = fixtof(vtx[i]->y);
		vtx_f[i]->z = fixtof(vtx[i]->z);
	}

	allegro_gl_screen_polygon3d_f(bmp, type, texture, vc, vtx_f);
	for (i = 0; i < vc; i++)
		free(vtx_f[i]);
	free(vtx_f);
}


static void allegro_gl_screen_quad3d_f(struct BITMAP *bmp, int type,
									   struct BITMAP *texture,
									   V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4) {

	V3D_f *vtx_f[4];
	vtx_f[0] = v1;
	vtx_f[1] = v2;
	vtx_f[2] = v3;
	vtx_f[3] = v4;

	allegro_gl_screen_polygon3d_f(bmp, type, texture, 4, vtx_f);
}



static void allegro_gl_screen_quad3d(struct BITMAP *bmp, int type,
			struct BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4) {

	V3D *vtx[4];
	vtx[0] = v1;
	vtx[1] = v2;
	vtx[2] = v3;
	vtx[3] = v4;

	allegro_gl_screen_polygon3d(bmp, type, texture, 4, vtx);
}



static void allegro_gl_screen_triangle3d(struct BITMAP *bmp, int type,
										 struct BITMAP *texture,
										 V3D *v1, V3D *v2, V3D *v3) {
	V3D *vtx[3];
	vtx[0] = v1;
	vtx[1] = v2;
	vtx[2] = v3;

	allegro_gl_screen_polygon3d(bmp, type, texture, 3, vtx);
}



static void allegro_gl_screen_triangle3d_f(struct BITMAP *bmp, int type,
										   struct BITMAP *texture,
										   V3D_f *v1, V3D_f *v2, V3D_f *v3) {
	V3D_f *vtx_f[3];
	vtx_f[0] = v1;
	vtx_f[1] = v2;
	vtx_f[2] = v3;

	allegro_gl_screen_polygon3d_f(bmp, type, texture, 3, vtx_f);
}



void __allegro_gl__glvtable_update_vtable(GFX_VTABLE ** vtable)
{
	int maskcolor = (*vtable)->mask_color;
	int depth = (*vtable)->color_depth;

	AGL_LOG(2, "glvtable.c:__allegro_gl__glvtable_update_vtable\n");
	allegro_gl_screen_vtable.color_depth = depth;
	/* makecol_depth is used below instead of the MASK_COLOR_x constants
	 * because we may have changed the RGB shift values in order to
	 * use the packed pixels extension
	 */
	allegro_gl_screen_vtable.mask_color =
	    makecol_depth(depth, getr(maskcolor), getg(maskcolor), getb(maskcolor));
	
	*vtable = &allegro_gl_screen_vtable;

	__allegro_gl_driver->screen_masked_blit = screen_masked_blit_standard;
	if (allegro_gl_extensions_GL.NV_register_combiners) {
		__allegro_gl_driver->screen_masked_blit
		                                       = screen_masked_blit_nv_register;
	}
	else if (allegro_gl_info.num_texture_units >= 3) {
		__allegro_gl_driver->screen_masked_blit =
		                                         screen_masked_blit_combine_tex;
	}
}



/* Saved projection matrix */
static double allegro_gl_projection_matrix[16];
static double allegro_gl_modelview_matrix[16];



/** \ingroup allegro
  * Prepares for Allegro drawing to the screen.
  *
  * Since AllegroGL actually calls OpenGL commands to perform Allegro functions
  * for 2D drawing, some OpenGL capabilities may interfer with those operations
  * In order to obtain the expected results, allegro_gl_set_allegro_mode() must
  * be called to disable the depth test, texturing, fog and lighting and set
  * the view matrices to an appropriate state. Call
  * allegro_gl_unset_allegro_mode() to restore OpenGL in its previous state.
  *
  * You should encapsulate all Allegro code dealing with the screen in
  * between allegro_gl_set_allegro_mode() and allegro_gl_unset_allegro_mode().
  *
  * If you need to use regular OpenGL commands in between, you may do so,
  * but you can get unexpected results. This is generally not recommended.
  * You should first call allegro_gl_unset_allegro_mode() to restore the
  * original OpenGL matrices. After that, you may freely call any OpenGL
  * command. Don't forget to call back allegro_gl_set_allegro_mode() to switch
  * back to Allegro commands.
  *
  * AllegroGL saves the current OpenGL state with glPushAttrib so you should
  * make sure that at least one level is available in the attribute stack
  * otherwise the next call to allegro_gl_unset_allegro_mode() may fail.
  *
  * Also note that allegro_gl_set_allegro_mode() implicitely calls
  * allegro_gl_set_projection() so you do not need to do it yourself.
  *
  * \sa	allegro_gl_unset_allegro_mode() allegro_gl_set_projection()
  * \sa allegro_gl_unset_projection()
  */
void allegro_gl_set_allegro_mode(void)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_set_allegro_mode\n");

	/* Save the OpenGL state  then set it up */
	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT
	           | GL_POINT_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glPointSize(1.);

	/* Create pool texture */
	if (!__allegro_gl_pool_texture) {
		glGenTextures(1, &__allegro_gl_pool_texture);
	}

	glBindTexture(GL_TEXTURE_2D, __allegro_gl_pool_texture);
		/* Create a texture without defining the data */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
	allegro_gl_set_projection();

	/* For some reason, ATI Rage Pro isn't able to draw correctly without a
	 * texture bound. So we bind a dummy 1x1 texture to work around the issue.
	 */
	if (allegro_gl_info.is_ati_rage_pro) {
		if (!__allegro_gl_dummy_texture) {
			GLubyte tex[4] = {255, 255, 255, 255};
			glGenTextures(1, &__allegro_gl_dummy_texture);
			glBindTexture(GL_TEXTURE_2D, __allegro_gl_dummy_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
			             GL_RGBA, GL_UNSIGNED_BYTE, tex);
		}
		glBindTexture(GL_TEXTURE_2D, __allegro_gl_dummy_texture);
	}
#ifdef ALLEGRO_MACOSX
	/* MacOSX 10.2.x has a bug: glRasterPos causes a crash (it is used in
	 *'blit'). This stops it happening.
	 */
	glBegin(GL_POINTS);
	glEnd();
#endif
}



/** \ingroup allegro
  * Restores previous OpenGL settings.
  *
  * Restores the OpenGL state that have saved during the last call of
  * allegro_gl_set_allegro_mode().
  *
  * Note that allegro_gl_unset_allegro_mode implicitely calls
  * allegro_gl_unset_projection() so you do not need to do it yourself.
  *
  * \sa	allegro_gl_set_allegro_mode() allegro_gl_set_projection()
  * \sa allegro_gl_unset_projection()
  */
void allegro_gl_unset_allegro_mode(void)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_unset_allegro_mode\n");

	switch(allegro_gl_display_info.vidmem_policy) {
		case AGL_KEEP:
			break;
		case AGL_RELEASE:
			if (__allegro_gl_pool_texture) {
				glDeleteTextures(1, &__allegro_gl_pool_texture);
				__allegro_gl_pool_texture = 0;
			}
			break;
	}
	allegro_gl_unset_projection();
	glPopAttrib();
}



/** \ingroup allegro
  * Prepares for Allegro drawing to the screen.
  *
  * This function sets the OpenGL projection and modelview matrices so
  * that 2D OpenGL coordinates match the usual Allegro coordinate system.
  *
  * OpenGL uses a completely different coordinate system than Allegro. So
  * to be able to use Allegro operations on the screen, you first need
  * to properly set up the OpenGL projection and modelview matrices.
  * AllegroGL provides this set of functions to allow proper alignment of
  * OpenGL coordinates with their Allegro counterparts.
  *
  * Since AllegroGL actually calls OpenGL commands to perform Allegro functions
  * for 2D drawing, some OpenGL capabilities such as texturing or depth testing
  * may interfer with those operations. In order to prevent such inconveniences,
  * you should call allegro_gl_set_allegro_mode() instead of
  * allegro_gl_set_projection()
  *
  * allegro_gl_set_projection() and allegro_gl_unset_projection() are not
  * nestable, which means that you should not call allegro_gl_set_projection()
  * inside another allegro_gl_set_projection() block. Similarly for
  * allegro_gl_unset_projection().
  *
  * Have a look at examp/exalleg.c for an example of
  * combining Allegro drawing commands and OpenGL.
  *
  * \sa allegro_gl_unset_projection() allegro_gl_set_allegro_mode()
  * \sa allegro_gl_unset_allegro_mode()
  */
void allegro_gl_set_projection(void)
{
	GLint v[4];
	AGL_LOG(2, "glvtable.c:allegro_gl_set_projection\n");
	
	/* Setup OpenGL matrices */
	glGetIntegerv(GL_VIEWPORT, &v[0]);
	glMatrixMode(GL_MODELVIEW);
	glGetDoublev(GL_MODELVIEW_MATRIX, allegro_gl_modelview_matrix);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, allegro_gl_projection_matrix);
	glLoadIdentity();
	gluOrtho2D(v[0] - 0.325, v[0] + v[2] - 0.325, v[1] + v[3] - 0.325, v[1] - 0.325);
}



/** \ingroup allegro
  * Restores previously saved projection.
  *
  * This function returns the projection and modelview matrices to their
  * state before the last allegro_gl_set_projection() was called.
  *
  * \sa allegro_gl_set_projection() allegro_gl_set_allegro_mode()
  * \sa allegro_gl_unset_allegro_mode()
  */
void allegro_gl_unset_projection(void)
{
	AGL_LOG(2, "glvtable.c:allegro_gl_unset_projection\n");
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(allegro_gl_projection_matrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(allegro_gl_modelview_matrix);
}



void allegro_gl_memory_blit_between_formats(struct BITMAP *src,
    struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y,
    int width, int height)
{
	AGL_LOG(2, "AGL::blit_between_formats\n");

	/* screen -> memory */
	if (is_screen_bitmap(src)) {
		allegro_gl_screen_blit_to_memory(src, dest, source_x, source_y,
		                                 dest_x, dest_y, width, height);
		return;
	}

	/* video -> memory */
	if (is_video_bitmap(src)) {
		allegro_gl_video_blit_to_memory(src, dest, source_x, source_y,
		                                dest_x, dest_y, width, height);
		return;
	}
	
	/* memory -> screen */
	if (is_screen_bitmap(dest)) {
		allegro_gl_screen_blit_from_memory(src, dest, source_x, source_y,
		                                   dest_x, dest_y, width, height);
		return;
	}

	/* memory -> video */
	if (is_video_bitmap(dest)) {
		allegro_gl_video_blit_from_memory(src, dest, source_x, source_y,
		                                  dest_x, dest_y, width, height);
		return;
	}

	switch(bitmap_color_depth(dest)) {
		#ifdef ALLEGRO_COLOR8
		case 8:
			__blit_between_formats8(src, dest, source_x, source_y,
			                        dest_x, dest_y, width, height);
			return;
		#endif
		#ifdef ALLEGRO_COLOR16
		case 15:
			__blit_between_formats15(src, dest, source_x, source_y,
			                         dest_x, dest_y, width, height);
			return;
		case 16:
			__blit_between_formats16(src, dest, source_x, source_y,
			                         dest_x, dest_y, width, height);
			return;
		#endif
		#ifdef ALLEGRO_COLOR24
		case 24:
			__blit_between_formats24(src, dest, source_x, source_y,
			                         dest_x, dest_y, width, height);
			return;
		#endif
		#ifdef ALLEGRO_COLOR32
		case 32:
			__blit_between_formats32(src, dest, source_x, source_y,
			                         dest_x, dest_y, width, height);
			return;
		#endif
		default:
			TRACE("--== ERROR ==-- AGL::blit_between_formats : %i -> %i bpp\n",
			      bitmap_color_depth(src), bitmap_color_depth(dest));
			return;
	}
}



static void dummy_unwrite_bank(void)
{
}



static GFX_VTABLE allegro_gl_screen_vtable = {
	0,
	0,
	dummy_unwrite_bank,			//void *unwrite_bank;
	NULL,						//AL_METHOD(void, set_clip, (struct BITMAP *bmp));
	allegro_gl_screen_acquire,
	allegro_gl_screen_release,
	NULL,						//AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));
	NULL,						//AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));
	allegro_gl_screen_getpixel,
	allegro_gl_screen_putpixel,
	allegro_gl_screen_vline,
	allegro_gl_screen_hline,
	allegro_gl_screen_hline,
	allegro_gl_screen_line,
	allegro_gl_screen_line,
	allegro_gl_screen_rectfill,
	allegro_gl_screen_triangle,
	allegro_gl_screen_draw_sprite,
	allegro_gl_screen_draw_256_sprite,
	allegro_gl_screen_draw_sprite_v_flip,
	allegro_gl_screen_draw_sprite_h_flip,
	allegro_gl_screen_draw_sprite_vh_flip,
	allegro_gl_screen_draw_trans_rgba_sprite,
	allegro_gl_screen_draw_trans_rgba_sprite,
	NULL,						//AL_METHOD(void, draw_lit_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color));
	allegro_gl_screen_draw_rle_sprite,
	allegro_gl_screen_draw_trans_rgba_rle_sprite,
	allegro_gl_screen_draw_trans_rgba_rle_sprite,
	NULL,						//AL_METHOD(void, draw_lit_rle_sprite, (struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color));
	allegro_gl_screen_draw_character,
	allegro_gl_screen_draw_glyph,
	allegro_gl_screen_blit_from_memory,
	allegro_gl_screen_blit_to_memory,
	NULL,						//AL_METHOD(void, blit_from_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
	NULL,						//AL_METHOD(void, blit_to_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
	allegro_gl_screen_blit_to_self,
	allegro_gl_screen_blit_to_self, /* ..._forward */
	allegro_gl_screen_blit_to_self, /* ..._backward */
	allegro_gl_memory_blit_between_formats,
	allegro_gl_screen_masked_blit,
	allegro_gl_screen_clear_to_color,
	allegro_gl_screen_pivot_scaled_sprite_flip,
	NULL,                       //AL_METHOD(void, do_stretch_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked));
	NULL,                       //AL_METHOD(void, draw_gouraud_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4));
	NULL,                       //AL_METHOD(void, draw_sprite_end, (void));
	NULL,                       //AL_METHOD(void, blit_end, (void));
	allegro_gl_screen_polygon,
	allegro_gl_screen_rect,
	_soft_circle,               //AL_METHOD(void, circle, (struct BITMAP *bmp, int x, int y, int radius, int color));
	_soft_circlefill,           //AL_METHOD(void, circlefill, (struct BITMAP *bmp, int x, int y, int radius, int color));
	_soft_ellipse,              //AL_METHOD(void, ellipse, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
	_soft_ellipsefill,          //AL_METHOD(void, ellipsefill, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
	_soft_arc,                  //AL_METHOD(void, arc, (struct BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color));
	_soft_spline,               //AL_METHOD(void, spline, (struct BITMAP *bmp, AL_CONST int points[8], int color));
	_soft_floodfill,            //AL_METHOD(void, floodfill, (struct BITMAP *bmp, int x, int y, int color));
	allegro_gl_screen_polygon3d,
	allegro_gl_screen_polygon3d_f,
	allegro_gl_screen_triangle3d,
	allegro_gl_screen_triangle3d_f,
	allegro_gl_screen_quad3d,
	allegro_gl_screen_quad3d_f,
	allegro_gl_screen_draw_sprite_ex
};

/** \} */
