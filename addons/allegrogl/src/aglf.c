/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file aglf.c
 *  \brief Text output and font support in OpenGL.
 */

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "allegro.h"

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

#define PREFIX_E "agl-font ERROR: "



static int aglf_font_generation_mode = AGL_FONT_POLYGONS;


/* find_range:
 *  Searches a font for a specific character.
 */
static AL_CONST FONT_AGL_DATA *find_range(AL_CONST FONT_AGL_DATA *f, int c) {

	while (f) {
		if ((c >= f->start) && (c < f->end))
			return f;

		f = f->next;
	}

	return NULL;
}



/* allegro_gl_printf(FONT *f, float x, float y, float z, int color,
			char *format, ...) */
/** Equivalent to:
 *  <pre>
 *   r = getr(color);
 *   g = getg(color);
 *   b = getb(color);
 *   a = geta(color);
 *   glColor4f(r, g, b, a);
 *   allegro_gl_printf_ex(f, x, y, z,
 *                        format, ...);
 *  </pre>
 *
 *  Note that the current primary color is not preserved.
 */
int allegro_gl_printf(AL_CONST FONT *f, float x, float y, float z, int color,
                      AL_CONST char *format, ...) {

#define BUF_SIZE 1024
	char buf[BUF_SIZE];
	va_list ap;
	
	if (!__allegro_gl_valid_context)
		return 0;
	
	/* Get the string */
	va_start(ap, format);
		uvszprintf(buf, BUF_SIZE, format, ap);
	va_end(ap);

#undef BUF_SIZE

	/* Set color */
	{
		GLubyte c[4];
		c[0] = (GLubyte)getr(color);
		c[1] = (GLubyte)getg(color);
		c[2] = (GLubyte)getb(color);
		c[3] = (__allegro_gl_use_alpha && bitmap_color_depth(screen) == 32)
		     ? (GLubyte)geta(color) : 255;

		glColor4ubv(c);
	}

	return allegro_gl_printf_ex(f, x, y, z, buf);
}



/* allegro_gl_printf_ex(FONT *f, float x, float y, float z,
 *                      char *format, ...)
 */
/** Prints a formatted string (printf style) on the screen.
 *
 *  \param f       Which font to use.
 *  \param x,y,z   Coordinates to print at. They specify the top-left
 *                 corner of the text position.
 *  \param format  The format string (see printf() for details)
 * 
 *  For bitmap fonts, the raster position is set to (x,y),
 *  'z' is ignored.
 *  The current modelview matrix applies to this code so you may want to
 *  use glLoadIdentity() beforehand. On the other hand, you can use the
 *  modelview matrix to apply transformations to the text. This will only work
 *  with textured or vector (outline) fonts.
 *  This function only accepts AllegroGL formated fonts, as converted by
 *  allegro_gl_convert_allegro_font(), or loaded by
 *  allegro_gl_load_system_font() or allegro_gl_load_system_font_ex().
 *
 *  Texturing must be enabled for this function to work with
 *  #AGL_FONT_TYPE_TEXTURED fonts.
 *
 *  Remember to use <code>glEnable(GL_TEXTURE_2D)</code> to enable texturing.
 *
 *  The resulting size may not be what you expect. For bitmaped fonts,
 *  there is nothing you can do about this appart changing the font itself.
 *  Textured and Vector fonts are more flexible, in that you can use
 *  glScale to adjust the size of the characters.
 *
 *  If you need to draw the text without the black backround, we suggest
 *  you set up a proper blending mode prior to drawing the text, such as:
 *   <pre>
 *     glEnable(GL_BLEND);
 *     glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
 *   </pre>
 *
 *  If you want to emulate the Allegro drawing mode (no blending at all),
 *  then you should use the folowing code instead:
 *
 *   <pre>
 *     glDisable(GL_DEPTH_TEST);
 *     glEnable(GL_BLEND);
 *
 *     glBlendFunc(GL_DST_COLOR, GL_ZERO);
 *     allegro_gl_printf();
 *
 *     glBlendFunc(GL_ONE, GL_ONE);
 *     allegro_gl_printf();    // Same as the one above!
 *
 *     glEnable(GL_DEPTH_TEST);
 *   </pre>
 *
 *  Have a look at NeHe's Tutorial #20 for details on this technique.
 *  http://nehe.gamedev.net/
 *
 *  The most flexible way to use fonts, though, is to use alpha textures
 *  based on a greyscale font.  Set the texture format to
 *  <code>GL_ALPHA4</code> or <code>GL_ALPHA8</code> before creating the
 *  (textured) font.  Then you can set the colour and blend modes like so:
 *
 *  <pre>
 *    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 *    allegro_gl_printf_ex(my_font, x, y, z, "Hi!")
 *  </pre>
 *
 *  \return The number of characters printed.
 */
