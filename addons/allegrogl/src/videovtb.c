/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file videovtb.c
  * \brief video bitmaps (ie. texture rendering) vtable
  *  Some of these don't work correctly or will be very slow.
  */

#include <string.h>
#include <limits.h>

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


#define MASKED_BLIT 1
#define BLIT        2
#define TRANS       3


static GFX_VTABLE allegro_gl_video_vtable;

/* Counter for video bitmaps. screen = 1 */
static int video_bitmap_count = 2;

static int __allegro_gl_video_bitmap_bpp = -1;

extern BITMAP *__agl_drawing_pattern_bmp;
BITMAP *old_pattern = NULL;

void allegro_gl_destroy_video_bitmap(BITMAP *bmp);



static int allegro_gl_make_video_bitmap_helper1(int w, int h, int x, int y,
                                   GLint target, AGL_VIDEO_BITMAP **pvid) {

	int internal_format;
	int bpp;

	if (__allegro_gl_video_bitmap_bpp == -1) {
		bpp = bitmap_color_depth(screen);
	}
	else {
		bpp = __allegro_gl_video_bitmap_bpp;
	}

	(*pvid) = malloc(sizeof(AGL_VIDEO_BITMAP));

	if (!(*pvid))
		return -1;

	memset(*pvid, 0, sizeof(AGL_VIDEO_BITMAP));

	/* Create associated bitmap */
	(*pvid)->memory_copy = create_bitmap_ex(bpp, w, h);
	if (!(*pvid)->memory_copy)
		return -1;
	
	(*pvid)->format = __allegro_gl_get_bitmap_color_format((*pvid)->memory_copy, AGL_TEXTURE_HAS_ALPHA);
	(*pvid)->type = __allegro_gl_get_bitmap_type((*pvid)->memory_copy, 0);
	internal_format = __allegro_gl_get_texture_format_ex((*pvid)->memory_copy, AGL_TEXTURE_HAS_ALPHA);

	(*pvid)->target = target;

	/* Fill in some values in the bitmap to make it act as a subbitmap
	 */
	(*pvid)->width  = w;
	(*pvid)->height = h;
	(*pvid)->x_ofs = x;
	(*pvid)->y_ofs = y;

	/* Make a texture out of it */
	glGenTextures(1, &((*pvid)->tex));
	if (!((*pvid)->tex))
		return -1;

	glEnable((*pvid)->target);
	glBindTexture((*pvid)->target, ((*pvid)->tex));

	glTexImage2D((*pvid)->target, 0, internal_format, w, h,
	             0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	/* By default, use the Allegro filtering mode - ie: Nearest */
	glTexParameteri((*pvid)->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri((*pvid)->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* <mmimica>
	   Clamping removed because we want video bitmaps that are set for
	   patterned drawing to be GL_REPEAT (default wrapping mode). Doesn't
	   seem to break anything.
	*/
#if 0
	/* Clamp to edge */
	{
		GLenum clamp = GL_CLAMP_TO_EDGE;
		if (!allegro_gl_extensions_GL.SGIS_texture_edge_clamp) {
			clamp = GL_CLAMP;
		}
		glTexParameteri((*pvid)->target, GL_TEXTURE_WRAP_S, clamp);
		glTexParameteri((*pvid)->target, GL_TEXTURE_WRAP_T, clamp);
	}
#endif

	glDisable((*pvid)->target);

	if (allegro_gl_extensions_GL.EXT_framebuffer_object) {
		glGenFramebuffersEXT(1, &((*pvid)->fbo));
		if (!(*pvid)->fbo) {
			glDeleteTextures(1, &((*pvid)->tex));
			return -1;
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (*pvid)->fbo);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, (*pvid)->target, (*pvid)->tex, 0);
		if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
			/* Some FBO implementation limitation was hit, will use normal textures. */
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glDeleteFramebuffersEXT(1, &((*pvid)->fbo));
			(*pvid)->fbo = 0;
			return 0;
		}
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	else {
		(*pvid)->fbo = 0;
	}

	return 0;
}



static int allegro_gl_make_video_bitmap_helper0(int w, int h, int x, int y,
                                             	AGL_VIDEO_BITMAP **pvid) {
		
	int is_power_of_2 = (!(w & (w - 1)) && !(h & (h - 1)));
	int texture_rect_available = allegro_gl_extensions_GL.ARB_texture_rectangle
#ifdef ALLEGRO_MACOSX
							 || allegro_gl_extensions_GL.EXT_texture_rectangle
#endif
							 || allegro_gl_extensions_GL.NV_texture_rectangle;
	GLint max_rect_texture_size = 0;

	if (texture_rect_available) {
		glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB, &max_rect_texture_size);
	}

	if (w <= allegro_gl_info.max_texture_size &&
		h <= allegro_gl_info.max_texture_size) {
		if (allegro_gl_extensions_GL.ARB_texture_non_power_of_two ||
			is_power_of_2) {
			if (allegro_gl_make_video_bitmap_helper1(w, h, x, y,
											GL_TEXTURE_2D, pvid)) {
				return -1;
			}
		}
		else if (texture_rect_available &&
				 w <= max_rect_texture_size &&
				 h <= max_rect_texture_size) {
			if (allegro_gl_make_video_bitmap_helper1(w, h, x, y,
											GL_TEXTURE_RECTANGLE_ARB, pvid)) {
				return -1;
			}
		}
		else {
			/* NPO2 textures are not suppored by the driver in any way.
			 * Split the bitmap into smaller POT bitmaps. */
			const unsigned int BITS = sizeof(int) * CHAR_BIT;
			unsigned int i, j;
			unsigned int w1, h1;
			unsigned int x1, y1;
			unsigned int p1, p2;

			/* Find the POTs using bits. */
			y1 = 0;
			for (i = 0; i < BITS; i++) {
				p1 = 1 << i;
				if (h & p1)
					h1 = p1;
				else
					continue;

				x1 = 0;
				for (j = 0; j < BITS; j++) {
					p2 = 1 << j;
					if (w & p2)
						w1 = p2;
					else
						continue;

					if (allegro_gl_make_video_bitmap_helper0(w1, h1, x + x1,
														y + y1, pvid)) {
						return -1;
					}
					do {
						pvid = &((*pvid)->next);
					} while (*pvid);

					x1 += w1;
				}

				y1 += h1;
			}
		}
	}
	else {
		/* Texture is too big to fit. Split it in 4 and try again. */
		int w1, w2, h1, h2;

		w2 = w / 2;
		w1 = w - w2;

		h2 = h / 2;
		h1 = h - h2;

		/* Even a 1x1 texture didn't work? Bail*/
		if (!w2 && !h2) {
			return -1;
		}

		/* Top-left corner */
		if (allegro_gl_make_video_bitmap_helper0(w1, h1, x, y, pvid)) {
			return -1;
		}
		do {
			pvid = &((*pvid)->next);
		} while (*pvid);

		/* Top-right corner */
		if (w2) {
			if (allegro_gl_make_video_bitmap_helper0(w2, h1, x + w1, y, pvid)) {
				return -1;
			}
			do {
				pvid = &((*pvid)->next);
			} while (*pvid);
		}
		/* Bottom-left corner */
		if (h2) {
			if (allegro_gl_make_video_bitmap_helper0(w1, h2, x, y + h1, pvid)) {
				return -1;
			}
			do {
				pvid = &((*pvid)->next);
			} while (*pvid);
		}
		/* Bottom-right corner */
		if (w2 && h2) {
			if (allegro_gl_make_video_bitmap_helper0(w2, h2, x + w1, y + h1, pvid)) {
				return -1;
			}
			do {
				pvid = &((*pvid)->next);
			} while (*pvid);
		}
	}

	return 0;
}



/* Will make a (potentially piece-wise) texture out of the specified bitmap
 * Source -must- be a memory bitmap or memory subbitmap (created by Allegro
 * only).
 *
 */
static BITMAP *allegro_gl_make_video_bitmap(BITMAP *bmp) {
	
	/* Grab a pointer to the bitmap data to patch */
	void *ptr = &bmp->extra;
	AGL_VIDEO_BITMAP **pvid = (AGL_VIDEO_BITMAP**)ptr;
	
	/* Convert bitmap to texture */
	if (allegro_gl_make_video_bitmap_helper0(bmp->w, bmp->h, 0, 0, pvid)) {
		goto agl_error;
	}

	return bmp;
	
agl_error:
	allegro_gl_destroy_video_bitmap(bmp);
	return NULL;
}



/* void allegro_gl_destroy_video_bitmap(BITMAP *bmp) */
/** destroy_video_bitmap() overload.
  * Will destroy the video bitmap. You mustn't use the BITMAP pointer after
  * calling this function.
  */
void allegro_gl_destroy_video_bitmap(BITMAP *bmp) {

	AGL_VIDEO_BITMAP *vid = bmp ? bmp->extra : NULL, *next;
	
	if (!bmp)
		return;
	
	while (vid) {
		if (vid->memory_copy)
			destroy_bitmap(vid->memory_copy);
	
		if (vid->tex)
			glDeleteTextures(1, &vid->tex);

		if (vid->fbo)
			glDeleteFramebuffersEXT(1, &vid->fbo);
								
		next = vid->next;
		free(vid);
		vid = next;
	}

	free(bmp->vtable);
	free(bmp);
	
	return;
}



/* BITMAP *allegro_gl_create_video_bitmap(int w, int h) */
/** create_video_bitmap() overload.
  * This function will create a video bitmap using texture memory.
  * Video bitmaps do not currently share space with the frame buffer (screen).
  * Video bitmaps are lost when switching screen modes.
  */
BITMAP *allegro_gl_create_video_bitmap(int w, int h) {
	GFX_VTABLE *vtable;
	BITMAP *bitmap;

	bitmap = malloc(sizeof(BITMAP) + sizeof(char *));
	
	if (!bitmap)
		return NULL;

	bitmap->dat = NULL;
	bitmap->w = bitmap->cr = w;
	bitmap->h = bitmap->cb = h;
	bitmap->clip = TRUE;
	bitmap->cl = bitmap->ct = 0;
	bitmap->write_bank = bitmap->read_bank = NULL;
	/* We should keep tracks of allocated bitmaps for the ref counter */
	bitmap->id = BMP_ID_VIDEO | video_bitmap_count;
	bitmap->extra = NULL;
	bitmap->x_ofs = 0;
	bitmap->y_ofs = 0;
	bitmap->seg = _default_ds();
	bitmap->line[0] = NULL;
	bitmap->vtable = NULL;

	if (!allegro_gl_make_video_bitmap(bitmap)) {
		return NULL;
	}
	video_bitmap_count++;
	
	/* XXX <rohannessian> We ought to leave the Allegro values intact
	 * Avoids bad interaction with correct Allegro programs.
	 */
	vtable = malloc(sizeof(struct GFX_VTABLE));
	*vtable = allegro_gl_video_vtable;
	if (__allegro_gl_video_bitmap_bpp == -1) {
		vtable->color_depth = bitmap_color_depth(screen);
	}
	else {
		vtable->color_depth = __allegro_gl_video_bitmap_bpp;
	}
	switch (vtable->color_depth) {
		case 8:
			vtable->mask_color = MASK_COLOR_8;
		break;
		case 15:
			vtable->mask_color = MASK_COLOR_15;
		break;
		case 16:
			vtable->mask_color = MASK_COLOR_16;
		break;
		case 24:
			vtable->mask_color = MASK_COLOR_24;
		break;
		case 32:
			vtable->mask_color = MASK_COLOR_32;
		break;
	}
	bitmap->vtable = vtable;

	return bitmap;
}



/* allegro_gl_set_video_bitmap_color_depth(int bpp) */
/** \ingroup bitmap
 *  Sets the color depth you'd like AllegroGL to use for video bitmaps.
 *  This function lets you create video bitmaps of any color depth using
 *  allegro's create_video_bitmap(). You can create a RGBA8 bitmap and take
 *  advantage of hardware accelerated blending, no matter what the current color
 *  depth is.
 *  This call will affect all subsequent calls to create_video_bitmap().
 *
 *  Default value for the video bitmap color depth if the color depth of the
 *  screen bitmap. To revert to the default color depth, pass -1.
 *
 *  \param bpp Color depth in btis per pixel.
 *  \return The previous value of the video bitmap color depth.
 */
GLint allegro_gl_set_video_bitmap_color_depth(int bpp) {
	GLint old_val = __allegro_gl_video_bitmap_bpp;
	__allegro_gl_video_bitmap_bpp = bpp;
	return old_val;
}


/* static void allegro_gl_video_acquire(struct BITMAP *bmp) */
/** \ingroup glvtable
  * acquire_bitmap(bmp) overload.
  * This doesn't do anything, since OpenGL textures
  * doesn't need locking. You don't need to call this function
  * in your program.
  */
static void allegro_gl_video_acquire(struct BITMAP *bmp) {}



/* static void allegro_gl_video_release(struct BITMAP *bmp) */
/** \ingroup glvtable
  * release_bitmap(bmp) overload.
  * This doesn't do anything, since OpenGL textures
  * doesn't need locking. You don't need to call this function
  * in your program.
  */
static void allegro_gl_video_release(struct BITMAP *bmp) {}



static void set_drawing_pattern(void)
{
	if (_drawing_pattern && !is_memory_bitmap(_drawing_pattern)) {
		old_pattern = _drawing_pattern;
		drawing_mode(_drawing_mode, __agl_drawing_pattern_bmp,
					 _drawing_x_anchor, _drawing_y_anchor);
	}
}



static void unset_drawing_pattern(void)
{
	if (old_pattern) {
		drawing_mode(_drawing_mode, old_pattern,
					 _drawing_x_anchor, _drawing_y_anchor);
		old_pattern = NULL;
	}
}



static int allegro_gl_video_getpixel(struct BITMAP *bmp, int x, int y)
{
	int pix = -1;
	AGL_VIDEO_BITMAP *vid;
	AGL_LOG(2, "glvtable.c:allegro_gl_screen_getpixel\n");	
	
	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}
	if (x < bmp->cl || x >= bmp->cr || y < bmp->ct || y >= bmp->cb) {
		return -1;
	}

	vid = bmp->extra;
	
	while (vid) {
		if (vid->x_ofs <= x && vid->y_ofs <= y
		 && vid->x_ofs + vid->memory_copy->w > x
		 && vid->y_ofs + vid->memory_copy->h > y) {
			
			pix = getpixel(vid->memory_copy, x - vid->x_ofs, y - vid->y_ofs);
			break;
		}
		vid = vid->next;
	}
	
	if (pix != -1) {
		return pix;
	}
	
	return -1;
}



