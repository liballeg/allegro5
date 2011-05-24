/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file fontconv.c
 *  \brief Allegro FONT conversion routines.
 *
 * Notes: - Depends on the Allegro's FONT structure remaining
 *          intact.
 * Bugs:  - Bitmapped font support is flakey at best.
 *
 */

#include <math.h>
#include <string.h>
#include <stdio.h>

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "alleggl.h"
#include "allglint.h"

#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#if defined ALLEGRO_WITH_XWINDOWS && !defined ALLEGROGL_GENERIC_DRIVER
#include <xalleg.h>
#include <GL/glx.h>
#endif

#define PREFIX_I                "agl-font INFO: "
#define PREFIX_W                "agl-font WARNING: "
#define PREFIX_E                "agl-font ERROR: "


/* Number of pixels between characters in a textured font.
 */
#define FONT_CHARACTER_SPACING 2

/* Uncomment to have the font generator dump screenshots of the textures it
 * generates.
 */
/* #define SAVE_FONT_SCREENSHOT */


static int agl_get_font_height(AL_CONST FONT *f);
static int agl_char_length(const FONT *f, int ch);
static int agl_text_length(const FONT *f, const char *str);

static int agl_get_font_ranges(FONT *f);
static int agl_get_font_range_begin(FONT *f, int range);
static int agl_get_font_range_end(FONT *f, int range);
static FONT *agl_extract_font_range(FONT *f, int start, int end);
static FONT *agl_merge_fonts(FONT *f1, FONT *f2);

#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 0)
static int agl_transpose_font(FONT *f, int drange);
#endif


FONT_VTABLE _agl_font_vtable = {
	agl_get_font_height,
	agl_char_length,
	agl_text_length,
	NULL, /* render_char */
	NULL, /* render */
	allegro_gl_destroy_font,
	agl_get_font_ranges,
	agl_get_font_range_begin,
	agl_get_font_range_end,
	agl_extract_font_range,
	agl_merge_fonts,
#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 0)
	agl_transpose_font
#endif
};


FONT_VTABLE *font_vtable_agl = &_agl_font_vtable;

static void aglf_convert_allegro_font_to_bitmap(FONT_AGL_DATA *dest, FONT *f,
                                                void *src, int *height);
static void aglf_convert_allegro_font_to_texture(FONT_AGL_DATA **dest, FONT *f,
                             void *src, int *height, float scale, GLint format);
static GLuint aglf_upload_texture(BITMAP *bmp, GLint format, int has_alpha);
static int aglf_check_texture(BITMAP *bmp, GLint format, int has_alpha);
static BITMAP* look_for_texture(int beg, int end, AGL_GLYPH *glyphs,
                                int max_w, int max_h, int total_area,
                                GLint format, int has_alpha);



union mixed_ptr {
	FONT_MONO_DATA* mf;
	FONT_COLOR_DATA* cf;
	void *ptr;
};


/* Stores info about a texture size */
typedef struct texture_size {
	int w, h;
} texture_size;



static int agl_get_font_height(AL_CONST FONT *f) {
	return f->height;
}


/* iroundf:
 * Round float to nearest integer, away from zero.
 */
static int iroundf(float v) {
	float f = floor(v);
	float c = ceil(v);

	if (v >= 0) {
		/* distance to ceil smaller than distance to floor */
		if ((c - v) < (v - f))
			return (int)c;
		else
			return (int)f;
	}
	else {
		/* distance to ceil smaller than distance to floor */
		if ((c - v) < (v - f))
			return (int)f;
		else
			return (int)c;
	}
}



/* agl_char_length_fractional:
 * Returns the width, in fractional pixels of the given character.
 */
static float agl_char_length_fractional(const FONT *f, int ch) {
	FONT_AGL_DATA *fad = f->data;

	if (fad->type == AGL_FONT_TYPE_TEXTURED) {
		while (fad) {
			if (ch >= fad->start && ch < fad->end) {
				AGL_GLYPH *coords = &(fad->glyph_coords[ch - fad->start]);
				return (coords->offset_x + coords->w + coords->offset_w)
				       / fabs(fad->scale);
			}

			fad = fad->next;
		}
	}
	else if (fad->type == AGL_FONT_TYPE_BITMAP) {
		while (fad) {
			if (ch >= fad->start && ch < fad->end) {
				FONT_GLYPH **gl = fad->data;
				return gl[ch - fad->start]->w;
			}

			fad = fad->next;
		}
	}

	/* if we don't find the character, then search for the missing
	 * glyph, but don't get stuck in a loop. */
	if (ch != allegro_404_char)
		return agl_char_length_fractional(f, allegro_404_char);

	return 0;
}



/* agl_char_length:
 * font vtable entry
 * Returns the width, in pixels of the given character.
 */
static int agl_char_length(const FONT *f, int ch) {
	return iroundf(agl_char_length_fractional(f, ch));
}



/* agl_text_length:
 * font vtable entry
 * Returns the length, in pixels, of a string as rendered in a font.
 */
static int agl_text_length(const FONT *f, const char *str) {
	int ch = 0;
	float l = 0;
	const char *p = str;
	ASSERT(f);
	ASSERT(str);

	while ( (ch = ugetxc(&p)) ) {
		l += agl_char_length_fractional(f, ch);
	}

	return iroundf(l);
}



/* agl_get_font_ranges:
 * font vtable entry
 * Returns the number of character ranges in a font, or -1 if that information
 *   is not available.
 */
static int agl_get_font_ranges(FONT *f) {
	FONT_AGL_DATA *fad;
	int ranges = 0;

	if (!f)
		return 0;

	fad = (FONT_AGL_DATA*)(f->data);

	while (fad) {
		FONT_AGL_DATA *next = fad->next;

		ranges++;
		if (!next)
			return ranges;
		fad = next;
	}

	return -1;
}



/* agl_get_font_range_begin:
 * font vtable entry
 * Get first character for font.
 */
static int agl_get_font_range_begin(FONT *f, int range) {
	FONT_AGL_DATA *fad;
	int n = 0;

	if (!f || !f->data)
		return -1;

	if (range < 0)
		range = 0;

	fad = (FONT_AGL_DATA*)(f->data);
	while (fad && n <= range) {
		FONT_AGL_DATA *next = fad->next;

		if (!next || range == n)
			return fad->start;
		fad = next;
		n++;
	}

	return -1;
}



/* agl_get_font_range_end:
 * font vtable entry
 * Get last character for font range.
 */
static int agl_get_font_range_end(FONT *f, int range) {
	FONT_AGL_DATA* fad = 0;
	int n = 0;

	if (!f || !f->data)
		return -1;

	fad = (FONT_AGL_DATA*)(f->data);

	while (fad && (n <= range || range == -1)) {
		FONT_AGL_DATA *next = fad->next;
		if (!next || range == n)
			return fad->end - 1;
		fad = next;
		n++;
	}

	return -1;
}