int allegro_gl_printf_ex(AL_CONST FONT *f, float x, float y, float z,
                         AL_CONST char *format, ...) {
	#define BUF_SIZE 1024
	char buf[BUF_SIZE];
	va_list ap;
	
	AL_CONST FONT_AGL_DATA *range = NULL;
	int c, pos = 0;
	int count = 0;
	AL_CONST FONT_AGL_DATA *d;
	GLint vert_order, cull_mode;
	GLint matrix_mode;

	int restore_rasterpos = 0;
	GLuint old_texture_bind = 0;
	GLfloat old_raster_pos[4];
	

	if (!__allegro_gl_valid_context)
		return 0;

	/* Check arguments */
	if (!format || !f) {
		TRACE(PREFIX_E "agl_printf: Null parameter\n");
		return 0;
	}
		
	if (f->vtable != font_vtable_agl) {
		TRACE(PREFIX_E "agl_printf: Font parameter isn't of the AGL "
		      "type.\n");
		return 0;
	}
		
	d = (AL_CONST FONT_AGL_DATA*)f->data;

	/* Get the string */
	va_start(ap, format);
		uvszprintf(buf, BUF_SIZE, format, ap);
	va_end(ap);

#undef BUF_SIZE

	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glGetIntegerv(GL_FRONT_FACE, &vert_order);
	glGetIntegerv(GL_CULL_FACE_MODE, &cull_mode);	
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	{	GLint temp;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &temp);
		old_texture_bind = (GLuint)temp;
	}

	if (d->type == AGL_FONT_TYPE_BITMAP) {
		glTranslatef(0, 0, -1);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glGetFloatv(GL_CURRENT_RASTER_POSITION, old_raster_pos);
		glRasterPos2f(x, y);
		restore_rasterpos = 1;
	}
	else if (d->type == AGL_FONT_TYPE_OUTLINE) {
		glTranslatef(x, y, z);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else if (d->type == AGL_FONT_TYPE_TEXTURED) {
		glTranslatef(x, y, z);
	}


	while ((c = ugetc(buf + pos)) != 0) {

		pos += ucwidth(c);

		if ((!range) || (c < range->start) || (c >= range->end)) {
			/* search for a suitable character range */
			range = find_range(d, c);

			if (!range) {
				range = find_range(d, (c = '^'));

				if (!range)
					continue;
			}
		}
		
		/* Set up texture */
		if (d->type == AGL_FONT_TYPE_TEXTURED) {
			glBindTexture(GL_TEXTURE_2D, range->texture);
		}
		
		/* draw the character */
		c -= range->start;
		c += range->list_base;
		
		glCallList(c);

		count++;
	}
	
	glPopMatrix();

	glMatrixMode(matrix_mode);
	glFrontFace(vert_order);
	glCullFace(cull_mode);	

	glBindTexture(GL_TEXTURE_2D, old_texture_bind);

	if (restore_rasterpos) {
		glRasterPos4fv(old_raster_pos);
	}

	return count;
}



#ifndef ALLEGROGL_GENERIC_DRIVER
#ifdef ALLEGRO_WINDOWS