static void update_texture_memory(AGL_VIDEO_BITMAP *vid, int x1, int y1,
                                  int x2, int y2) {
	GLint saved_row_length;
	GLint saved_alignment;
	GLint type;
	GLint format;
	int bpp;
	BITMAP *temp = NULL;
	BITMAP *vbmp = vid->memory_copy;;

	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &saved_row_length);
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &saved_alignment);

	bpp = BYTES_PER_PIXEL(bitmap_color_depth(vid->memory_copy));
	format = vid->format;
	type = vid->type;

	glColor4ub(255, 255, 255, 255);

	/* If packed pixels (or GL 1.2) isn't supported, then we need to convert
	 * the bitmap into something GL can understand - 24-bpp should do it.
	 */
	if (!allegro_gl_extensions_GL.EXT_packed_pixels
		                                  && bitmap_color_depth(vbmp) < 24) {
		temp = create_bitmap_ex(24, vbmp->w, vbmp->h);
		if (!temp)
			return;

		blit(vbmp, temp, 0, 0, 0, 0, temp->w, temp->h);
		vbmp = temp;
		bpp = BYTES_PER_PIXEL(bitmap_color_depth(vbmp));
		format = __allegro_gl_get_bitmap_color_format(vbmp, 0);
		type = __allegro_gl_get_bitmap_type(vbmp, 0);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,
				  vbmp->h > 1
				 ? (vbmp->line[1] - vbmp->line[0]) / bpp
				 : vbmp->w);

	glEnable(vid->target);
	glBindTexture(vid->target, vid->tex);
	glTexSubImage2D(vid->target, 0,
		x1, y1, x2 - x1 + 1, y2 - y1 + 1, format,
		type, vbmp->line[y1] + x1 * bpp);
	glBindTexture(vid->target, 0);
	glDisable(vid->target);

	if (temp)
		destroy_bitmap(temp);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, saved_row_length);
	glPixelStorei(GL_UNPACK_ALIGNMENT, saved_alignment);
}