/* Creates a call lists from given glyph coords. Returns list base.*/
static int create_textured_font_call_lists(AGL_GLYPH *coords, int max, BITMAP *bmp,
													float scale, int *height) {
	GLuint list;
	int i;

	int rev = scale < 0 ? 1 : 0;
	scale = fabs(scale);

	list = glGenLists(max);

	for (i = 0; i < max; i++) {
		/* Coords of glyph in texture (texture coords) */
		float tx = (float)coords[i].x / bmp->w;
		float ty = 1.0 - (float)coords[i].y / bmp->h;
		/* Size of glyph in texture (texture coords) */
		float dtx = (float)(coords[i].w) / bmp->w;
		float dty = (float)(coords[i].h) / bmp->h;

		/* Offset to apply to glyph (output coords) */
		float xoffs = (float)coords[i].offset_x / scale;
		float yoffs = (float)coords[i].offset_y / scale;
		/* Size of rendered glyph (output coords) */
		float woffs = (float)coords[i].w / scale;
		float hoffs = (float)coords[i].h / scale;

		/* Size of overall screen character including dead space */
		float sizew = (float)(coords[i].offset_x + coords[i].w
					+ coords[i].offset_w) / scale;
		int sizeh = iroundf((coords[i].offset_y + coords[i].h
		            + coords[i].offset_h) / scale);

		if ((*height) < sizeh)
			*height = sizeh;

		if (rev) {
			hoffs = -hoffs;
			yoffs = -yoffs;
		}

		glNewList(list + i, GL_COMPILE);

		glBegin(GL_QUADS);
			glTexCoord2f(tx, ty);
			glVertex2f(xoffs, -yoffs);

			glTexCoord2f(tx + dtx, ty);
			glVertex2f(xoffs + woffs, -yoffs);

			glTexCoord2f(tx + dtx, ty - dty);
			glVertex2f(xoffs + woffs, -yoffs - hoffs);

			glTexCoord2f(tx, ty - dty);
			glVertex2f(xoffs, -yoffs - hoffs);
		glEnd();

		glTranslatef(sizew, 0, 0);

		glEndList();
	}

	return list;
}



/* copy_glyph_range:
 * Copies part of glyph range.
 */
static FONT_AGL_DATA* copy_glyph_range(FONT_AGL_DATA *fad, int start, int end,
																int *height) {
	int i, count, w = 0, h = 0;
	AGL_GLYPH *coords;
	BITMAP *bmp, *srcbmp;
	FONT_AGL_DATA *newfad = NULL;

	if (fad->type != AGL_FONT_TYPE_TEXTURED)
		return NULL;

	count = end - start;

	coords = malloc(count * sizeof (AGL_GLYPH));

	/* for now, just copy glyph coords of the range */
	for (i = 0; i < count; i++) {
		coords[i] = fad->glyph_coords[start - fad->start + i];
		coords[i].glyph_num = i;
	}

	/* calculate the width of the glyphs and find the max height */
	for (i = 0; i < count; i++) {
		int hh = coords[i].h + coords[i].offset_y + coords[i].offset_h;
		if (h < hh)
			h = hh;
		w += coords[i].w + coords[i].offset_w + coords[i].offset_x;
	}

	srcbmp = (BITMAP*)fad->data;

	/* allocate a new bitmap to hold new glyphs */
	w = __allegro_gl_make_power_of_2(w);
	h = __allegro_gl_make_power_of_2(h);
	bmp = create_bitmap_ex(bitmap_color_depth(srcbmp), w, h);
	if (!bmp) {
		TRACE(PREFIX_E "copy_glyph_range: Unable to create bitmap of size"
				"%ix%i pixels!\n", w, h);
		free(coords);
		return NULL;
	}

	if (__allegro_gl_get_num_channels(fad->format) == 4) {
		clear_to_color(bmp, bitmap_mask_color(bmp));
	}
	else {
		clear_bitmap(bmp);
	}

	/* blit every glyph from the range to the new bitmap */
	w = 0;
	for (i = 0; i < count; i++) {
		int ch = start - fad->start + i;
		int ww = coords[i].w + coords[i].offset_w + coords[i].offset_x;
		blit(srcbmp, bmp, fad->glyph_coords[ch].x, 0, w, 0, ww, bmp->h);
		/* fix new glyphs coords while here */
		coords[i].x = w;
		w += ww;
	}

	newfad = malloc(sizeof(struct FONT_AGL_DATA));

	newfad->type = AGL_FONT_TYPE_TEXTURED;
	newfad->is_free_chunk = 0;
	newfad->scale = fad->scale;
	newfad->format = fad->format;
	newfad->has_alpha = fad->has_alpha;
	newfad->start = start;
	newfad->end = end;
	newfad->data = bmp;
	newfad->glyph_coords = coords;
	newfad->next = NULL;
	newfad->list_base = create_textured_font_call_lists(coords, count, bmp,
													newfad->scale, height);
	newfad->texture = aglf_upload_texture(bmp, newfad->format, newfad->has_alpha);

	return newfad;
}



/* agl_extract_font_range:
 * font vtable entry
 * Extracts a glyph range from a given font and makes a new font of it.
 */
static FONT *agl_extract_font_range(FONT *f, int start, int end) {
	FONT *retval = NULL;
	FONT_AGL_DATA *fad, *next, *newfad = NULL;
	int count;

	if (!f)
		return NULL;

	/* check if range boundaries make sense */
	if (start == -1 && end == -1) {
	}
	else if (start == -1 && end > agl_get_font_range_begin(f, -1)) {
	}
	else if (end == -1 && start <= agl_get_font_range_end(f, -1)) {
	}
	else if (start <= end && start != -1 && end != -1) {
	}
	else
		return NULL;

	fad = (FONT_AGL_DATA*)f->data;

	/* only textured fonts are supported */
	if (fad->type != AGL_FONT_TYPE_TEXTURED)
		return NULL;

	/* anticipate invalid range values */
	start = MAX(start, agl_get_font_range_begin(f, -1));
	if (end > -1) {
		end = MIN(end, agl_get_font_range_end(f, -1));
	}
	else {
		end = agl_get_font_range_end(f, -1);
	}
	end++;

	retval = malloc(sizeof (struct FONT));
	retval->height = 0;
	retval->vtable = font_vtable_agl;

	next = fad;
	count = end - start;

	while (next) {
		/* find the range that is covered by the requested range
		 * check if the requested and processed ranges at least overlap
		 *    or if the requested range wraps processed range.
		 */
		if ((start >= next->start && start < next->end)
		 || (end   <= next->end   && end   > next->start)
		 || (start <  next->start && end   > next->end)) {
			int local_start, local_end;

			/* extract the overlapping range */
			local_start = MAX(next->start, start);
			local_end = MIN(next->end, end);

			if (newfad) {
				newfad->next = copy_glyph_range(next, local_start, local_end,
															&(retval->height));
				newfad = newfad->next;
				newfad->is_free_chunk = TRUE;
			}
			else {
				newfad = copy_glyph_range(next, local_start, local_end,
															&(retval->height));
				retval->data = newfad;
			}
		}

		next = next->next;
	}

	return retval;
}



/* agl_merge_fonts:
 * font vtable entry
 * Merges f2 with f1 and returns a new font.
 */