static FONT *win_load_system_font(char *name, int type, int style, int w, int h, float depth, int start, int end) {

	HFONT hFont;

	FONT_AGL_DATA *data;
	FONT *ret;
	
	ret = malloc(sizeof(FONT));
	if (!ret) {
		TRACE(PREFIX_E "win_load_system_font: Ran out of memory "
		      "while allocating %i bytes\n", sizeof(FONT));
		return NULL;
	}
	data = malloc(sizeof(FONT_AGL_DATA));
	if (!data) {
		free(ret);
		TRACE(PREFIX_E "win_load_system_font: Ran out of memory "
		      "while allocating %i bytes\n", sizeof(FONT_AGL_DATA));
		return NULL;
	}
	ret->vtable = font_vtable_agl;
	ret->data = data;
		
	data->list_base = glGenLists(end - start);
	data->start = start;
	data->end = end;
	data->next = NULL;
	data->is_free_chunk = 0;

	if (type == AGL_FONT_TYPE_BITMAP || type == AGL_FONT_TYPE_DONT_CARE) {

		HDC dc;

		hFont = CreateFont( -h, w,
			0, 0,
			(style & AGL_FONT_STYLE_BOLD) ? FW_BOLD
			         : ((style & AGL_FONT_STYLE_BLACK) ? FW_BLACK : FW_NORMAL),
			((style & AGL_FONT_STYLE_ITALIC) ? TRUE : FALSE),
			((style & AGL_FONT_STYLE_UNDERLINE) ? TRUE : FALSE),
			((style & AGL_FONT_STYLE_STRIKEOUT) ? TRUE : FALSE),
			ANSI_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			(style & AGL_FONT_STYLE_ANTI_ALIASED) ? ANTIALIASED_QUALITY
			         : DEFAULT_QUALITY,
			FF_DONTCARE | DEFAULT_PITCH,
			name);

		dc = GetDC(win_get_window());

		SelectObject(dc, hFont);

		wglUseFontBitmaps(dc, start, end - start, data->list_base);
		data->type = AGL_FONT_TYPE_BITMAP;
		data->data = NULL;
	}
	else if (type == AGL_FONT_TYPE_OUTLINE) {
		HDC dc;

		GLYPHMETRICSFLOAT *gmf;
		gmf = malloc(sizeof(GLYPHMETRICSFLOAT) * (end - start));
		memset(gmf, 0, sizeof(GLYPHMETRICSFLOAT) * (end - start));

		hFont = CreateFont( -h, w,
			0, 0,
			(style & AGL_FONT_STYLE_BOLD) ? FW_BOLD
			          : ((style & AGL_FONT_STYLE_BLACK) ? FW_BLACK : FW_NORMAL),
			((style & AGL_FONT_STYLE_ITALIC) ? TRUE : FALSE),
			((style & AGL_FONT_STYLE_UNDERLINE) ? TRUE : FALSE),
			((style & AGL_FONT_STYLE_STRIKEOUT) ? TRUE : FALSE),
			ANSI_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			(style & AGL_FONT_STYLE_ANTI_ALIASED) ? ANTIALIASED_QUALITY
			          : DEFAULT_QUALITY,
			FF_DONTCARE | DEFAULT_PITCH,
			name);

		dc = GetDC(win_get_window());

		SelectObject(dc, hFont);
		wglUseFontOutlines(dc, start, end - start, data->list_base,
		    0.0, depth, (aglf_font_generation_mode == AGL_FONT_POLYGONS)
		    ? WGL_FONT_POLYGONS : WGL_FONT_LINES, gmf);

		data->type = AGL_FONT_TYPE_OUTLINE;
		data->data = gmf;
	}

	return ret;
}
#endif



#ifdef ALLEGRO_WITH_XWINDOWS
static FONT *x_load_system_font(char *name, int type, int style, int w, int h,
                                              float depth, int start, int end) {
	FONT_AGL_DATA *data;
	FONT *ret;
	XFontStruct *xfont;

	ret = malloc(sizeof(FONT));
	if (!ret) {
		TRACE(PREFIX_E "x_load_system_font: Ran out of memory "
		      "while allocating %zi bytes\n", sizeof(FONT));
		return NULL;
	}
	data = malloc(sizeof(FONT_AGL_DATA));
	if (!data) {
		free(ret);
		TRACE(PREFIX_E "x_load_system_font: Ran out of memory "
		      "while allocating %zi bytes\n", sizeof(FONT_AGL_DATA));
		return NULL;
	}
	ret->vtable = font_vtable_agl;
	ret->data = data;
		
	data->list_base = glGenLists(end - start);
	data->start = start;
	data->end = end;
	data->next = NULL;
	data->is_free_chunk = 0;

	if (type == AGL_FONT_TYPE_BITMAP || type == AGL_FONT_TYPE_DONT_CARE) {
		char buf[256], major_type[256], minor_type[2];
		
		usprintf(major_type, "medium");
		if (style & AGL_FONT_STYLE_BOLD)
			usprintf(major_type, "bold");
		minor_type[0] = (style & AGL_FONT_STYLE_ITALIC) ? 'i' : 'r';
		minor_type[1] = '\0';
		
		usprintf(buf, "-*-%s-%s-%s-normal-*-%i-*-*-*-*-*-*-*", name,
		                                            major_type, minor_type, h);
		/* Load the font */
		xfont = XLoadQueryFont(_xwin.display, buf);
		if (!xfont) {
			free(ret);
			free(data);
			TRACE(PREFIX_E "x_load_system_font: Failed to load "
			      "%s\n", buf);
			return NULL;
		}
		glXUseXFont(xfont->fid, start, end - start, data->list_base);
		data->type = AGL_FONT_TYPE_BITMAP;
		data->data = NULL;
		XFreeFont(_xwin.display, xfont);
	}
	else {
		/* Not Yet Implemented */
		return NULL;
	}
	
	return ret;
}
#endif
#endif /* ALLEGROGL_GENERIC_DRIVER */