static void allegro_gl_video_putpixel(struct BITMAP *bmp, int x, int y,
                                      int color) {
	AGL_VIDEO_BITMAP *vid;

	if (is_sub_bitmap(bmp)) {
		x += bmp->x_ofs;
		y += bmp->y_ofs;
	}
	if (x < bmp->cl || x >= bmp->cr || y < bmp->ct || y >= bmp->cb) {
		return;
	}

	vid = bmp->extra;

	while (vid) {
		if (vid->x_ofs <= x && vid->y_ofs <= y
		 && vid->x_ofs + vid->memory_copy->w > x
		 && vid->y_ofs + vid->memory_copy->h > y) {

			set_drawing_pattern();
			putpixel(vid->memory_copy, x - vid->x_ofs, y - vid->y_ofs, color);
			unset_drawing_pattern();
			update_texture_memory(vid, x - vid->x_ofs, y - vid->y_ofs, x - vid->x_ofs, y - vid->y_ofs);
			break;
		}
		vid = vid->next;
	}
	
	return;
}



static void allegro_gl_video_vline(BITMAP *bmp, int x, int y1, int y2,
                                   int color) {

	AGL_VIDEO_BITMAP *vid;
	
	AGL_LOG(2, "glvtable.c:allegro_gl_video_vline\n");
	vid = bmp->extra;
	
	if (is_sub_bitmap(bmp)) {
		x  += bmp->x_ofs;
		y1 += bmp->y_ofs;
		y2 += bmp->y_ofs;
	}
	if (x < bmp->cl || x >= bmp->cr) {
		return;
	}
	
	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}

	if (y1 < bmp->ct) {
		y1 = bmp->ct;
	}
	if (y2 >= bmp->cb) {
		y2 = bmp->cb - 1;
	}

	while (vid) {
		BITMAP *vbmp = vid->memory_copy;
		
		int _y1, _y2, _x;
		if (vid->x_ofs > x || vid->y_ofs > y2
		 || vid->x_ofs + vbmp->w <= x
		 || vid->y_ofs + vbmp->h <= y1) {
			
			vid = vid->next;
			continue;
		}
		
		_y1 = MAX(y1, vid->y_ofs) - vid->y_ofs;
		_y2 = MIN(y2, vid->y_ofs + vbmp->h - 1)	- vid->y_ofs;
		_x = x - vid->x_ofs;

		set_drawing_pattern();
		vline(vbmp, _x, _y1, _y2, color);
		unset_drawing_pattern();
		update_texture_memory(vid, _x, _y1, _x, _y2);

		vid = vid->next;
	}
	
	return;
}



static void allegro_gl_video_hline(BITMAP *bmp, int x1, int y, int x2,
                                   int color) {

	AGL_VIDEO_BITMAP *vid;
	
	AGL_LOG(2, "glvtable.c:allegro_gl_video_hline\n");
	vid = bmp->extra;

	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		x2 += bmp->x_ofs;
		y  += bmp->y_ofs;
	}

	if (y < bmp->ct || y >= bmp->cb) {
		return;
	}
	
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}

	if (x1 < bmp->cl) {
		x1 = bmp->cl;
	}
	if (x2 >= bmp->cr) {
		x2 = bmp->cr - 1;
	}

	while (vid) {
		BITMAP *vbmp = vid->memory_copy;
		
		int _x1, _x2, _y;
		if (vid->y_ofs > y || vid->x_ofs > x2
		 || vid->x_ofs + vbmp->w <= x1
		 || vid->y_ofs + vbmp->h <= y) {
			
			vid = vid->next;
			continue;
		}
		
		_x1 = MAX(x1, vid->x_ofs) - vid->x_ofs;
		_x2 = MIN(x2, vid->x_ofs + vbmp->w - 1)	- vid->x_ofs;
		_y = y - vid->y_ofs;

		set_drawing_pattern();
		hline(vbmp, _x1, _y, _x2, color);	
		unset_drawing_pattern();
		update_texture_memory(vid, _x1, _y, _x2, _y);

		vid = vid->next;
	}
	
	return;
}



static void allegro_gl_video_line(struct BITMAP *bmp, int x1, int y1, int x2,
                                  int y2, int color) {
	
	/* Note: very very slow */						
	do_line(bmp, x1, y1, x2, y2, color, allegro_gl_video_putpixel);
	
	return;
}
	


static void allegro_gl_video_rectfill(struct BITMAP *bmp, int x1, int y1,
                                      int x2, int y2, int color) {

	AGL_VIDEO_BITMAP *vid;

	AGL_LOG(2, "glvtable.c:allegro_gl_video_rectfill\n");
	vid = bmp->extra;

	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		x2 += bmp->x_ofs;
		y1 += bmp->y_ofs;
		y2 += bmp->y_ofs;
	}
	
	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}

	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}

	if (x1 < bmp->cl) {
		x1 = bmp->cl;
	}
	if (x2 > bmp->cr) {
		x2 = bmp->cr;
	}
	if (y1 < bmp->ct) {
		y1 = bmp->ct;
	}
	if (y1 > bmp->cb) {
		y1 = bmp->cb;
	}

	while (vid) {
		BITMAP *vbmp = vid->memory_copy;
		
		int _y1, _y2, _x1, _x2;
		if (vid->x_ofs > x2 || vid->y_ofs > y2
		 || vid->x_ofs + vbmp->w <= x1
		 || vid->y_ofs + vbmp->h <= y1) {
			
			vid = vid->next;
			continue;
		}
		
		_y1 = MAX(y1, vid->y_ofs) - vid->y_ofs;
		_y2 = MIN(y2, vid->y_ofs + vbmp->h - 1)	- vid->y_ofs;
		_x1 = MAX(x1, vid->x_ofs) - vid->x_ofs;
		_x2 = MIN(x2, vid->x_ofs + vbmp->w - 1)	- vid->x_ofs;

		set_drawing_pattern();
		rectfill(vbmp, _x1, _y1, _x2, _y2, color);
		unset_drawing_pattern();

		update_texture_memory(vid, _x1, _y1, _x2, _y2);

		vid = vid->next;
	}

	return;
}