static FONT *agl_merge_fonts(FONT *f1, FONT *f2) {
	FONT *retval;
	FONT_AGL_DATA *fad1, *fad2, *fad = NULL;
	int phony = 0;

	if (!f1 || !f2)
		return NULL;

	fad1 = (FONT_AGL_DATA*)f1->data;
	fad2 = (FONT_AGL_DATA*)f2->data;

	/* fonts must be textured and of the same format */
	if (fad1->type != AGL_FONT_TYPE_TEXTURED ||
	    fad2->type != AGL_FONT_TYPE_TEXTURED)
		return NULL;

	if (fad1->format != fad2->format)
		return NULL;

	/* alloc output font */
	retval = malloc(sizeof(struct FONT));
	retval->vtable = font_vtable_agl;
	retval->height = MAX(f1->height, f2->height);

	while (fad1 || fad2) {
		if (fad1 && (!fad2 || fad1->start < fad2->start)) {
			if (fad) {
				fad->next = copy_glyph_range(fad1, fad1->start, fad1->end,
																	 	&phony);
				fad = fad->next;
				fad->is_free_chunk = TRUE;
			}
			else {
				fad = copy_glyph_range(fad1, fad1->start, fad1->end, &phony);
				retval->data = fad;
			}
			fad1 = fad1->next;
		}
		else {
			if (fad) {
				fad->next = copy_glyph_range(fad2, fad2->start, fad2->end,
																	&phony);
				fad = fad->next;
				fad->is_free_chunk = TRUE;
			}
			else {
				fad = copy_glyph_range(fad2, fad2->start, fad2->end, &phony);
				retval->data = fad;
			}
			fad2 = fad2->next;
		}
	}

	return retval;
}



#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 0)
/* agl_transpose_font:
 * font vtable entry
 * Transposes characters in a font.
 */
static int agl_transpose_font(FONT *f, int drange) {
	FONT_AGL_DATA* fad = 0;

	if (!f)
		return -1;

	fad = (FONT_AGL_DATA*)(f->data);

	while(fad) {
		FONT_AGL_DATA* next = fad->next;
		fad->start += drange;
		fad->end += drange;
		fad = next;
	}

	return 0;
}
#endif



/* allegro_gl_convert_allegro_font_ex(FONT *f, int type, float scale, GLint format) */
/** Equivalent to:
 * <pre>
 *   allegro_gl_convert_allegro_font_ex(f, type, scale, format_state);
 * </pre>
 *
 * Where format_state is the last specified format to
 * allegro_gl_set_texture_format(). If allegro_gl_set_texture_format() was not
 * previously called, AllegroGL will try to determine the texture format
 * automatically.
 *
 * \deprecated
 */
FONT *allegro_gl_convert_allegro_font(FONT *f, int type, float scale) {
	GLint format = allegro_gl_get_texture_format(NULL);
	return allegro_gl_convert_allegro_font_ex(f, type, scale, format);
}



/* allegro_gl_convert_allegro_font_ex(FONT *f, int type, float scale, GLint format) */
/** Converts a regular Allegro FONT to the AGL format for 3D display.
 *
 *  \param f      The Allegro font to convert.
 *  \param type   The font type to convert to.
 *                (see #AGL_FONT_TYPE_DONT_CARE, #AGL_FONT_TYPE_BITMAP,
 *                #AGL_FONT_TYPE_TEXTURED)
 *  \param scale  The scaling factor (see below).
 *  \param format The texture internal format to use.
 *
 *  This function will build a texture map and/or a display list that will
 *  be automatically uploaded to the video card. You can only convert to
 *  bitmap or textured fonts.
 *
 *  You can't convert an Allegro font to a vector font (#AGL_FONT_TYPE_OUTLINE)
 *  as the logistics of such an operation are quite complex.
 *  The original font will NOT be modified.
 *
 *  A valid OpenGL rendering context is required, so you may only call this
 *  function after a successful call to set_gfx_mode() with a valid OpenGL
 *  mode.
 *
 *  You should destroy the font via allegro_gl_destroy_font() when you are
 *  done with it.
 *
 * <b>Scaling</b>
 *
 *  For #AGL_FONT_TYPE_TEXTURED fonts, glyphs in the font need to be mapped
 *  to OpenGL coordinates. The scale factor ensures that you get the scaling
 *  you need. scale reprents the number of pixels to be mapped to 1.0 OpenGL
 *  units. For allegro_gl_convert_font() to behave like AllegroGL 0.0.24 and
 *  earlier, you'll need to pass 16.0 as the scale factor.
 *
 *  Alternativaly, you can make all your fonts be 1.0 units high by using:
 *  <pre>
 *   allegro_gl_convert_allegro_font(f, #AGL_FONT_TYPE_TEXTURED, 1.0/text_height(f));
 *  </pre>
 *
 *  Note that it is allowed for the scaling factor to be negative, in case your
 *  fonts appear upside down.
 *
 *  If you are planning to use your fonts on an orthographic projection where
 *  one unit maps to one pixel, then you should pass 1.0 as scale.
 *
 *  The scaling factor has no meaning for #AGL_FONT_TYPE_BITMAP fonts, so
 *  it's ignored if the conversion will lead to a font of that type.
 *
 * <b>Format</b>
 *
 *  The format specifies what internal format OpenGL should use for the texture,
 *  in the case of #AGL_FONT_TYPE_TEXTURED fonts. It has the same semantics as
 *  the internalformat parameter of glTexImage2D(). If you would like for
 *  AllegroGL to pick a texture format for you, you may supply -1 as the texture
 *  format. The default format for monochrome fonts is GL_INTENSITY4. The
 *  default format for colored fonts is GL_RGB8.
 *
 *  \return The converted font, or NULL on error.
 */