/* void allegro_gl_set_font_generation_mode(int mode) */
/** Set the font generation mode for system fonts.
 *
 *  \b Note: This function is deprecated and will be removed
 *  in a future version.
 * 
 *  \param mode Can be either #AGL_FONT_POLYGONS or #AGL_FONT_LINES.
 *              for creating polygonal or line characters.
 *              Default is #AGL_FONT_POLYGONS.
 *
 *  Subsequent calls to allegro_gl_load_system_font() and
 *  allegro_gl_load_system_font_ex() may be affected by this function.
 *
 *  \deprecated 
 */
void allegro_gl_set_font_generation_mode(int mode) {
	aglf_font_generation_mode = mode;
	return;
}



/* FONT *allegro_gl_load_system_font(char *name, int style, int w, int h) */
/** Short hand for aglf_load_system_font_ex(name, #AGL_FONT_TYPE_OUTLINE,
 *                                                   style, w, h, 0.0f, 32, 256)
 *
 *  \b Note: This function is deprecated and will be removed
 *  in a future version.
 *
 *  \deprecated 
 */
FONT *allegro_gl_load_system_font(char *name, int style, int w, int h) {

	return allegro_gl_load_system_font_ex(name, AGL_FONT_TYPE_OUTLINE,
	                                      style, w, h, 0.0f, 32, 256);
}



/* FONT *allegro_gl_load_system_font_ex(char *name, int type, int style,
			int w, int h, float depth, int start, int end) */
/** Loads a system font. 
 * 
 *  \b Note: This function is deprecated and will be removed
 *  in a future version.
 *
 *  \param name       The name of the system font ("Courrier" or "Arial"
 *                    for example)
 *  \param type       The font type to generate (#AGL_FONT_TYPE_DONT_CARE,
 *                    #AGL_FONT_TYPE_BITMAP, #AGL_FONT_TYPE_OUTLINE)
 *  \param style      The text decorations of the font to create
 *                    (#AGL_FONT_STYLE_ITALIC, etc - can be or'ed together)
 *  \param w,h        The size of the font characters that will be created.
 *  \param depth      The z-depth of the font. 0.0f is flat, 1.0f is very
 *                    thick
 *  \param start,end  The range of characters to create from 'start'
 *                    (included) to 'end' (excluded) in UTF-8 format
 *                    (ANSI only on Windows).
 *
 * \note
 *  - In #AGL_FONT_TYPE_OUTLINE type, some system fonts seem to be unresponsive
 *    to the size paramaters (w and h). You cannot depend on either a system
 *    font being present, or the font being what you expect - it is possible to
 *    have two fonts with the same name but with different graphic data.
 *  - BITMAP fonts have no depth.
 *  - The width and height parameters are also system dependent, and may or
 *    may not work with the selected font.
 *
 *  \return The loaded font, or NULL on error.
 *
 *  \deprecated
 */
FONT *allegro_gl_load_system_font_ex(char *name, int type, int style,
                                int w, int h, float depth, int start, int end) {

	FONT *ret = NULL;

	if (!__allegro_gl_valid_context)
		return NULL;

	if (!name) {
		TRACE(PREFIX_E "load_system_font: Nameless font\n");
		return NULL;
	}

	/* Load a system font */

#ifndef ALLEGROGL_GENERIC_DRIVER
#ifdef ALLEGRO_WINDOWS
	ret = win_load_system_font(name, type, style, w, h, depth, start, end);
#elif defined ALLEGRO_UNIX
	XLOCK();
	ret = x_load_system_font(name, type, style, w, h, depth, start, end);
	XUNLOCK();
#else
	/* Other platform */
#endif
#endif

	return ret;
}



/** void allegro_gl_destroy_font(FONT *usefont) */
/** Destroys the font.
 *
 *  \param f The AGL font to be destroyed.
 *
 *  The allocated memory is freed, as well as any display list, or texture
 *  objects it uses.
 *  
 *  You cannot use that font anymore after calling this function.
 *  It's safe to call this function with a NULL pointer; a note will be
 *  placed in allegro.log if DEBUGMODE was defined.
 *
 *  If NULL is passed as the font to destroy, then this function returns
 *  immediately.
 */