static void allegro_gl_video_triangle(struct BITMAP *bmp, int x1, int y1,
                                      int x2, int y2, int x3, int y3, int color)
{	
	AGL_VIDEO_BITMAP *vid;
	int min_y, max_y, min_x, max_x;

	AGL_LOG(2, "glvtable.c:allegro_gl_video_triangle\n");
	vid = bmp->extra;

	if (is_sub_bitmap(bmp)) {
		x1 += bmp->x_ofs;
		x2 += bmp->x_ofs;
		x3 += bmp->x_ofs;
		y1 += bmp->y_ofs;
		y2 += bmp->y_ofs;
		y3 += bmp->y_ofs;
	}

	min_y = MIN(y1, MIN(y2, y3));
	min_x = MIN(x1, MIN(x2, x3));
	max_y = MAX(y1, MAX(y2, y3));
	max_x = MAX(x1, MAX(x2, x3));

	while (vid) {
		BITMAP *vbmp = vid->memory_copy;
		
		int _y1, _y2, _x1, _x2, _x3, _y3;
		if (vid->x_ofs > max_x || vid->y_ofs > max_y
		 || vid->x_ofs + vbmp->w <= min_x
		 || vid->y_ofs + vbmp->h <= min_y) {
			
			vid = vid->next;
			continue;
		}
		
		_y1 = y1 - vid->y_ofs;
		_y2 = y2 - vid->y_ofs;
		_y3 = y3 - vid->y_ofs;
		_x1 = x1 - vid->x_ofs;
		_x2 = x2 - vid->x_ofs;
		_x3 = x3 - vid->x_ofs;

		set_clip_rect(vbmp, bmp->cl - vid->x_ofs, bmp->ct - vid->y_ofs,
		                    bmp->cr - vid->x_ofs - 1, bmp->cb - vid->y_ofs - 1);

		set_drawing_pattern();

		triangle(vbmp, _x1, _y1, _x2, _y2, _x3, _y3, color);

		unset_drawing_pattern();

		set_clip_rect(vbmp, 0, 0, vbmp->w - 1, vbmp->h - 1);

		/* Not quite the minimal rectangle occupied by the triangle, but
		* pretty close */
		_y1 = MAX(0, min_y - vid->y_ofs);
		_y2 = MIN(vbmp->h - 1, max_y - vid->y_ofs);
		_x1 = MAX(0, min_x - vid->x_ofs);
		_x2 = MIN(vbmp->w - 1, max_x - vid->x_ofs);

		update_texture_memory(vid, _x1, _y1, _x2, _y2);

		vid = vid->next;
	}
}



static void allegro_gl_video_blit_from_memory_ex(BITMAP *source, BITMAP *dest,
					int source_x, int source_y, int dest_x, int dest_y,
	 				int width, int height, int draw_type) {

	AGL_VIDEO_BITMAP *vid;
	BITMAP *dest_parent = dest;
	
	if (is_sub_bitmap (dest)) {
	   dest_x += dest->x_ofs;
	   dest_y += dest->y_ofs;
	   while (dest_parent->id & BMP_ID_SUB)
	      dest_parent = (BITMAP *)dest_parent->extra;
	}

	if (dest_x < dest->cl) {
		dest_x = dest->cl;
	}
	if (dest_y < dest->ct) {
		dest_y = dest->ct;
	}
	if (dest_x + width >= dest->cr) {
		width = dest->cr - dest_x;
	}
	if (dest_y + height >= dest->cb) {
		height = dest->cb - dest_y;
	}
	if (width < 1 || height < 1) {
		return;
	}
	
	vid = dest_parent->extra;

	while (vid) {
		BITMAP *vbmp = vid->memory_copy;

		int _x, _y, _w, _h, _sx, _sy;
		if (vid->x_ofs >= dest_x + width || vid->y_ofs >= dest_y + height
		 || vid->x_ofs + vbmp->w <= dest_x
		 || vid->y_ofs + vbmp->h <= dest_y) {
			
			vid = vid->next;
			continue;
		}

		_x = MAX (vid->x_ofs, dest_x) - vid->x_ofs;
		_w = MIN (vid->x_ofs + vbmp->w, dest_x + width)
		   - vid->x_ofs - _x;
		_y = MAX (vid->y_ofs, dest_y) - vid->y_ofs;
		_h = MIN (vid->y_ofs + vbmp->h, dest_y + height)
		   - vid->y_ofs - _y;

		_sx = source_x + vid->x_ofs + _x - dest_x;
		_sy = source_y + vid->y_ofs + _y - dest_y;

		if (draw_type == BLIT) {
			blit(source, vbmp, _sx, _sy, _x, _y, _w, _h);
		}
		else if (draw_type == MASKED_BLIT) {
			masked_blit(source, vbmp, _sx, _sy, _x, _y, _w, _h);
		}
		else if (draw_type == TRANS) {
			BITMAP *clip = create_sub_bitmap(source, _sx, _sy, _w, _h);
			if (!clip)
				return;
			draw_trans_sprite(vbmp, clip, _x, _y);
			destroy_bitmap(clip);
		}

		update_texture_memory(vid, _x, _y, _x + _w - 1, _y + _h - 1);

		vid = vid->next;
	}

	return;	
}


void allegro_gl_video_blit_from_memory(BITMAP *source, BITMAP *dest,
					int source_x, int source_y, int dest_x, int dest_y,
	 				int width, int height) {

	allegro_gl_video_blit_from_memory_ex(source, dest, source_x, source_y,
										 dest_x, dest_y, width, height, BLIT);
	return;
}



void allegro_gl_video_blit_to_memory(struct BITMAP *source, struct BITMAP *dest,
                         int source_x, int source_y, int dest_x, int dest_y,
                         int width, int height) {

	AGL_VIDEO_BITMAP *vid;
	BITMAP *source_parent = source;
	
	AGL_LOG(2, "glvtable.c:allegro_gl_video_blit_to_memory\n");
	
	if (is_sub_bitmap(source)) {
	   source_x += source->x_ofs;
	   source_y += source->y_ofs;
	   while (source_parent->id & BMP_ID_SUB)
	      source_parent = (BITMAP *)source_parent->extra;
	}

	vid = source_parent->extra;
	
	while (vid) {
		BITMAP *vbmp = vid->memory_copy;
		int x, y, dx, dy, w, h;

		x = MAX(source_x, vid->x_ofs) - vid->x_ofs;
		y = MAX(source_y, vid->y_ofs) - vid->y_ofs;
		w = MIN(vid->x_ofs + vbmp->w, source_x + width) - vid->x_ofs;
		h = MIN(vid->y_ofs + vbmp->h, source_y + height) - vid->y_ofs;
		dx = MAX(0, vid->x_ofs - source_x) + dest_x;
		dy = MAX(0, vid->y_ofs - source_y) + dest_y;

		blit(vbmp, dest, x, y, dx, dy, w, h);
	
		vid = vid->next;
	}

	return;	
}



/* Just like allegro_gl_video_blit_from_memory(), except that draws only to the
 * memory copy.
 */
static void __video_update_memory_copy(BITMAP *source, BITMAP *dest,
							int source_x, int source_y, int dest_x, int dest_y,
							int width, int height, int draw_type) {
	AGL_VIDEO_BITMAP *vid;
	BITMAP *dest_parent = dest;
	
	if (is_sub_bitmap (dest)) {
	   dest_x += dest->x_ofs;
	   dest_y += dest->y_ofs;
	   while (dest_parent->id & BMP_ID_SUB)
	      dest_parent = (BITMAP *)dest_parent->extra;
	}

	if (dest_x < dest->cl) {
		dest_x = dest->cl;
	}
	if (dest_y < dest->ct) {
		dest_y = dest->ct;
	}
	if (dest_x + width >= dest->cr) {
		width = dest->cr - dest_x;
	}
	if (dest_y + height >= dest->cb) {
		height = dest->cb - dest_y;
	}
	if (width < 1 || height < 1) {
		return;
	}
	
	vid = dest_parent->extra;

	while (vid) {
		int sx, sy;
		BITMAP *vbmp = vid->memory_copy;

		int dx, dy, w, h;
		if (vid->x_ofs >= dest_x + width || vid->y_ofs >= dest_y + height
		 || vid->x_ofs + vbmp->w <= dest_x
		 || vid->y_ofs + vbmp->h <= dest_y) {
			
			vid = vid->next;
			continue;
		}

		dx = MAX (vid->x_ofs, dest_x) - vid->x_ofs;
		w = MIN (vid->x_ofs + vbmp->w, dest_x + width)
		   - vid->x_ofs - dx;
		dy = MAX (vid->y_ofs, dest_y) - vid->y_ofs;
		h = MIN (vid->y_ofs + vbmp->h, dest_y + height)
		   - vid->y_ofs - dy;

		sx = source_x + vid->x_ofs + dx - dest_x;
		sy = source_y + vid->y_ofs + dy - dest_y;

		if (draw_type == MASKED_BLIT) {
			masked_blit(source, vbmp, sx, sy, dx, dy, w, h);
		}
		else if (draw_type == BLIT) {
			blit(source, vbmp, sx, sy, dx, dy, w, h);
		}
		else if (draw_type == TRANS) {
			BITMAP *clip = create_sub_bitmap(source, sx, sy, w, h);
			if (!clip)
				return;
			draw_trans_sprite(vbmp, clip, dx, dy);
			destroy_bitmap(clip);
		}

		vid = vid->next;
	}
	
	return;
}