FONT *allegro_gl_convert_allegro_font_ex(FONT *f, int type, float scale,
                                         GLint format) {
	int max = 0, height = 0;
	int i;
	FONT *dest;
	FONT_AGL_DATA *destdata;
	int has_alpha = 0;

	union {
		FONT_MONO_DATA* mf;
		FONT_COLOR_DATA* cf;
		void *ptr;
	} dat;

	if (!__allegro_gl_valid_context) {
		return NULL;
	}

	if (!f) {
		TRACE(PREFIX_E "convert_allegro_font: Null source\n");
		return NULL;
	}

	/* Make sure it's an Allegro font - we don't want any surprises */
#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
	if (f->vtable != font_vtable_mono && f->vtable != font_vtable_color && f->vtable != font_vtable_trans) {
#else
	if (f->vtable != font_vtable_mono && f->vtable != font_vtable_color) {
#endif
		TRACE(PREFIX_I "convert_allegro_font: Source font is not "
		      "in Allegro format\n");
		return NULL;
	}

	/* No vector fonts allowed as destination */
	if (type == AGL_FONT_TYPE_OUTLINE) {
		/* Can't convert bitmap to vector font */
		TRACE(PREFIX_I "convert_allegro_font: Unable to convert a "
		      "pixmap font to a vector font.\n");
		return NULL;
	}

	/* Make sure the scaling factor is appropreate */
	if (fabs(scale) < 0.001) {
		TRACE(PREFIX_W "convert_allegro_font: Scaling factor might be "
		      "too small: %f\n", scale);
	}

	/* Count number of ranges */
	max = get_font_ranges(f);

	/* There should really be an API for this */
	dest = (FONT*)malloc(sizeof(FONT));
	if (!dest) {
		TRACE(PREFIX_E "convert_allegro_font: Ran out of memory "
		      "while allocating %i bytes\n", (int)sizeof(FONT));
		return NULL;
	}
	destdata = (FONT_AGL_DATA*)malloc(sizeof(FONT_AGL_DATA) * max);
	if (!destdata) {
		TRACE(PREFIX_E "convert_allegro_font: Ran out of memory "
		      "while allocating %i bytes\n", (int)sizeof(FONT_AGL_DATA) * max);
		return NULL;
	}
	memset(destdata, 0, sizeof(FONT_AGL_DATA) * max);

	/* Construct the linked list */
	for (i = 0; i < max - 1; i++) {
		destdata[i].next = &destdata[i + 1];
	}
	destdata[max - 1].next = NULL;

	/* Set up the font */
	dest->data = destdata;
	dest->vtable = font_vtable_agl;
	dest->height = 0;

	destdata->type = type;

	if (type == AGL_FONT_TYPE_DONT_CARE) {
		destdata->type = AGL_FONT_TYPE_TEXTURED;
	}

#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
	has_alpha = (f->vtable == font_vtable_trans);
#endif

	/* Convert each range */
	dat.ptr = f->data;

	while (dat.ptr) {

		destdata->has_alpha = has_alpha;

		if (type == AGL_FONT_TYPE_BITMAP) {
			aglf_convert_allegro_font_to_bitmap(destdata, f, dat.ptr, &height);
		}
		else if (type == AGL_FONT_TYPE_TEXTURED) {
			aglf_convert_allegro_font_to_texture(&destdata, f, dat.ptr, &height,
			                                     scale, format);
		}

		if (height > dest->height) {
			dest->height = height;
		}

		dat.ptr = (is_mono_font(f) ? (void*)dat.mf->next : (void*)dat.cf->next);

		destdata = destdata->next;
	}

	return dest;
}



/* QSort helper for sorting glyphs according to width,
 * then height - largest first.
 */
static int sort_glyphs(const void *c1, const void *c2) {
	AGL_GLYPH *g1 = (AGL_GLYPH*)c1;
	AGL_GLYPH *g2 = (AGL_GLYPH*)c2;

	if (g1->w < g2->w) {
		return 1;
	}
	else if (g1->w == g2->w) {
		return -g1->h + g2->h;
	}
	else {
		return -1;
	}
}



/* QSort helper for unsorting glyphs.
 */
static int unsort_glyphs(const void *c1, const void *c2) {
	AGL_GLYPH *g1 = (AGL_GLYPH*)c1;
	AGL_GLYPH *g2 = (AGL_GLYPH*)c2;

	return g1->glyph_num - g2->glyph_num;
}



/* QSort helper for sorting textures by area.
 */
static int sort_textures(const void *c1, const void *c2) {
	texture_size *t1 = (texture_size*)c1;
	texture_size *t2 = (texture_size*)c2;

	return t1->w * t1->h - t2->w * t2->h;
}



#ifdef SAVE_FONT_SCREENSHOT
static void save_shot(BITMAP *bmp) {

	int i;
	char name[128];

	for (i = 0; i < 1000; i++) {
		/* TGA, in case it is a truecolor font with alpha */
		sprintf(name, "fonttest_%02i.tga", i);
		if (!exists(name)) {
			save_tga(name, bmp, NULL);
			break;
		}
	}
}
#endif



/* Helper function. This will try to place all the glyphs in the bitmap the
 * best way it can
 */
static int aglf_sort_out_glyphs(BITMAP *bmp, AGL_GLYPH *glyphs, const int beg,
                                const int end) {

	int i, j;
	int last_line = 0;
	int last_x = 0;

	/* We now try to make all the glyphs fit on the bitmap */
	for (i = 0; i < end - beg; i++) {
		int collide = FALSE;

		/* Place glyphs on last_line */
		glyphs[i].x = last_x;
		glyphs[i].y = last_line;

		/* Check for collision */

		for (j = 0; j < i; j++) {
			if ((glyphs[i].x >= glyphs[j].x + glyphs[j].w)
			        || (glyphs[i].y >= glyphs[j].y + glyphs[j].h)
			        || (glyphs[j].x >= glyphs[i].x + glyphs[i].w)
			        || (glyphs[j].y >= glyphs[i].y + glyphs[i].h)) {
				continue;
			}
			last_x = glyphs[j].x + glyphs[j].w;
			glyphs[i].x = last_x;
			j = 0;
		}

		if ((last_x + glyphs[i].w > bmp->w)
		 || (last_line + glyphs[i].h > bmp->h)) {
			collide = TRUE;
		}

		if (collide) {
			/* If there was a collision, we need to find the sprite with
			 * the smallest height that is still grater than last_line.
			 * We also need to redo this glyph.
			 */
			int min_line = bmp->h + 1;
			int min_glyph = -1;

			for (j = 0; j < i; j++) {
				if ( glyphs[j].y + glyphs[j].h < min_line
				  && glyphs[j].y + glyphs[j].h
				  > last_line - FONT_CHARACTER_SPACING) {

					min_line = glyphs[j].y + glyphs[j].h
					         + FONT_CHARACTER_SPACING;
					min_glyph = j;
				}
			}
			/* If it can't possibly all fit, failure */
			if (min_glyph == -1) {
				TRACE(PREFIX_I "sort_out_glyphs: Unable to fit all glyphs into "
				      "the texture.\n");
				return FALSE;
			}
			/* Otherwise, start over at the top of that glyph */
			last_x = glyphs[min_glyph].x;
			last_line = min_line;

			/* Redo this glyph */
			i--;
		}
		else {
			last_x += glyphs[i].w + FONT_CHARACTER_SPACING;
		}
	}

	/* All ok */
	return TRUE;
}



static int split_font(FONT *f, void *source, void **dest1, void **dest2) {

	union mixed_ptr range1, range2, src;
	int colored;
	int i;

	(*dest1) = NULL;
	(*dest2) = NULL;
	src.ptr = source;

	colored = (is_mono_font(f) ? FALSE : TRUE);

	/* Allocate the ranges that we need */
	range1.ptr = malloc(colored ? sizeof(FONT_COLOR_DATA)
	                            : sizeof(FONT_MONO_DATA));
	if (!range1.ptr) {
		TRACE(PREFIX_E "split_font() - Ran out of memory while "
		      "trying ot allocate %i bytes.\n",
		      colored ? (int)sizeof(FONT_COLOR_DATA)
		      : (int)sizeof(FONT_MONO_DATA));
		return FALSE;
	}

	range2.ptr = malloc(colored ? sizeof(FONT_COLOR_DATA)
	                            : sizeof(FONT_MONO_DATA));
	if (!range2.ptr) {
		TRACE(PREFIX_E "split_font() - Ran out of memory while "
		      "trying to allocate %i bytes.\n",
		      colored ? (int)sizeof(FONT_COLOR_DATA)
		              : (int)sizeof(FONT_MONO_DATA));
		free(range1.ptr);
		return FALSE;
	}

	(*dest1) = range1.ptr;
	(*dest2) = range2.ptr;

	/* Now we split up the range */
	if (colored) {
		/* Half the range */
		int mid = src.cf->begin + (src.cf->end - src.cf->begin) / 2;

		range1.cf->begin = src.cf->begin;
		range1.cf->end = mid;
		range2.cf->begin = mid;
		range2.cf->end = src.cf->end;

		range1.cf->next = NULL;
		range2.cf->next = NULL;

		/* Split up the bitmaps */
		range1.cf->bitmaps = malloc(sizeof(BITMAP*)
		                                * (range1.cf->end - range1.cf->begin));
		if (!range1.cf->bitmaps) {
			TRACE(PREFIX_E "split_font() - Ran out of memory "
			      "while trying to allocate %i bytes.\n",
			      (int)sizeof(BITMAP*) * (range1.cf->end - range1.cf->begin));
			free(range1.ptr);
			free(range2.ptr);
			return FALSE;
		}

		range2.cf->bitmaps = malloc(sizeof(BITMAP*)
		                                 * (range2.cf->end - range2.cf->begin));
		if (!range2.cf->bitmaps) {
			TRACE(PREFIX_E "split_font() - Ran out of memory "
			      "while trying to allocate %i bytes.\n",
			      (int)sizeof(BITMAP*) * (range2.cf->end - range2.cf->begin));
			free(range1.cf->bitmaps);
			free(range1.ptr);
			free(range2.ptr);
			return FALSE;
		}


		for (i = 0; i < (range1.cf->end - range1.cf->begin); i++) {
			range1.cf->bitmaps[i] = src.cf->bitmaps[i];
		}
		for (i = 0; i < (range2.cf->end - range2.cf->begin); i++) {
			range2.cf->bitmaps[i] =
			           src.cf->bitmaps[i + range2.cf->begin - range1.cf->begin];
		}
	}
	else {
		/* Half the range */
		int mid = src.mf->begin + (src.mf->end - src.mf->begin) / 2;

		range1.mf->begin = src.mf->begin;
		range1.mf->end = mid;
		range2.mf->begin = mid;
		range2.mf->end = src.mf->end;

		range1.mf->next = NULL;
		range2.mf->next = NULL;

		/* Split up the bitmaps */
		range1.mf->glyphs = malloc(sizeof(FONT_GLYPH*)
		                                 * (range1.mf->end - range1.mf->begin));
		if (!range1.mf->glyphs) {
			TRACE(PREFIX_E "split_font() - Ran out of memory "
			      "while trying to allocate %i bytes.\n",
			    (int)sizeof(FONT_GLYPH*) * (range1.mf->end - range1.mf->begin));
			free(range1.ptr);
			free(range2.ptr);
			return FALSE;
		}

		range2.mf->glyphs = malloc(sizeof(FONT_GLYPH*)
		                                 * (range2.mf->end - range2.mf->begin));
		if (!range2.mf->glyphs) {
			TRACE(PREFIX_E "split_font() - Ran out of memory "
			      "while trying to allocate %i bytes.\n",
			    (int)sizeof(FONT_GLYPH*) * (range2.mf->end - range2.mf->begin));
			free(range1.mf->glyphs);
			free(range1.ptr);
			free(range2.ptr);
			return FALSE;
		}

		for (i = 0; i < (range1.mf->end - range1.mf->begin); i++) {
			range1.mf->glyphs[i] = src.mf->glyphs[i];
		}
		for (i = 0; i < (range2.mf->end - range2.mf->begin); i++) {
			range2.mf->glyphs[i] =
			            src.mf->glyphs[i + range2.mf->begin - range1.mf->begin];
		}
	}

	return TRUE;
}



/* Destroys a split font */
static void destroy_split_font(FONT *f, union mixed_ptr range1,
                                        union mixed_ptr range2) {

	if (!is_mono_font(f)) {
		free(range1.cf->bitmaps);
		free(range2.cf->bitmaps);
	}
	else {
		free(range1.mf->glyphs);
		free(range2.mf->glyphs);
	}

	free(range1.ptr);
	free(range2.ptr);

	return;
}



static int do_crop_font_range(FONT *f, AGL_GLYPH *glyphs, int beg, int end) {

	int i, j, k;
	int max = end - beg;
	char buf[32];

	/* Allocate a temp bitmap to work with */
	BITMAP *temp = create_bitmap(32, 32);

	if (!temp) {
		TRACE(PREFIX_E "crop_font_range - Unable to create "
		      "bitmap of size: %ix%i!\n", 32, 32);
		goto error;
	}

	/* Crop glyphs */
	for (i = 0; i < max; i++) {
		int used = 0;

		if (glyphs[i].w > temp->w || glyphs[i].h > temp->h) {
			int old_w = temp->w, old_h = temp->h;
			destroy_bitmap(temp);
			temp = create_bitmap(old_w * 2, old_h * 2);
			if (!temp) {
				TRACE(PREFIX_E "crop_font_range - Unable to "
				      "create bitmap of size: %ix%i!\n", old_w * 2, old_h * 2);
				goto error;
			}
		}
		clear_bitmap(temp);

		usetc(buf + usetc(buf, glyphs[i].glyph_num + beg), 0);

		textout_ex(temp, f, buf, 0, 0,
		            makecol_depth(bitmap_color_depth(temp), 255, 255, 255), 0);

		/* Crop top */
		for (j = 0; j < glyphs[i].h; j++) {
			used = 0;

			for (k = 0; k < glyphs[i].w; k++) {
				if (getpixel(temp, k, j)) {
					used = 1;
					glyphs[i].offset_y += j;
					glyphs[i].h -= j;
					break;
				}
			}
			if (used)
				break;
		}

		/* If just the top crop killed our glyph, then skip it entirely */
		if (!used) {
			TRACE(PREFIX_I "crop_font_range: skipping glyph %i\n", i);
			glyphs[i].offset_y = 0;
			glyphs[i].offset_h = glyphs[i].h - 1;
			glyphs[i].offset_w = glyphs[i].w - 2;
			glyphs[i].h = 1;
			glyphs[i].w = 1;
			continue;
		}

		/* Crop bottom */
		j = glyphs[i].h + glyphs[i].offset_y - 1;
		for ( /* above */; j >= glyphs[i].offset_y; j--) {
			used = 0;

			for (k = 0; k < glyphs[i].w; k++) {
				if (getpixel(temp, k, j)) {
					used = 1;
					glyphs[i].offset_h +=
					                   glyphs[i].h + glyphs[i].offset_y - j - 2;
					glyphs[i].h -= glyphs[i].h + glyphs[i].offset_y - j - 1;
					break;
				}
			}
			if (used)
				break;
		}

		/* Crop Left */
		for (j = 0; j < glyphs[i].w; j++) {
			used = 0;

			k = MAX(glyphs[i].offset_y - 1, 0);
			for (/* above */; k < glyphs[i].offset_y + glyphs[i].h + 1; k++) {
				if (getpixel(temp, j, k)) {
					used = 1;
					glyphs[i].offset_x += j;
					glyphs[i].w -= j;
					break;
				}
			}
			if (used)
				break;
		}

		/* Crop Right */
		j = glyphs[i].w + glyphs[i].offset_x - 1;
		for (/* above */; j >= glyphs[i].offset_x; j--) {
			used = 0;

			k = MAX(glyphs[i].offset_y - 1, 0);
			for (/* above */; k < glyphs[i].offset_y + glyphs[i].h + 1; k++) {
				if (getpixel(temp, j, k)) {
					used = 1;
					glyphs[i].offset_w +=
					                   glyphs[i].w + glyphs[i].offset_x - 1 - j;
					glyphs[i].w -= glyphs[i].w + glyphs[i].offset_x - j - 1;
					break;
				}
			}
			if (used)
				break;
		}
#ifdef LOGLEVEL
		TRACE(PREFIX_I "crop_font_range: Glyph %i (%c) offs: x: %i  y: %i, "
		      "w: %i  h: %i,  offs: w: %i  h: %i\n", i, i + beg,
		      glyphs[i].offset_x, glyphs[i].offset_y, glyphs[i].w, glyphs[i].h,
		      glyphs[i].offset_w, glyphs[i].offset_h);
#endif
	}

	destroy_bitmap(temp);

	return TRUE;

error:
	if (temp) {
		destroy_bitmap(temp);
	}

	return FALSE;
}



/* Crops a font over a particular range */
static int crop_font_range(FONT *f, void *src, int beg, int end,
                           AGL_GLYPH *glyphs,
                           int *net_area, int *gross_area,
                           int *max_w, int *max_h) {

	int i;
	int crop = 1;
	int max = end - beg;
	int ret = TRUE;

	union mixed_ptr dat;
	dat.ptr = src;

	/* Disable cropping for trucolor fonts. */
	if (is_color_font(f)) {
		FONT_COLOR_DATA *fcd = f->data;
		if (bitmap_color_depth(fcd->bitmaps[0]) != 8) {
			crop = 0;
		}
	}

	/* Load default sizes */
	for (i = 0; i < max; i++) {
		glyphs[i].glyph_num = i;

		if (is_mono_font(f)) {
			glyphs[i].w = dat.mf->glyphs[i]->w + 1;
			glyphs[i].h = dat.mf->glyphs[i]->h + 1;
		} else {
			glyphs[i].w = dat.cf->bitmaps[i]->w + 1;
			glyphs[i].h = dat.cf->bitmaps[i]->h + 1;
		}
		glyphs[i].offset_w = -1;
		glyphs[i].offset_h = -1;

		/* Not placed yet */
		glyphs[i].x = -1;
	}

	if (crop) {
		ret = do_crop_font_range(f, glyphs, beg, end);
	}

	(*gross_area) = 0;
	(*net_area) = 0;
	(*max_w) = 0;
	(*max_h) = 0;

	/* Find max w and h, total area covered by the bitmaps, and number of
	 * glyphs
	 */
	for (i = 0; i < max; i++) {
		if (glyphs[i].w > *max_w) (*max_w) = glyphs[i].w;
		if (glyphs[i].h > *max_h) (*max_h) = glyphs[i].h;
		(*net_area) += glyphs[i].w * glyphs[i].h;
		(*gross_area) += (glyphs[i].w + FONT_CHARACTER_SPACING)
		               * (glyphs[i].h + FONT_CHARACTER_SPACING);
	}
	return ret;

}



/* Tries to find a texture that will fit the font
 */
static BITMAP* look_for_texture(int beg, int end, AGL_GLYPH *glyphs,
			int max_w, int max_h, int total_area, GLint format, int has_alpha) {

	BITMAP *bmp = NULL;
	int i, j;

	/* Max texture size (1 << n) */
	/* XXX <rohannessian> We should use ARB_np2 if we can
	 *
	 * Other note: w*h shouldn't exceed 31 bits; otherwise, we get funny
	 * behavior on 32-bit architectures. Limit texture sizes to 32k*32k
	 * (30 bits).
	 */
#define MIN_TEXTURE_SIZE 2
#define NUM_TEXTURE_SIZE 13
	texture_size texture_sizes[NUM_TEXTURE_SIZE * NUM_TEXTURE_SIZE];

	/* Set up texture sizes */
	for (i = 0; i < NUM_TEXTURE_SIZE; i++) {
		for (j = 0; j < NUM_TEXTURE_SIZE; j++) {
			texture_sizes[j + i * NUM_TEXTURE_SIZE].w =
			                                        1 << (j + MIN_TEXTURE_SIZE);
			texture_sizes[j + i * NUM_TEXTURE_SIZE].h =
			                                        1 << (i + MIN_TEXTURE_SIZE);
		}
	}

	/* Sort texture sizes by area */
	qsort(texture_sizes, NUM_TEXTURE_SIZE * NUM_TEXTURE_SIZE,
	                                      sizeof(texture_size), &sort_textures);

	for (i = 0; i < NUM_TEXTURE_SIZE * NUM_TEXTURE_SIZE; i++) {
		int num_channels;

		/* Check the area - it must be larger than
		 * all the glyphs
		 */
		texture_size *t = &texture_sizes[i];
		int area = t->w * t->h;
		int depth = 24;

		if (area < total_area) {
			continue;
		}

		/* Check against max values */
		if ((t->h < max_h) || (t->w < max_w)) {
			continue;
		}

		TRACE(PREFIX_I "look_for_texture: candidate size: %ix%i\n", t->w, t->h);

		/* Check that the texture can, in fact, be created */
		num_channels = __allegro_gl_get_num_channels(format);
		if (num_channels == 1) {
			depth = 8;
		}
		else if (num_channels == 4) {
			depth = 32;
		}
		else {
			depth = 24;
		}
		bmp = create_bitmap_ex(depth, t->w, t->h);

		if (!bmp) {
			TRACE(PREFIX_W "look_for_texture: Out of memory while "
			      "creating bitmap\n");
			continue;
		}

		if (!aglf_check_texture(bmp, format, has_alpha)) {
			TRACE(PREFIX_I "look_for_texture: Texture rejected by driver\n");
			destroy_bitmap(bmp);
			bmp = NULL;
			continue;
		}

		/* Sort out the glyphs */
		TRACE(PREFIX_I "look_for_texture: Sorting on bmp: %p, beg: %i, "
		      "end: %i\n", bmp, beg, end);

		if (aglf_sort_out_glyphs(bmp, glyphs, beg, end) == TRUE) {
			/* Success? */
			return bmp;
		}

		/* Failure? Try something else */
		TRACE(PREFIX_I "look_for_texture: Conversion failed\n");
		destroy_bitmap(bmp);
		bmp = NULL;
	}

	return NULL;
}



#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
/* This is only used to render chars from an Allegro font which has the
 * font_vtable_trans vtable. If the target is an 8-bit bitmap, only the alpha
 * channel is used. Otherwise, blit is used, to preserve the alpha channel.
 */
static int dummy_render_char(AL_CONST FONT* f, int ch, int fg, int bg,
	BITMAP* bmp, int x, int y)
{
	FONT_COLOR_DATA* cf = (FONT_COLOR_DATA*)(f->data);
	BITMAP *glyph = NULL;

	while(cf) {
		if(ch >= cf->begin && ch < cf->end) {
			glyph = cf->bitmaps[ch - cf->begin];
			break;
		}
		cf = cf->next;
	}

	if (glyph)
	{
		if (bitmap_color_depth(bmp) == 8) {
			int gx, gy;
			for (gy = 0; gy < bmp->h; gy++) {
				for (gx = 0; gx < bmp->w; gx++) {
					int c = getpixel(glyph, gx, gy);
					int a = geta(c);
					putpixel(bmp, x + gx, y + gy, a);
				}
			}
		}
		else
			blit(glyph, bmp, 0, 0, x, y, glyph->w, glyph->h);
		return bmp->w;
	}
	return 0;
}
#endif



/* Function to draw a character in a bitmap for conversion */
static int draw_glyphs(BITMAP *bmp, FONT *f, GLint format, int beg, int end,
                       AGL_GLYPH *glyphs) {
	char buf[32];
	int i, j;

#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
	if (bitmap_color_depth(bmp) == 8 && f->vtable != font_vtable_trans) {
#else
	if (bitmap_color_depth(bmp) == 8) {
#endif
		/* Generate an alpha font */
		BITMAP *rgbbmp = create_bitmap_ex(24, bmp->w, bmp->h);

		if (!rgbbmp) {
			TRACE(PREFIX_E "convert_allegro_font_to_texture: "
			      "Ran out of memory while creating %ix%ix%i bitmap!\n",
			      bmp->w, bmp->h, 24);
			return FALSE;
		}

		clear_bitmap(rgbbmp);

		for (i = 0; i < end - beg; i++) {
			usetc(buf + usetc(buf, glyphs[i].glyph_num + beg), 0);

			textout_ex(rgbbmp, f, buf, glyphs[i].x - glyphs[i].offset_x,
			                          glyphs[i].y - glyphs[i].offset_y, -1, -1);
		}

		/* Convert back to 8bpp */
		for (j = 0; j < bmp->h; j++) {
			for (i = 0; i < bmp->w; i++) {
				int pix = _getpixel24(rgbbmp, i, j);
				int r = getr24(pix);
				int g = getg24(pix);
				int b = getb24(pix);
				int gray = (r * 77 + g * 150 + b * 28 + 255) >> 8;
				_putpixel(bmp, i, j, MID(0, gray, 255));
			}
		}
		destroy_bitmap(rgbbmp);
	}
	else {
#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
		int (*borrowed_color_vtable)(AL_CONST FONT*, int, int, int, BITMAP*, int, int) = NULL;

		//In order to keep the alpha channel in textout_ex we borrow
		//the color font vtable which uses maked_blit() instead of
		//draw_trans_sprite() to draw glyphs.
		if (f->vtable == font_vtable_trans) {
			borrowed_color_vtable = f->vtable->render_char;
			f->vtable->render_char = dummy_render_char;
		}
#endif

		if (__allegro_gl_get_num_channels(format) == 4) {
			clear_to_color(bmp, bitmap_mask_color(bmp));
		}
		else {
			clear_bitmap(bmp);
		}

		for (i = 0; i < end - beg; i++) {
			usetc(buf + usetc(buf, glyphs[i].glyph_num + beg), 0);
			textout_ex(bmp, f, buf, glyphs[i].x - glyphs[i].offset_x,
			         glyphs[i].y - glyphs[i].offset_y, -1, -1);
		}

#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
		if (borrowed_color_vtable) {
			f->vtable->render_char = borrowed_color_vtable;
		}
#endif
	}

	return TRUE;
}



/* Converts a single font range to a texture.
 * dest - Receives the result.
 * f - The original font.
 * src - The original font data.
 */
static void aglf_convert_allegro_font_to_texture(FONT_AGL_DATA **dest, FONT *f,
                            void *src, int *height, float scale, GLint format) {

	int max = 0;
	BITMAP *bmp = NULL;
	int beg = 0, end = 0;
	int max_w, max_h;
	int total_area, gross_area;

	AGL_GLYPH *glyph_coords;

	union mixed_ptr dat;
	dat.ptr = src;

	if (is_mono_font(f)) {
		beg = dat.mf->begin;
		end = dat.mf->end;
		max = dat.mf->end - dat.mf->begin;
		if (format == -1) {
			format = GL_INTENSITY4;
		}
	}
	else if (is_color_font(f)) {
		beg = dat.cf->begin;
		end = dat.cf->end;
		max = dat.cf->end - dat.cf->begin;
		if (format == -1) {
#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
			format = (f->vtable == font_vtable_trans ? GL_RGBA8 : GL_RGB8);
#else
			format = GL_RGB8;
#endif
		}
	}

	/* Allocate glyph sizes */
	glyph_coords = malloc(max * sizeof(AGL_GLYPH));
	memset(glyph_coords, 0, max * sizeof(AGL_GLYPH));

	if (crop_font_range(f, dat.ptr, beg, end, glyph_coords,
	                      &total_area, &gross_area, &max_w, &max_h) == FALSE) {
		TRACE(PREFIX_I "convert_allegro_font_to_texture: Unable to crop font "
		      "range\n");
		free(glyph_coords);
		return;
	}

	TRACE(PREFIX_I "convert_allegro_font_to_texture: Total area of glyphs: "
	      "%i pixels (%i pixels gross) - max_w: %i, max_h: %i\n",
	      total_area, gross_area, max_w, max_h);

	/* Sort glyphs by width, then height */
	qsort(glyph_coords, end - beg, sizeof(AGL_GLYPH), &sort_glyphs);


	/* Now, we look for the appropriate texture size */
	bmp = look_for_texture(beg, end, glyph_coords, max_w, max_h,
	                       total_area, format, (*dest)->has_alpha);

	/* No texture sizes were found - we should split the font up */
	if (!bmp) {
		int height1;
		union mixed_ptr f1, f2;
		FONT_AGL_DATA *dest1, *dest2;

		free(glyph_coords);

		dest1 = *(dest);
		dest2 = malloc(sizeof(FONT_AGL_DATA));

		if (!dest2) {
			TRACE(PREFIX_E "convert_allegro_font_to_texture: "
			      "Out of memory while trying to allocate %i bytes.\n",
			      (int)sizeof(FONT_AGL_DATA));
			return;
		}

		memset(dest2, 0, sizeof(FONT_AGL_DATA));

		dest2->next = dest1->next;
		dest1->next = dest2;
		dest2->is_free_chunk = TRUE;
		dest2->format = dest1->format;
		dest2->has_alpha = dest1->has_alpha;

		if (split_font(f, dat.ptr, &f1.ptr, &f2.ptr) == FALSE) {
			TRACE(PREFIX_E "convert_allegro_font_to_texture: Unable "
			      "to split font!\n");
			dest1->next = dest2->next;
			free(dest2);
			return;
		}

		aglf_convert_allegro_font_to_texture(&dest1, f, f1.ptr, height, scale,
		                                     format);
		height1 = (*height);
		aglf_convert_allegro_font_to_texture(&dest2, f, f2.ptr, height, scale,
		                                     format);
		destroy_split_font(f, f1, f2);

		if (height1 > (*height))
			(*height) = height1;
		(*dest) = dest2;

		return;
	}

	TRACE(PREFIX_I "convert_allegro_font_to_texture: Using texture "
	      "%ix%ix%i for font conversion.\n", bmp->w, bmp->h,
	      bitmap_color_depth(bmp));

	/* Now that all the glyphs are in place, we draw them into the bitmap */
	if (draw_glyphs(bmp, f, format, beg, end, glyph_coords) == FALSE) {
		destroy_bitmap(bmp);
		free(glyph_coords);
		return;
	}

	/* Un-Sort glyphs  */
	qsort(glyph_coords, end - beg, sizeof(AGL_GLYPH), &unsort_glyphs);

#if (defined SAVE_FONT_SCREENSHOT)
		save_shot(bmp);
#endif

	(*dest)->list_base =
	         create_textured_font_call_lists(glyph_coords, max, bmp,
	                                         scale, height);

	(*dest)->texture = aglf_upload_texture(bmp, format, (*dest)->has_alpha);
	(*dest)->type = AGL_FONT_TYPE_TEXTURED;
	(*dest)->format = format;
	(*dest)->scale = scale;
	(*dest)->start = beg;
	(*dest)->end = end;
	(*dest)->data = bmp;
	(*dest)->glyph_coords = glyph_coords;

	return;
}



static void aglf_convert_allegro_font_to_bitmap(FONT_AGL_DATA *dest, FONT *f,
                                                       void *src, int *height) {

	int max = 0;
	int i, j, k;
	int beg = 0, end = 0;
	int mask;
	FONT_GLYPH **glyph;

	union {
		FONT_MONO_DATA* mf;
		FONT_COLOR_DATA* cf;
		void *ptr;
	} dat;

	dat.ptr = src;

	if (is_mono_font(f))
		max = dat.mf->end - dat.mf->begin;
	else if (is_color_font(f))
		max = dat.cf->end - dat.cf->begin;
	else
		return;

	glyph = malloc(sizeof(FONT_GLYPH*) * max);

	if (!glyph) {
		TRACE(PREFIX_E "convert_allegro_font_to_bitmap: Ran out of "
		      "memory while allocating %i bytes\n", (int)sizeof(FONT_GLYPH));
		return;
	}

	*height = f->height;

	if (is_mono_font(f)) {

		/* for each glyph */
		for (i = 0; i < max; i++) {
			FONT_GLYPH *oldgl = dat.mf->glyphs[i];

			int size = sizeof(FONT_GLYPH) + ((oldgl->w + 31) / 32) * 4
			                                                         * oldgl->h;

			/* create new glyph */
			FONT_GLYPH *newgl = (FONT_GLYPH*)malloc(size);

			if (!newgl)
				break;

			memset(newgl, 0, size);

			newgl->w = oldgl->w;
			newgl->h = oldgl->h;

			/* update the data */
			for (j = 0; j < oldgl->h; j++) {
				for (k = 0; k < ((oldgl->w + 7) / 8); k++) {
					int addr = (oldgl->h - j - 1) * ((oldgl->w + 31) / 32) * 4
					           + k;
					newgl->dat[addr] = oldgl->dat[j * ((oldgl->w + 7) / 8) + k];
				}
			}

			glyph[i] = newgl;
		}
	}
	/* Reduce to 1 bit */
	else if (is_color_font(f)) {
		/* for each glyph */
		for (i = 0; i < max; i++) {

			int size;
			BITMAP *oldgl = dat.cf->bitmaps[i];
			FONT_GLYPH *newgl;

			mask = bitmap_mask_color(oldgl);

			size = sizeof(FONT_GLYPH) + ((oldgl->w + 31) / 32) * 4 * oldgl->h;

			/* create new glyph */
			newgl = (FONT_GLYPH*)malloc(size);

			if (!newgl)
				break;

			memset(newgl, 0, size);

			newgl->w = oldgl->w;
			newgl->h = oldgl->h;

			/* update the data */
			for (j = 0; j < oldgl->h; j++) {
				for (k = 0; k < oldgl->w; k++) {
					int addr = (oldgl->h - j - 1) * ((oldgl->w + 31) / 32) * 4
					         + (k / 8);
					newgl->dat[addr] |= (getpixel(oldgl, k, j) == mask)
					                 ? 0 : (1 << (k & 7));
				}
			}

			glyph[i] = newgl;
		}
	}
	/* Create call lists */
	{
		GLuint list = glGenLists(max);

		for (i = 0; i < max; i++) {
			glNewList(list + i, GL_COMPILE);

			glBitmap(glyph[i]->w, glyph[i]->h, 0, 0, glyph[i]->w, 0,
			         glyph[i]->dat);

			glEndList();
		}
		dest->list_base = list;
	}

	dest->is_free_chunk = 0;
	dest->type = AGL_FONT_TYPE_BITMAP;
	dest->start = beg;
	dest->end = end;
	dest->data = glyph;

	return;
}



static int aglf_check_texture(BITMAP *bmp, GLint format, int has_alpha) {

	int flags = AGL_TEXTURE_FLIP | AGL_TEXTURE_MIPMAP;

	if (format == GL_ALPHA4 || format == GL_ALPHA8 || format == GL_ALPHA
	 || format == GL_INTENSITY4 || format == GL_INTENSITY8
	 || format == GL_INTENSITY
	 || format == GL_LUMINANCE4 || format == GL_LUMINANCE8
	 || format == GL_LUMINANCE
	 || format == 1) {
		flags |= AGL_TEXTURE_ALPHA_ONLY;
	}
	else if (format == GL_RGBA8) {
		if (has_alpha) {
			flags |= AGL_TEXTURE_HAS_ALPHA;
		}
		else {
			flags |= AGL_TEXTURE_MASKED;
		}
	}

	return allegro_gl_check_texture_ex(flags, bmp, format);
}



static GLuint aglf_upload_texture(BITMAP *bmp, GLint format, int has_alpha) {

	int flags = AGL_TEXTURE_FLIP | AGL_TEXTURE_MIPMAP;
	GLuint texture;

	if (format == GL_ALPHA4 || format == GL_ALPHA8 || format == GL_ALPHA
	 || format == GL_INTENSITY4 || format == GL_INTENSITY8
	 || format == GL_INTENSITY
	 || format == GL_LUMINANCE4 || format == GL_LUMINANCE8
	 || format == GL_LUMINANCE
	 || format == 1) {
		flags |= AGL_TEXTURE_ALPHA_ONLY;
	}
	else if (__allegro_gl_get_num_channels(format) == 4) {
		if (has_alpha) {
			flags |= AGL_TEXTURE_HAS_ALPHA;
		}
		else {
			flags |= AGL_TEXTURE_MASKED;
		}
	}

	TRACE(PREFIX_I "Want texture format: %s\n",
		__allegro_gl_get_format_description(format));
	texture = allegro_gl_make_texture_ex(flags, bmp, format);
	TRACE(PREFIX_I "Texture ID is: %u\n", texture);

	return texture;
}