void allegro_gl_destroy_font(FONT *f) {

	FONT_AGL_DATA *data;

	if (!f) {
		return;
	}
	if (f->vtable != font_vtable_agl) {
		TRACE(PREFIX_E "destroy_font: Font is not of AGL type\n");	
		return;
	}
	
	data = f->data;
	
	if (!data) {
		TRACE(PREFIX_E "destroy_font: Font is inconsistent\n");	
		return;
	}

	/* Iterate through every segment of the font */
	while (data) {	
		FONT_AGL_DATA *datanext;
		
		/* Release all resources taken up by this font */
		if (data->type == AGL_FONT_TYPE_BITMAP
		 || data->type == AGL_FONT_TYPE_OUTLINE
		 || data->type == AGL_FONT_TYPE_TEXTURED) {

			if (__allegro_gl_valid_context) {
				if (data->list_base)
					glDeleteLists(data->list_base, data->end - data->start);
				if (data->texture)
					glDeleteTextures(1, &data->texture);
			}
		}
		if (data->type == AGL_FONT_TYPE_OUTLINE) {
			if (data->data) 
				free(data->data);
		}
		else if (data->type == AGL_FONT_TYPE_TEXTURED) {
			if (data->data)
				destroy_bitmap(data->data);
			if (data->glyph_coords)
				free(data->glyph_coords);
		}
		else if (data->type == AGL_FONT_TYPE_BITMAP) {
			if (data->data) {
				int i;
				FONT_GLYPH **gl = data->data;
				for (i = 0; i < data->end - data->start; i++) {
					if (gl[i])
						free(gl[i]);
				}
				free(gl);
			}
		}
		datanext = data->next;

		if (data->is_free_chunk)
			free(data);
			
		data = datanext;
	}
	free(f->data);

	if (f != font)
		free(f);
	
	return;
}



/* size_t allegro_gl_list_font_textures(FONT *f, GLuint *ids, size_t max_num_id) */
/** List the texture ID of all textures forming the specified font.
 *
 *  The font specified must be an AllegroGL font.
 *
 *  If ids is not NULL, then the ID numbers of all textures used by the font
 *  are written to the GLuint array pointed by ids. The size of that array
 *  is specified by the max_num_id parameter. This function will never write
 *  more than 'max_num_id' values in the ids array.
 *
 *  If f is NULL, then zero is returned and the ids array is never touched.
 *
 *  If the font does not contain any textures (because it is a bitmap or outline
 *  font, for example), then zero is returned.
 *
 *  \return Number of texture IDs that make up this font.
 *
 *  Here are two examples of the use of this function:
 *
 * <pre>
 *   int num_ids = allegro_gl_list_font_textures(font, NULL, 0);
 *
 *   GLuint *id = malloc(sizeof(GLuint) * num_ids);
 *
 *   if (!id) {
 *     //handle error
 *   }
 *
 *   allegro_gl_list_font_textures(font, id, num_ids);
 *
 *   for (i = 0; i < num_ids; i++) {
 *       glBindTexture(GL_TEXTURE_2D, id[i]);
 *       // Use this texture
 *   }
 *
 *   free(id);
 * </pre>
 *
 * <pre>
 *   GLint id[10];  // Reserve a safe number
 *   GLint num_ids = allegro_gl_list_font_textures(font, id, 10);
 * </pre>
 */
size_t allegro_gl_list_font_textures(FONT *f, GLuint *ids, size_t max_num_id) {

	size_t num_ids = 0;
	FONT_AGL_DATA *data;

	if (!f) {
		return 0;
	}
	if (f->vtable != font_vtable_agl) {
		TRACE(PREFIX_E "list_font_textures: Font is not of AGL type\n");	
		return 0;
	}
	
	data = f->data;
	
	if (!data) {
		TRACE(PREFIX_E "list_font_textures: Font is inconsistent\n");	
		return 0;
	}

	if (!__allegro_gl_valid_context) {
		return 0;
	}

	/* Iterate through all font segments */
	while (data) {
		if (data->texture) {
			/* Add the texture ID in the array, if it's not NULL and if there
			 * is room.
			 */
			if (ids && num_ids < max_num_id) {
				ids[num_ids] = data->texture;
			}
			num_ids++;
		}

		data = data->next;
	}
	
	return num_ids;
}