#define FOR_EACH_TEXTURE_FRAGMENT(  \
	screen_blit_from_vid,           /* used when dest is FBO to blit to texture 
									   memory from video bitmap */                \
	screen_blit_from_mem,           /* used when dest is FBO to blit to texture
									   memory from memory bitmap */               \
	mem_copy_blit_from_vid,         /* used to update the memory copy of the
									   dest from the source video bitmap */       \
	mem_copy_blit_from_mem,         /* used to update the memory copy of the
									   dest from the source memory bitmap */      \
	vid_and_mem_copy_blit_from_vid, /* used when dest is not FBO, draws to both
									   memory copy and texute memory of the dest,
									   from video bitmap source*/                 \
	vid_and_mem_copy_blit_from_mem) /* used when dest is not FBO, draws to both
									   memory copy and texute memory of the dest,
									   from memory bitmap source */               \
{                                                          \
	int used_fbo = FALSE;                                  \
	AGL_VIDEO_BITMAP *vid;                                 \
                                                           \
	vid = dest->extra;                                     \
	if (vid->fbo) {                                        \
		int sx, sy;                                        \
		int dx, dy;                                        \
		int w, h;                                          \
                                                           \
		static GLint v[4];                                 \
		static double allegro_gl_projection_matrix[16];    \
		static double allegro_gl_modelview_matrix[16];     \
                                                           \
		glGetIntegerv(GL_VIEWPORT, &v[0]);                 \
		glMatrixMode(GL_MODELVIEW);                        \
		glGetDoublev(GL_MODELVIEW_MATRIX, allegro_gl_modelview_matrix);   \
		glMatrixMode(GL_PROJECTION);                                      \
		glGetDoublev(GL_PROJECTION_MATRIX, allegro_gl_projection_matrix); \
                                                                          \
		while (vid) {                                                     \
			if (dest_x >= vid->x_ofs + vid->memory_copy->w ||             \
				dest_y >= vid->y_ofs + vid->memory_copy->h ||             \
				vid->x_ofs >= dest_x + width ||          \
				vid->y_ofs >= dest_y + height) {         \
				vid = vid->next;                         \
				continue;                                \
			}                                            \
                                                         \
			dx = MAX(vid->x_ofs, dest_x) - vid->x_ofs;                  \
			w = MIN(vid->x_ofs + vid->memory_copy->w, dest_x + width)   \
			  - vid->x_ofs - dx;                                        \
			dy = MAX(vid->y_ofs, dest_y) - vid->y_ofs;                  \
			h = MIN(vid->y_ofs + vid->memory_copy->h, dest_y + height)  \
			  - vid->y_ofs - dy;                                        \
                                                                        \
			sx = source_x + vid->x_ofs + dx - dest_x;                   \
			sy = source_y + vid->y_ofs + dy - dest_y;                   \
                                                                        \
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vid->fbo);         \
                                                                        \
			glViewport(0, 0, vid->memory_copy->w, vid->memory_copy->h); \
			glMatrixMode(GL_PROJECTION);                                \
			glLoadIdentity();                                           \
			gluOrtho2D(0, vid->memory_copy->w, 0, vid->memory_copy->h); \
			glMatrixMode(GL_MODELVIEW);                                 \
                                             \
			if (is_memory_bitmap(source)) {  \
				screen_blit_from_mem;        \
			}                                \
			else {                           \
				screen_blit_from_vid;        \
			}                                \
                                \
			vid = vid->next;    \
		}                       \
                                \
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); \
                                                     \
		glViewport(v[0], v[1], v[2], v[3]);          \
		glMatrixMode(GL_PROJECTION);                 \
		glLoadMatrixd(allegro_gl_projection_matrix); \
		glMatrixMode(GL_MODELVIEW);                  \
		glLoadMatrixd(allegro_gl_modelview_matrix);  \
                                          \
		used_fbo = TRUE;                  \
	}                                     \
                                          \
	if (is_video_bitmap(source)) {        \
		int sx, sy;      \
		int dx, dy;      \
		int w, h;        \
                         \
		vid = source->extra;   \
                               \
		while (vid) {          \
			if (source_x >= vid->x_ofs + vid->memory_copy->w ||  \
				source_y >= vid->y_ofs + vid->memory_copy->h ||  \
				vid->x_ofs >= source_x + width ||                \
				vid->y_ofs >= source_y + height) {               \
				vid = vid->next;                                 \
				continue;                                        \
			}                                                    \
                                                                 \
			sx = MAX(vid->x_ofs, source_x) - vid->x_ofs;         \
			w = MIN(vid->x_ofs + vid->memory_copy->w, source_x + width)  \
			  - vid->x_ofs - sx;                                         \
			sy = MAX(vid->y_ofs, source_y) - vid->y_ofs;                 \
			h = MIN(vid->y_ofs + vid->memory_copy->h, source_y + height) \
			  - vid->y_ofs - sy;                                         \
                                                                         \
			dx = dest_x + vid->x_ofs + sx - source_x;  \
			dy = dest_y + vid->y_ofs + sy - source_y;  \
                                                       \
			if (used_fbo) {                            \
				mem_copy_blit_from_vid;                \
			}                                          \
			else {                                     \
				vid_and_mem_copy_blit_from_vid;        \
			}                                  \
                                               \
			vid = vid->next;                   \
		}                                      \
	}                                          \
	else if (is_memory_bitmap(source)) {       \
		if (used_fbo) {                        \
			mem_copy_blit_from_mem;            \
		}                                      \
		else {                                 \
			vid_and_mem_copy_blit_from_mem;    \
		}                                      \
	}                                          \
}


/* allegro_gl_video_blit_to_self:
 * blit() overload for video -> video blits
 */
void allegro_gl_video_blit_to_self(struct BITMAP *source, struct BITMAP *dest,
	int source_x, int source_y, int dest_x, int dest_y, int width, int height) {

	FOR_EACH_TEXTURE_FRAGMENT (
		allegro_gl_screen_blit_to_self(source, screen, sx, sy, dx, dy, w, h),
		allegro_gl_screen_blit_to_self(source, screen, sx, sy, dx, dy, w, h),
		__video_update_memory_copy(vid->memory_copy, dest, sx, sy, dx, dy, w, h, BLIT),
		__video_update_memory_copy(source, dest, source_x, source_y, dest_x, dest_y, width, height, BLIT),
		allegro_gl_video_blit_from_memory(vid->memory_copy, dest, sx, sy, dx, dy, w, h),
		allegro_gl_video_blit_from_memory(source, dest, source_x, source_y, dest_x, dest_y, width, height)
	)
}


static void do_masked_blit_video(struct BITMAP *source, struct BITMAP *dest,
			int source_x, int source_y, int dest_x, int dest_y,
			int width, int height, int flip_dir, int blit_type) {

	FOR_EACH_TEXTURE_FRAGMENT (
		do_masked_blit_screen(source, screen, sx, sy, dx, dy, w, h, flip_dir, blit_type),
		do_masked_blit_screen(source, screen, sx, sy, dx, dy, w, h, flip_dir, blit_type),
		__video_update_memory_copy(vid->memory_copy, dest, sx, sy, dx, dy, w, h, MASKED_BLIT),
		__video_update_memory_copy(source, dest, source_x, source_y, dest_x, dest_y, width, height, MASKED_BLIT),
		allegro_gl_video_blit_from_memory_ex(vid->memory_copy, dest, sx, sy, dx, dy, w, h, MASKED_BLIT),
		allegro_gl_video_blit_from_memory_ex(source, dest, source_x, source_y, dest_x, dest_y, width, height, MASKED_BLIT)
	)
}


/* allegro_gl_video_masked_blit:
 * masked_blit() overload for video -> video masked blits
 */
static void allegro_gl_video_masked_blit(struct BITMAP *source,
				struct BITMAP *dest, int source_x, int source_y,
				int dest_x,	int dest_y, int width, int height) {
	do_masked_blit_video(source, dest, source_x, source_y, dest_x, dest_y,
						 width, height, FALSE, AGL_REGULAR_BMP | AGL_NO_ROTATION);

	return;
}


/* allegro_gl_video_draw_sprite:
 * draw_sprite() overload for video -> video sprite drawing
 */
static void allegro_gl_video_draw_sprite(struct BITMAP *bmp,
						struct BITMAP *sprite, int x, int y) {

	do_masked_blit_video(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
						 FALSE, AGL_NO_ROTATION);

	return;
}


/* allegro_gl_video_draw_sprite_v_flip:
 * draw_sprite_v_flip() overload for video -> video blits
 * FIXME: Broken if the bitmap was split into multiple textures.
 * FIXME: Doesn't apply rotation and scale to the memory copy
 */
static void allegro_gl_video_draw_sprite_v_flip(struct BITMAP *bmp,
						struct BITMAP *sprite, int x, int y) {

	do_masked_blit_video(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
						 AGL_V_FLIP, AGL_NO_ROTATION);

	return;
}


/* allegro_gl_video_draw_sprite_h_flip:
 * draw_sprite_v_flip() overload for video -> video blits
 * FIXME: Broken if the bitmap was split into multiple textures.
 * FIXME: Doesn't apply rotation and scale to the memory copy
 */
static void allegro_gl_video_draw_sprite_h_flip(struct BITMAP *bmp,
						struct BITMAP *sprite, int x, int y) {

	do_masked_blit_video(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
						 AGL_H_FLIP, AGL_NO_ROTATION);

	return;
}


/* allegro_gl_video_draw_sprite_vh_flip:
 * draw_sprite_vh_flip() overload for video -> video blits
 * FIXME: Broken if the bitmap was split into multiple textures.
 * FIXME: Doesn't apply rotation and scale to the memory copy
 */
static void allegro_gl_video_draw_sprite_vh_flip(struct BITMAP *bmp,
						struct BITMAP *sprite, int x, int y) {

	do_masked_blit_video(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h,
						 AGL_V_FLIP | AGL_H_FLIP, AGL_NO_ROTATION);

	return;
}


/* allegro_gl_video_pivot_scaled_sprite_flip:
 * FIXME: Broken if the bitmap was split into multiple textures.
 * FIXME: Doesn't apply rotation and scale to the memory copy
 * FIXME: Doesn't work for when FBO is not available.
 */
static void allegro_gl_video_pivot_scaled_sprite_flip(struct BITMAP *bmp,
			struct BITMAP *sprite, fixed x, fixed y, fixed cx, fixed cy,
			fixed angle, fixed scale, int v_flip) {
	double dscale = fixtof(scale);
	GLint matrix_mode;
	
#define BIN_2_DEG(x) (-(x) * 180.0 / 128)
	
	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(fixtof(x), fixtof(y), 0.);
	glRotated(BIN_2_DEG(fixtof(angle)), 0., 0., -1.);
	glScaled(dscale, dscale, dscale);
	glTranslated(-fixtof(x+cx), -fixtof(y+cy), 0.);
	
	do_masked_blit_video(sprite, bmp, 0, 0, fixtoi(x), fixtoi(y),
	                      sprite->w, sprite->h, v_flip ? AGL_V_FLIP : FALSE, FALSE);
	glPopMatrix();
	glMatrixMode(matrix_mode);

#undef BIN_2_DEG

	return;
}


/* allegro_gl_video_do_stretch_blit:
 * overload for all kind of video -> video and video -> screen stretchers
 * FIXME: Doesn't apply scale to the memory copy
 * FIXME: Doesn't work for video->video when FBO is not available.
 */
static void allegro_gl_video_do_stretch_blit(BITMAP *source, BITMAP *dest,
               int source_x, int source_y, int source_width, int source_height,
               int dest_x, int dest_y, int dest_width, int dest_height,
               int masked) {
	/* note: src is a video bitmap, dest is not a memory bitmap */

	double scalew = ((double)dest_width) / source_width;
	double scaleh = ((double)dest_height) / source_height;
	
	GLint matrix_mode;

	/* BITMAP_BLIT_CLIP macro from glvtable.c is no good for scaled images. */
	if (dest->clip) {
		if ((dest_x >= dest->cr) || (dest_y >= dest->cb)
		 || (dest_x + dest_width < dest->cl) || (dest_y + dest_height < dest->ct)) {
			return;
		}
		if (dest_x < dest->cl) {
			source_x -= (dest_x - dest->cl) / scalew;
			dest_x = dest->cl;
		}
		if (dest_y < dest->ct) {
			source_y -= (dest_y - dest->ct) / scaleh;
			dest_y = dest->ct;
		}
		if (dest_x + dest_width > dest->cr) {
			source_width -= (dest_x + dest_width - dest->cr) / scalew;
			dest_width = dest->cr - dest_x;
		}
		if (dest_y + dest_height > dest->cb) {
			source_height -= (dest_y + dest_height - dest->cb) / scaleh;
			dest_height = dest->cb - dest_y;
		}
	}

	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(dest_x, dest_y, 0.);
	glScaled(scalew, scaleh, 1.);
	glTranslated(-dest_x, -dest_y, 0.);

	if (masked) {
		if (is_screen_bitmap(dest)) {
			do_masked_blit_screen(source, dest, source_x, source_y,
                                  dest_x, dest_y, source_width, source_height,
                                  FALSE, AGL_REGULAR_BMP);
		}
		else {
			do_masked_blit_video(source, dest, source_x, source_y,
                                 dest_x, dest_y, source_width, source_height,
                                 FALSE, AGL_REGULAR_BMP);
		}
	}
	else {
		allegro_gl_screen_blit_to_self(source, dest, source_x, source_y,
                           dest_x, dest_y, source_width, source_height);
	}

	glPopMatrix();
	glMatrixMode(matrix_mode);

	return;
}



/* allegro_gl_video_draw_trans_rgba_sprite:
 * draw_trans_sprite() overload for video -> video drawing
 */
static void allegro_gl_video_draw_trans_rgba_sprite(BITMAP *bmp,
								BITMAP *sprite, int x, int y) {
	/* Adapt variables for FOR_EACH_TEXTURE_FRAGMENT macro. */
	BITMAP *source = sprite;
	BITMAP *dest = bmp;
	int dest_x = x;
	int dest_y = y;
	int source_x = 0;
	int source_y = 0;
	int width = sprite->w;
	int height = sprite->h;
	GLint format = __allegro_gl_get_bitmap_color_format(sprite, AGL_TEXTURE_HAS_ALPHA);
	GLint type = __allegro_gl_get_bitmap_type(sprite, 0);

	if (__allegro_gl_blit_operation == AGL_OP_LOGIC_OP)
		glEnable(GL_COLOR_LOGIC_OP);
	else
		glEnable(GL_BLEND);

	FOR_EACH_TEXTURE_FRAGMENT (
		allegro_gl_screen_blit_to_self(source, screen, sx, sy, dx, dy, w, h),
		allegro_gl_upload_and_display_texture(sprite, sx, sy, dx, dy, w, h, 0, format, type),
		__video_update_memory_copy(vid->memory_copy, dest, sx, sy, dx, dy, w, h, TRANS),
		__video_update_memory_copy(source, dest, 0, 0, x, y, sprite->w, sprite->h, TRANS),
		allegro_gl_video_blit_from_memory_ex(vid->memory_copy, dest, sx, sy, dx, dy, w, h, TRANS),
		allegro_gl_video_blit_from_memory_ex(source, dest, 0, 0, x, y, sprite->w, sprite->h, TRANS)
	)

	if (__allegro_gl_blit_operation == AGL_OP_LOGIC_OP)
		glDisable(GL_COLOR_LOGIC_OP);
	else
		glDisable(GL_BLEND);

	return;
}



/* allegro_gl_video_draw_sprite_ex:
 * draw_sprite_ex() overload for video -> video and memory -> video drawing
 *
 * When mode is DRAW_SPRITE_TRANS:
 * FIXME: Broken if the bitmap was split into multiple textures.
 * FIXME: Doesn't apply flipping to the memory copy
 */
static void allegro_gl_video_draw_sprite_ex(BITMAP *bmp, BITMAP *sprite,
                                            int x, int y, int mode, int flip) {
	int matrix_mode;
	int lflip = 0;

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
			do_masked_blit_video(sprite, bmp, 0, 0, x, y,
                                 sprite->w, sprite->h, lflip, FALSE);
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

			allegro_gl_video_draw_trans_rgba_sprite(bmp, sprite, x, y);

			if (lflip) {
				glPopMatrix();
				glMatrixMode(matrix_mode);
			}
		break;
		case DRAW_SPRITE_LIT:
			/* not implemented */
		break;
	}
}



static void allegro_gl_video_clear_to_color(BITMAP *bmp, int color) {
	AGL_VIDEO_BITMAP *vid = bmp->extra;

	if (vid->fbo) {
		static GLint v[4];
		static double allegro_gl_projection_matrix[16];
		static double allegro_gl_modelview_matrix[16];

		glGetIntegerv(GL_VIEWPORT, &v[0]);
		glMatrixMode(GL_MODELVIEW);
		glGetDoublev(GL_MODELVIEW_MATRIX, allegro_gl_modelview_matrix);
		glMatrixMode(GL_PROJECTION);
		glGetDoublev(GL_PROJECTION_MATRIX, allegro_gl_projection_matrix);

		while (vid) {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vid->fbo);

			glViewport(0, 0, vid->memory_copy->w, vid->memory_copy->h);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, vid->memory_copy->w, 0, vid->memory_copy->h);
			glMatrixMode(GL_MODELVIEW);

			allegro_gl_screen_clear_to_color(bmp, color);
			clear_to_color(vid->memory_copy, color);
			vid = vid->next;
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
		glViewport(v[0], v[1], v[2], v[3]);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(allegro_gl_projection_matrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixd(allegro_gl_modelview_matrix);
	}
	else {
		allegro_gl_video_rectfill(bmp, 0, 0, bmp->w, bmp->h, color);
	}
}



/* FIXME: Doesn't work when FBO is not available.
 * FIXME: Doesn't care for segmented video bitmaps.
 */
static void allegro_gl_video_draw_color_glyph(struct BITMAP *bmp,
    struct BITMAP *sprite, int x, int y, int color, int bg)
{
	AGL_VIDEO_BITMAP *vid = bmp->extra;

	static GLint v[4];
	static double allegro_gl_projection_matrix[16];
	static double allegro_gl_modelview_matrix[16];

	if (!vid->fbo)
		return;

	glGetIntegerv(GL_VIEWPORT, &v[0]);
	glMatrixMode(GL_MODELVIEW);
	glGetDoublev(GL_MODELVIEW_MATRIX, allegro_gl_modelview_matrix);
	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, allegro_gl_projection_matrix);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vid->fbo);

	glViewport(0, 0, vid->memory_copy->w, vid->memory_copy->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, vid->memory_copy->w, 0, vid->memory_copy->h);
	glMatrixMode(GL_MODELVIEW);

	allegro_gl_screen_draw_color_glyph_ex(bmp, sprite, x, y, color, bg, 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glViewport(v[0], v[1], v[2], v[3]);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(allegro_gl_projection_matrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(allegro_gl_modelview_matrix);

	vid->memory_copy->vtable->draw_character(vid->memory_copy, sprite, x, y, color, bg);
}



static void allegro_gl_video_draw_256_sprite(BITMAP *bmp, BITMAP *sprite,
                                      int x, int y) {
	allegro_gl_video_draw_color_glyph(bmp, sprite, x, y, -1, _textmode);
}



static void allegro_gl_video_draw_character(BITMAP *bmp, BITMAP *sprite,
                                            int x, int y, int color, int bg) {
	allegro_gl_video_draw_color_glyph(bmp, sprite, x, y, color, bg);
}



/* FIXME: Doesn't work when FBO is not available.
 * FIXME: Doesn't care for segmented video bitmaps.
 */
static void allegro_gl_video_draw_glyph(struct BITMAP *bmp,
                               AL_CONST struct FONT_GLYPH *glyph, int x, int y,
                               int color, int bg) {
	AGL_VIDEO_BITMAP *vid = bmp->extra;

	static GLint v[4];
	static double allegro_gl_projection_matrix[16];
	static double allegro_gl_modelview_matrix[16];

	if (!vid->fbo)
		return;

	glGetIntegerv(GL_VIEWPORT, &v[0]);
	glMatrixMode(GL_MODELVIEW);
	glGetDoublev(GL_MODELVIEW_MATRIX, allegro_gl_modelview_matrix);
	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, allegro_gl_projection_matrix);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vid->fbo);

	glViewport(0, 0, vid->memory_copy->w, vid->memory_copy->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, vid->memory_copy->w, 0, vid->memory_copy->h);
	glMatrixMode(GL_MODELVIEW);

	allegro_gl_screen_draw_glyph_ex(bmp, glyph, x, y, color, bg, 1);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glViewport(v[0], v[1], v[2], v[3]);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(allegro_gl_projection_matrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(allegro_gl_modelview_matrix);

	vid->memory_copy->vtable->draw_glyph(vid->memory_copy, glyph, x, y, color, bg);
}



static void allegro_gl_video_polygon3d_f(BITMAP *bmp, int type, BITMAP *texture,
								  int vc, V3D_f *vtx[])
{
	AGL_VIDEO_BITMAP *vid = bmp->extra;

	/* Switch to software mode is using polygon type that isn't supported by
	   allegro_gl_screen_polygon3d_f. */
	int use_fbo = (type == POLYTYPE_FLAT) ||
				  (type == POLYTYPE_GRGB) ||
				  (type == POLYTYPE_GCOL) ||
				  (type == POLYTYPE_ATEX) ||
				  (type == POLYTYPE_PTEX) ||
				  (type == POLYTYPE_ATEX_TRANS) ||
				  (type == POLYTYPE_PTEX_TRANS);

	if (vid->fbo && use_fbo) {
		static GLint v[4];
		static double allegro_gl_projection_matrix[16];
		static double allegro_gl_modelview_matrix[16];

		glGetIntegerv(GL_VIEWPORT, &v[0]);
		glMatrixMode(GL_MODELVIEW);
		glGetDoublev(GL_MODELVIEW_MATRIX, allegro_gl_modelview_matrix);
		glMatrixMode(GL_PROJECTION);
		glGetDoublev(GL_PROJECTION_MATRIX, allegro_gl_projection_matrix);

		while (vid) {
			BITMAP *mem_texture;

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vid->fbo);

			glViewport(0, 0, vid->memory_copy->w, vid->memory_copy->h);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, vid->memory_copy->w, 0, vid->memory_copy->h);
			glMatrixMode(GL_MODELVIEW);

			allegro_gl_screen_polygon3d_f(bmp, type, texture, vc, vtx);

			if (is_video_bitmap(texture)) {
				AGL_VIDEO_BITMAP *vbmp = texture->extra;
				mem_texture = vbmp->memory_copy;
			}
			else {
				mem_texture = texture;
			}
			polygon3d_f(vid->memory_copy, type, mem_texture, vc, vtx);

			vid = vid->next;
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
		glViewport(v[0], v[1], v[2], v[3]);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(allegro_gl_projection_matrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixd(allegro_gl_modelview_matrix);
	}
	else {
		int i;
		AGL_VIDEO_BITMAP *vid;
	
		vid = bmp->extra;
	
		if (is_sub_bitmap(bmp)) {
			for (i = 0; i < vc; ++i) {
				vtx[i]->x += bmp->x_ofs;
				vtx[i]->y += bmp->y_ofs;
			}
		}

		while (vid) {
			BITMAP *mem_texture;
			int _y1, _y2, _x1, _x2;
			BITMAP *vbmp = vid->memory_copy;

			_x1 = 9999;
			for (i = 0; i < vc; ++i)
				if (vtx[i]->x < _x1) _x1 = vtx[i]->x;

			_x2 = -9999;
			for (i = 0; i < vc; ++i)
				if (vtx[i]->x > _x2) _x2 = vtx[i]->x;

			_y1 = 9999;
			for (i = 0; i < vc; ++i)
				if (vtx[i]->y < _y1) _y1 = vtx[i]->y;

			_y2 = -9999;
			for (i = 0; i < vc; ++i)
				if (vtx[i]->y > _y2) _y2 = vtx[i]->y;

			if (vid->x_ofs > _x2 || vid->y_ofs > _y2
			 || vid->x_ofs + vbmp->w <= _x1
			 || vid->y_ofs + vbmp->h <= _y1) {
				
				vid = vid->next;
				continue;
			}
			
			_x1 = MAX(0, _x1 - vid->x_ofs);
			_x2 = (_x2 - (vid->x_ofs + vbmp->w) > 0) ? vbmp->w - 1: _x2 - vid->x_ofs;
			_y1 = MAX(0, _y1 - vid->y_ofs);
			_y2 = (_x2 - (vid->y_ofs + vbmp->h) > 0) ? vbmp->h - 1: _y2 - vid->y_ofs;

			if (is_video_bitmap(texture)) {
				AGL_VIDEO_BITMAP *tex = texture->extra;
				mem_texture = tex->memory_copy;
			}
			else {
				mem_texture = texture;
			}
			polygon3d_f(vid->memory_copy, type, mem_texture, vc, vtx);
	
			update_texture_memory(vid, _x1, _y1, _x2, _y2);
	
			vid = vid->next;
		}
	}

	return;
}



static void allegro_gl_video_polygon3d(BITMAP *bmp, int type, BITMAP *texture,
								int vc, V3D *vtx[])
{
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

	allegro_gl_video_polygon3d_f(bmp, type, texture, vc, vtx_f);

	for (i = 0; i < vc; i++)
		free(vtx_f[i]);
	free(vtx_f);
}



static void allegro_gl_video_triangle3d(BITMAP *bmp, int type, BITMAP *texture,
										V3D *v1, V3D *v2, V3D *v3)
{
	V3D *vtx[3];
	vtx[0] = v1;
	vtx[1] = v2;
	vtx[2] = v3;
	
	allegro_gl_video_polygon3d(bmp, type, texture, 3, vtx);
}



static void allegro_gl_video_triangle3d_f(BITMAP *bmp, int type, BITMAP *texture,
										  V3D_f *v1, V3D_f *v2, V3D_f *v3)
{
	V3D_f *vtx_f[3];
	vtx_f[0] = v1;
	vtx_f[1] = v2;
	vtx_f[2] = v3;

	allegro_gl_video_polygon3d_f(bmp, type, texture, 3, vtx_f);
}



static void allegro_gl_video_quad3d(BITMAP *bmp, int type, BITMAP *texture,
									V3D *v1, V3D *v2, V3D *v3, V3D *v4)
{
	V3D *vtx[4];
	vtx[0] = v1;
	vtx[1] = v2;
	vtx[2] = v3;
	vtx[3] = v4;

	allegro_gl_video_polygon3d(bmp, type, texture, 4, vtx);
}



static void allegro_gl_video_quad3d_f(BITMAP *bmp, int type, BITMAP *texture,
							   V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4)
{
	V3D_f *vtx_f[4];
	vtx_f[0] = v1;
	vtx_f[1] = v2;
	vtx_f[2] = v3;
	vtx_f[3] = v4;

	allegro_gl_video_polygon3d_f(bmp, type, texture, 4, vtx_f);
}



static void dummy_unwrite_bank(void)
{
}



static GFX_VTABLE allegro_gl_video_vtable = {
	0,
	0,
	dummy_unwrite_bank,			//void *unwrite_bank;  /* C function on some machines, asm on i386 */
	NULL,						//AL_METHOD(void, set_clip, (struct BITMAP *bmp));
	allegro_gl_video_acquire,
	allegro_gl_video_release,
	NULL,						//AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));
	allegro_gl_created_sub_bitmap,
	allegro_gl_video_getpixel,
	allegro_gl_video_putpixel,
	allegro_gl_video_vline,
	allegro_gl_video_hline,
	allegro_gl_video_hline,
	allegro_gl_video_line,
	allegro_gl_video_line,
	allegro_gl_video_rectfill,
	allegro_gl_video_triangle,
	allegro_gl_video_draw_sprite,
	allegro_gl_video_draw_256_sprite,
	allegro_gl_video_draw_sprite_v_flip,
	allegro_gl_video_draw_sprite_h_flip,
	allegro_gl_video_draw_sprite_vh_flip,
	allegro_gl_video_draw_trans_rgba_sprite,
	allegro_gl_video_draw_trans_rgba_sprite,
	NULL,						//AL_METHOD(void, draw_lit_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color));
	NULL,						//AL_METHOD(void, allegro_gl_video_draw_rle_sprite, (struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y));
	NULL,						//AL_METHOD(void, draw_trans_rle_sprite, (struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y));
	NULL,						//AL_METHOD(void, draw_trans_rgba_rle_sprite, (struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y));
	NULL,						//AL_METHOD(void, draw_lit_rle_sprite, (struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color));
	allegro_gl_video_draw_character,
	allegro_gl_video_draw_glyph,
	allegro_gl_video_blit_from_memory,
	allegro_gl_video_blit_to_memory,
	NULL,						//AL_METHOD(void, blit_from_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
	NULL,						//AL_METHOD(void, blit_to_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
	allegro_gl_screen_blit_to_self, /* Video bitmaps use same method as screen */
	allegro_gl_screen_blit_to_self, /* ..._forward */
	allegro_gl_screen_blit_to_self, /* ..._backward */
	allegro_gl_memory_blit_between_formats,
	allegro_gl_video_masked_blit,
	allegro_gl_video_clear_to_color,
	allegro_gl_video_pivot_scaled_sprite_flip,
	allegro_gl_video_do_stretch_blit,
	NULL,                       //AL_METHOD(void, draw_gouraud_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4));
	NULL,                       //AL_METHOD(void, draw_sprite_end, (void));
	NULL,                       //AL_METHOD(void, blit_end, (void));
	_soft_polygon,              //AL_METHOD(void, polygon, (struct BITMAP *bmp, int vertices, AL_CONST int *points, int color));
	_soft_rect,                 //AL_METHOD(void, rect, (struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color));
	_soft_circle,               //AL_METHOD(void, circle, (struct BITMAP *bmp, int x, int y, int radius, int color));
	_soft_circlefill,           //AL_METHOD(void, circlefill, (struct BITMAP *bmp, int x, int y, int radius, int color));
	_soft_ellipse,              //AL_METHOD(void, ellipse, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
	_soft_ellipsefill,          //AL_METHOD(void, ellipsefill, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
	_soft_arc,                  //AL_METHOD(void, arc, (struct BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color));
	_soft_spline,               //AL_METHOD(void, spline, (struct BITMAP *bmp, AL_CONST int points[8], int color));
	_soft_floodfill,            //AL_METHOD(void, floodfill, (struct BITMAP *bmp, int x, int y, int color));
	allegro_gl_video_polygon3d,
	allegro_gl_video_polygon3d_f,
	allegro_gl_video_triangle3d,
	allegro_gl_video_triangle3d_f,
	allegro_gl_video_quad3d,
	allegro_gl_video_quad3d_f,
	allegro_gl_video_draw_sprite_ex
};

