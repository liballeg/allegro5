/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file texture.c
  * \brief AllegroGL texture management.
  */

#include <string.h>

#include "alleggl.h"
#include "allglint.h"

#include <allegro/internal/aintern.h>

#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

static GLint allegro_gl_opengl_internal_texture_format = -1;
static int allegro_gl_use_mipmapping_for_textures = 0;
int __allegro_gl_use_alpha = FALSE;
int __allegro_gl_flip_texture = TRUE;
GLint __allegro_gl_texture_read_format[5];
GLint __allegro_gl_texture_components[5];

#define PREFIX_I                "agl-tex INFO: "
#define PREFIX_W                "agl-tex WARNING: "
#define PREFIX_E                "agl-tex ERROR: "


/** \internal
 *  \ingroup texture
 *
 *  Returns a string for the given OpenGL texture format.
 */
char const *__allegro_gl_get_format_description(GLint format)
{
	static char str[256];
#define F(s) case s: return #s
	switch (format) {
		F(GL_ALPHA);
		F(GL_ALPHA4);
		F(GL_ALPHA8);
		F(GL_ALPHA12);
		F(GL_ALPHA16);
		F(GL_ALPHA16F_ARB);
		F(GL_ALPHA32F_ARB);
		F(GL_INTENSITY);
		F(GL_INTENSITY4);
		F(GL_INTENSITY8);
		F(GL_INTENSITY12);
		F(GL_INTENSITY16);
		F(GL_INTENSITY16F_ARB);
		F(GL_INTENSITY32F_ARB);
		F(GL_LUMINANCE);
		F(GL_LUMINANCE4);
		F(GL_LUMINANCE8);
		F(GL_LUMINANCE12);
		F(GL_LUMINANCE16);
		F(GL_LUMINANCE16F_ARB);
		F(GL_LUMINANCE32F_ARB);
		F(GL_LUMINANCE_ALPHA);
		F(GL_LUMINANCE4_ALPHA4);
		F(GL_LUMINANCE12_ALPHA4);
		F(GL_LUMINANCE8_ALPHA8);
		F(GL_LUMINANCE6_ALPHA2);
		F(GL_LUMINANCE12_ALPHA12);
		F(GL_LUMINANCE16_ALPHA16);
		F(GL_LUMINANCE_ALPHA16F_ARB);
		F(GL_LUMINANCE_ALPHA32F_ARB);
		F(GL_RGB);
		F(GL_R3_G3_B2);
		F(GL_RGB4);
		F(GL_RGB5);
		F(GL_RGB8);
		F(GL_RGB10);
		F(GL_RGB12);
		F(GL_RGB16);
		F(GL_RGB16F_ARB);
		F(GL_RGB32F_ARB);
		F(GL_RGBA);
		F(GL_RGBA2);
		F(GL_RGBA4);
		F(GL_RGB5_A1);
		F(GL_RGBA8);
		F(GL_RGB10_A2);
		F(GL_RGBA12);
		F(GL_RGBA16);
		F(GL_RGBA16F_ARB);
		F(GL_RGBA32F_ARB);
	}
	uszprintf(str, sizeof str, "%x", format);
	return str;
#undef F
}



/* int __allegro_gl_get_num_channels(GLenum format) */
/** \internal
 *  \ingroup texture
 *
 *  Returns the number of color components for the given color fromat.
 * 
 *  \param format Internal color format.
 *
 *  \return The number of color components.
 */
int __allegro_gl_get_num_channels(GLenum format) {
		
	switch (format) {
	case 1:
	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA8:
	case GL_ALPHA12:
	case GL_ALPHA16:
	case GL_ALPHA16F_ARB:
	case GL_ALPHA32F_ARB:
	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY8:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
	case GL_INTENSITY16F_ARB:
	case GL_INTENSITY32F_ARB:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
	case GL_LUMINANCE16F_ARB:
	case GL_LUMINANCE32F_ARB:
		return 1;
	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE8_ALPHA8:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
	case GL_LUMINANCE_ALPHA16F_ARB:
	case GL_LUMINANCE_ALPHA32F_ARB:
		return 2;
	case 3:
	case GL_RGB:
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
	case GL_RGB16F_ARB:
	case GL_RGB32F_ARB:
		return 3;
	case 4:
	case GL_RGBA:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA16F_ARB:
	case GL_RGBA32F_ARB:
		return 4;
	default:
		return 0;
	}
}



/* GLint __allegro_gl_get_texture_format_ex(BITMAP *bmp, int flags) */
/** \internal
 *  \ingroup texture
 *
 *  Returns the OpenGL internal texture format for this bitmap.
 * 
 *  \param bmp The bitmap to get the information of.
 *  \param flags The flags to use. See allegro_gl_make_texture_ex() for details.
 *
 *  \return The OpenGL internal texture format.
 */
GLint __allegro_gl_get_texture_format_ex(BITMAP *bmp, int flags) {

	if (!bmp) {
		return -1;
	}

	switch (bitmap_color_depth(bmp)) {
		case 32:
			if (flags
			     & (AGL_TEXTURE_HAS_ALPHA | AGL_TEXTURE_FORCE_ALPHA_INTERNAL)) {
				return GL_RGBA8;
			}
			else {
				return GL_RGB8;
			}
		case 8:
			return GL_INTENSITY8;
		case 15:
			if (flags & AGL_TEXTURE_FORCE_ALPHA_INTERNAL) {
				return GL_RGB5_A1;
			}
			else {
				return GL_RGB5;
			}
		case 16:
		case 24:
			if (flags & AGL_TEXTURE_FORCE_ALPHA_INTERNAL) {
				return GL_RGBA8;
			}
			else {
				return GL_RGB8;
			}
		default:
			return -1;
	}

	return -1;
}



/* GLint allegro_gl_get_texture_format(BITMAP *bmp) */
/** \ingroup texture
 *  Returns the OpenGL internal texture format for this bitmap.
 *  If left to default, it returns the number of color components
 *  in the bitmap. Otherwise, it simply returns the user defined
 *  value. If the bitmap parameter is NULL, then it returns the
 *  currently set format, or -1 if none were previously set.
 *  8 bpp bitmaps are assumed to be alpha channels only (GL_ALPHA8)
 *  by default.
 *
 *  \sa allegro_gl_set_texture_format(), allegro_gl_make_texture()
 * 
 *  \param bmp The bitmap to get the information of.
 *  \return The OpenGL internal texture format.
 *
 *  \deprecated
 */
GLint allegro_gl_get_texture_format(BITMAP *bmp) {

	if (bmp && allegro_gl_opengl_internal_texture_format == -1) {
		return __allegro_gl_get_texture_format_ex(bmp,
		        __allegro_gl_use_alpha ? AGL_TEXTURE_FORCE_ALPHA_INTERNAL : 0);
	}
	
	return allegro_gl_opengl_internal_texture_format;
}



/* GLint allegro_gl_set_texture_format(GLint format) */
/** \ingroup texture
 *  Sets the color format you'd like OpenGL to use for its textures.
 *  You can pass any of the available GL_* constants that describe
 *  the internal format (GL_ALPHA4...GL_RGBA16). It is recommended
 *  that you check for the availability of the format from the client,
 *  using allegro_gl_opengl_version(). No checking will be done as to the
 *  validity of the format. The format is a recommendation to the driver,
 *  so there is no guarentee that the actual format used will be the one
 *  you chose - this is implementation dependant. This call will affect
 *  all subsequent calls to allegro_gl_make_texture() and
 *  allegro_gl_make_masked_texture().
 *
 *  To revert to the default AllegroGL format, pass -1 as the format.
 *
 *  \sa allegro_gl_get_texture_format()
 *
 *  \param format The OpenGL internal texture format.
 *  \return The previous value of the texture format.
 *
 *  \deprecated
 */
GLint allegro_gl_set_texture_format(GLint format) {

	GLint old = allegro_gl_opengl_internal_texture_format;
	allegro_gl_opengl_internal_texture_format = format;
	return old;
}



/* GLenum __allegro_gl_get_bitmap_type(BITMAP *bmp, int flags) */
/** \internal
 *  \ingroup texture
 *
 *  Returns the pixel packing format of the selected bitmap in OpenGL
 *  format. OpenGL 1.1 only understands color component sizes (bytes
 *  per color), whereas OpenGL 1.2+ can understand Allegro's packed
 *  formats (such as 16 bit color, with 5-6-5 bit components).
 *
 *  If the bitmap is not representable to the OpenGL implementation 
 *  (e.g. it cannot handle packed formats because it's earlier than 
 *  1.2 and does not have the extension) this function returns -1.
 *
 *  \sa allegro_gl_make_texture_ex()
 *
 *  \param bmp    The bitmap to get the OpenGL pixel type of.
 *  \param flags The conversion flags
 *
 *  \return The pixel type of the bitmap.
 */
GLenum __allegro_gl_get_bitmap_type(BITMAP *bmp, int flags) {

	int c = bitmap_color_depth(bmp);

	switch (c) {

		case 8:
			return __allegro_gl_texture_read_format[0];

		case 15:
			return __allegro_gl_texture_read_format[1];

		case 16:
			return __allegro_gl_texture_read_format[2];

		case 24:
			return __allegro_gl_texture_read_format[3];

		case 32:
			return __allegro_gl_texture_read_format[4];
	
		default:
			TRACE(PREFIX_E "get_bitmap_type: unhandled bitmap depth: %d\n",
			      c);
			return -1;
	}
}



/* GLenum __allegro_gl_get_bitmap_color_format(BITMAP *bmp, int flags) */
/** \internal
 *  \ingroup texture
 *
 *  Returns the pixel color format of the selected Allegro bitmap in OpenGL
 *  format, given the pixel type.
 *
 *  \sa allegro_gl_make_texture_ex()
 *
 *  \param bmp   The bitmap to get the OpenGL pixel format of.
 *  \param flags The conversion flags
 *  
 *  \return The pixel type of the bitmap.
 */
GLenum __allegro_gl_get_bitmap_color_format(BITMAP *bmp, int flags) {

	int c = bitmap_color_depth(bmp);

	switch (c) {

		case 8:
			if (flags & AGL_TEXTURE_ALPHA_ONLY) {
				return GL_ALPHA;
			}
			else {
				return __allegro_gl_texture_components[0];
			}

		case 15:
			if (flags & AGL_TEXTURE_FORCE_ALPHA_INTERNAL) {
				return GL_RGBA;
			}
			else {
				return __allegro_gl_texture_components[1];
			}

		case 16:
			return __allegro_gl_texture_components[2];

		case 24:
			return __allegro_gl_texture_components[3];

		case 32:
			if (flags & (AGL_TEXTURE_HAS_ALPHA
			           | AGL_TEXTURE_FORCE_ALPHA_INTERNAL)) {
				return GL_RGBA;
			}
			else {
				return __allegro_gl_texture_components[4];
			}
	
		default:
			TRACE(PREFIX_E "get_bitmap_color_format: unhandled bitmap "
			      "depth: %d\n", c);
			return -1;
	}
}



/* int allegro_gl_use_mipmapping(int enable) */
/** \ingroup texture
 *  Tell AllegroGL to use Mipmapping or not when generating textures via
 *  its functions. This will not affect outside OpenGL states.
 *  Default is FALSE (off).
 *
 *  \sa allegro_gl_check_texture(), allegro_gl_make_texture()
 *      allegro_gl_make_masked_texture()
 *  
 *  \param enable Set to TRUE to enable mipmapping, FALSE otherwise.
 *  \return       The previous mode (either TRUE or FALSE).
 *
 *  \deprecated
 */
int allegro_gl_use_mipmapping(int enable) {

	int old = allegro_gl_use_mipmapping_for_textures;
	allegro_gl_use_mipmapping_for_textures = enable;
	return old;
}
	


/* int allegro_gl_use_alpha_channel(int enable) */
/** \ingroup texture
 *  Tell AllegroGL to use Alpha channel or not when generating textures via
 *  its functions. This will not affect outside OpenGL states.
 *  Default is FALSE (off).
 *
 *  \sa allegro_gl_check_texture(), allegro_gl_make_texture(),
 *      allegro_gl_make_masked_texture()
 *  
 *  \param enable Set to TRUE to enable textures with alpha channel, FALSE
 *                otherwise.
 *  \return       The previous mode (either TRUE or FALSE).
 *
 *  \deprecated
 */
int allegro_gl_use_alpha_channel(int enable) {

	int old = __allegro_gl_use_alpha;
	__allegro_gl_use_alpha = enable;
	return old;
}
	


/* int allegro_gl_flip_texture(int enable) */
/** \ingroup texture
 *  Tell AllegroGL to flip the texture vertically or not when generating
 *  textures via its functions, to conform to the usual OpenGL texture
 *  coordinate system (increasing upwards).
 *  Default is TRUE (on).
 *
 *  \sa allegro_gl_check_texture(), allegro_gl_make_texture(),
 *      allegro_gl_make_masked_texture()
 *  
 *  \param enable Set to TRUE to enable textures with alpha channel, FALSE
 *                otherwise.
 *  \return       The previous mode (either TRUE or FALSE).
 *
 *  \deprecated
 */
int allegro_gl_flip_texture(int enable) {

	int old = __allegro_gl_flip_texture;
	__allegro_gl_flip_texture = enable;
	return old;
}
	


/* int allegro_gl_check_texture_ex(int flags, BITMAP *bmp, GLuint internal_format) */
/** \ingroup texture
 *  Checks whether the specified bitmap is of the proper size for
 *  texturing. This checks for the card's limit on texture sizes.
 * 
 *  The parameters to this function are identical to those of
 *  allegro_gl_make_texture_ex().
 * 
 *  \b Important \b note: allegro_gl_check_texture does not depend on the
 *  current state of the texture memory of the driver. This function may
 *  return TRUE although there is not enough memory to currently make the
 *  texture resident. You may check if the texture is actually resident with
 *  glAreTexturesResident().
 *
 *  \sa allegro_gl_make_texture_ex()
 *
 *  \param flags           The bitmap conversion flags.
 *  \param bmp             The bitmap to be converted to a texture.
 *  \param internal_format The internal format to convert to.
 *
 *  \return TRUE if the bitmap can be made a texture of, FALSE otherwise.
 */
int allegro_gl_check_texture_ex(int flags, BITMAP *bmp,
                                   GLint internal_format) {

	return (allegro_gl_make_texture_ex(flags | AGL_TEXTURE_CHECK_VALID_INTERNAL,
	                                   bmp, internal_format) ? TRUE : FALSE);
}



/* Convert flags from pre-0.2.0 to 0.2.0+ */
static int __allegro_gl_convert_flags(int flags) {

	flags |= AGL_TEXTURE_RESCALE;

	if (allegro_gl_use_mipmapping_for_textures) {
		flags |= AGL_TEXTURE_MIPMAP;
	}
	if (__allegro_gl_use_alpha) {
		flags |= AGL_TEXTURE_HAS_ALPHA;
	}
	if (__allegro_gl_flip_texture) {
		flags |= AGL_TEXTURE_FLIP;
	}

	if (allegro_gl_opengl_internal_texture_format == GL_ALPHA4
	 || allegro_gl_opengl_internal_texture_format == GL_ALPHA8
	 || allegro_gl_opengl_internal_texture_format == GL_ALPHA12
	 || allegro_gl_opengl_internal_texture_format == GL_ALPHA16
	 || allegro_gl_opengl_internal_texture_format == GL_ALPHA
	 || allegro_gl_opengl_internal_texture_format == GL_INTENSITY4
	 || allegro_gl_opengl_internal_texture_format == GL_INTENSITY8
	 || allegro_gl_opengl_internal_texture_format == GL_INTENSITY12
	 || allegro_gl_opengl_internal_texture_format == GL_INTENSITY16
	 || allegro_gl_opengl_internal_texture_format == GL_INTENSITY
	 || allegro_gl_opengl_internal_texture_format == 1) {
		flags |= AGL_TEXTURE_ALPHA_ONLY;
	}

	return flags;
}



/* int allegro_gl_check_texture(BITMAP *bmp) */
/** \ingroup texture
 *  Checks whether the specified bitmap is of the proper size for
 *  texturing. This checks for the card's limit on texture sizes.
 * 
 *  \b Important \b note: allegro_gl_check_texture does not depend on the
 *  current state of the texture memory of the driver. This function may
 *  return TRUE although there is not enough memory to currently make the
 *  texture resident. You may check if the texture is actually resident with
 *  glAreTexturesResident().
 *
 *  \sa allegro_gl_check_texture_ex(), allegro_gl_make_texture_ex()
 *
 *  \param bmp             The bitmap to be converted to a texture.
 *
 *  \return TRUE if the bitmap can be made a texture of, FALSE otherwise.
 *
 *  \deprecated
 */
int allegro_gl_check_texture(BITMAP *bmp) {

	int flags = __allegro_gl_convert_flags(0);
	
	return allegro_gl_check_texture_ex(flags, bmp,
	                                 allegro_gl_opengl_internal_texture_format);
}



/* Integer log2 function. Not optimized for speed. */
static int log2i(int n) {
	
	int k;
	
	if (n < 1) {
		return -1;
	}

	k = 0;
	while (n >>= 1) {
		k++;
	}

	return k;
}



/* BITMAP *__allegro_gl_munge_bitmap(int flags, BITMAP *bmp, GLint *type, GLint *format) */
/** \internal
 *  \ingroup texture
 *
 *  Builds a bitmap to match the given flags. Adapts the type and format as
 *  needed. Subbitmaps can be specified. Clipping must have already been
 *  applied before calling this function.
 *
 *  If NULL is returned, then the original bitmap can be used directly.
 */
BITMAP *__allegro_gl_munge_bitmap(int flags, BITMAP *bmp, int x, int y,
                                  int w, int h, GLint *type, GLint *format) {
	
	BITMAP *ret = 0, *temp = 0;

	int need_rescale = 0;
	int need_alpha   = 0;
	int need_flip    = 0;
	int depth = bitmap_color_depth(bmp);
	int force_copy   = 0;

	const int old_w = w, old_h = h;

	if (flags & AGL_TEXTURE_RESCALE) {
		
		/* Check if rescaling is needed */

		/* NP2 is not supported, and the texture isn't a power-of-two.
		 * Resize the next power of 2
		 */		
		if (!allegro_gl_extensions_GL.ARB_texture_non_power_of_two
		 && ((w & (w - 1)) || (h & (h - 1)))) {
			w = __allegro_gl_make_power_of_2(w);
			h = __allegro_gl_make_power_of_2(h);
			TRACE(PREFIX_I "munge_bitmap: Rescaling bitmap from "
			      "%ix%i to %ix%i due to non-power-of-2 source size.\n",
			      old_w, old_h, w, h);
			need_rescale = 1;
		}

		/* Don't go over the max texture size */
		if (w > allegro_gl_info.max_texture_size) {
			w = allegro_gl_info.max_texture_size;
			TRACE(PREFIX_I "munge_bitmap: Rescaling bitmap from "
			      "%ix%i to %ix%i due to max supported size exceed.\n",
			      old_w, old_h, w, h);
			need_rescale = 1;
		}
		
		if (h > allegro_gl_info.max_texture_size) {
			h = allegro_gl_info.max_texture_size;
			TRACE(PREFIX_I "munge_bitmap: Rescaling bitmap from "
			      "%ix%i to %ix%i due to max supported size exceed.\n",
			      old_w, old_h, w, h);
			need_rescale = 1;
		}

		/* Voodoos don't support mipmaps for textures greater than 32x32.
		 * If we're allowed to rescale, rescale the bitmap to 32x32.
		 * XXX <rohannessian> Apparently, this is a bug in one version
		 * of the Voodoo GL driver. Need to figure out a workaround
		 * for that.
		 */
		if (allegro_gl_info.is_voodoo && (flags & AGL_TEXTURE_MIPMAP)
		  && (w > 32 || h > 32)) {

			w = MIN(32, w);
			h = MIN(32, h);
			
			TRACE(PREFIX_I "munge_bitmap: Rescaling bitmap from "
			      "%ix%i to %ix%i due to Voodoo driver bug.\n",
			      old_w, old_h, w, h);
			need_rescale = 1;
		}
	}

	/* Matrox G200 cards have a bug where rectangular textures can't have
	 * more than 4 levels of mipmaps (max_mip == 3). This doesn't seem
	 * to affect square textures.
	 *
	 * Note: Using GLU to build the mipmaps seems to work. Maybe AGL is
	 * doing something wrong?
	 *
	 * Workaround: Use GLU to build the mipmaps, and force depth to 24 or
	 * 32.
	 */
	if ( allegro_gl_info.is_matrox_g200 && (flags & AGL_TEXTURE_MIPMAP)) {
		int wl = log2i(w);
		int hl = log2i(h);

		if (w != h && MAX(wl, hl) > 3 && depth < 24
		 && !(flags & AGL_TEXTURE_ALPHA_ONLY)) {
			TRACE(PREFIX_I "munge_bitmap: G200 path in use.\n");
			depth = 24;
		}
	}
	
	/* Do we need to flip the texture on the t axis? */
	if (flags & AGL_TEXTURE_FLIP) {
		need_flip = 1;
	}


	/* If not supported, blit to a 24 bpp bitmap and try again
	 */
	if (*type == -1) {
		TRACE(PREFIX_W "munge_bitmap: using temporary 24bpp bitmap\n");
		depth = 24;
	}

	/* We need a texture that can be used for masked blits.
	 * Insert an alpha channel if one is not there.
	 * If it's already there, replace it by 0/1 as needed.
	 */
	if ((flags & AGL_TEXTURE_MASKED) && !(flags & AGL_TEXTURE_ALPHA_ONLY)) {
		need_alpha = 1;

		switch (depth) {
		case 15:
			if (!allegro_gl_extensions_GL.EXT_packed_pixels) {
				depth = 32;
			}
			break;
		case 8:
		case 16:
		case 24:
		case 32:
			depth = 32;
			break;
		}
		force_copy = 1;
	}

	/* Allegro fills in 0 for the alpha channel. Matrox G200 seems to ignore
	 * the internal format; so we need to drop down to 24-bpp if no alpha
	 * will be needed.
	 */
	if (allegro_gl_info.is_matrox_g200 && !(flags & AGL_TEXTURE_MASKED)
	   && !(flags & AGL_TEXTURE_HAS_ALPHA) && depth == 32) {
		TRACE(PREFIX_I "munge_bitmap: G200 path in use.\n");
		depth = 24;
		force_copy = 1;
	}

	
	/* Do we need to do a color depth conversion or bitmap copy? */
	if (depth != bitmap_color_depth(bmp) || force_copy) {

		TRACE(PREFIX_I "munge_bitmap: Need to perform depth conversion from %i "
		      "to %i bpp.\n", bitmap_color_depth(bmp), depth);

		temp = create_bitmap_ex(depth, bmp->w, bmp->h);

		if (!temp) {
			TRACE(PREFIX_E "munge_bitmap: Unable to create temporary bitmap "
			      "%ix%ix%i\n", bmp->w, bmp->h, depth);
			return NULL;
		}

		/* XXX <rohannessian> Use palette conversion?
		 */
		if (bitmap_color_depth(bmp) == 8 && depth > 8) {
			int i, j;
			for (j = 0; j < bmp->h; j++) {
				for (i = 0; i < bmp->w; i++) {
					int c = _getpixel(bmp, i, j);
					putpixel(temp, i, j, makecol_depth(depth, c, c, c));
				}
			}
		}
		else {
			blit(bmp, temp, 0, 0, 0, 0, bmp->w, bmp->h);
		}
		bmp = temp;

		*format = __allegro_gl_get_bitmap_color_format(bmp, flags);
		*type = __allegro_gl_get_bitmap_type(bmp, flags);
	}



	/* Nothing to do? */
	if (!need_rescale && !need_alpha && !need_flip) {

		TRACE(PREFIX_I "munge_bitmap: No need for munging - returning %p\n",
		      temp);
		
		/* Return depth-converte bitmap, if present */
		if (temp) {
			return temp;
		}
		return NULL;
	}

	ret = create_bitmap_ex(depth, w, h);
	
	if (!ret) {
		TRACE(PREFIX_E "munge_bitmap: Unable to create result bitmap "
		      "%ix%ix%i\n", w, h, depth);
		goto error;
	}


	/* No need to fill in bitmap if we're just making a query */
	if (flags & AGL_TEXTURE_CHECK_VALID_INTERNAL) {
		if (temp) {
			destroy_bitmap(temp);
		}
		return ret;
	}


	/* Perform flip
	 * I don't want to have to deal with *yet another* temporary bitmap
	 * so instead, I fugde the line pointers around.
	 * This will work because we require Allegro memory bitmaps anyway.
	 */
	if (need_flip) {
		int i;
		TRACE(PREFIX_I "munge_bitmap: Flipping bitmap.\n");
		for (i = 0; i < bmp->h/2; i++) {
			unsigned char *l = bmp->line[i];
			bmp->line[i] = bmp->line[bmp->h - i - 1];
			bmp->line[bmp->h - i - 1] = l;
		}
	}
	
	/* Rescale bitmap */
	if (need_rescale) {
		TRACE(PREFIX_I "munge_bitmap: Rescaling bitmap.\n");
		stretch_blit(bmp, ret, x, y, old_w, old_h, 0, 0, ret->w, ret->h);
	}
	else {
		TRACE(PREFIX_I "munge_bitmap: Copying bitmap.\n");
		blit(bmp, ret, x, y, 0, 0, w, h);
	}

	/* Restore the original bitmap, if needed */
	if (need_flip && !temp) {
		int i;
		TRACE(PREFIX_I "munge_bitmap: Unflipping bitmap.\n");
		for (i = 0; i < bmp->h/2; i++) {
			unsigned char *l = bmp->line[i];
			bmp->line[i] = bmp->line[bmp->h - i - 1];
			bmp->line[bmp->h - i - 1] = l;
		}
	}
	
	/* Insert alpha channel */
	if (need_alpha) {
		int i, j;
		int mask = bitmap_mask_color(ret);

		/* alpha mask for 5.5.5.1 pixels */
		int alpha = (-1) ^ makecol_depth(depth, 255, 255, 255);

		TRACE(PREFIX_I "munge_bitmap: Inserting alpha channel.\n");

		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				int pix;
				
				switch (depth) {
				case 32:
					pix = _getpixel32(ret, i, j);

					if (pix == mask) {
						pix = 0;
					}
					else if ((flags & AGL_TEXTURE_HAS_ALPHA) == 0) {
						int r, g, b;
						r = getr32(pix);
						g = getg32(pix);
						b = getb32(pix);
						pix = makeacol32(r, g, b, 255);
					}
					_putpixel32(ret, i, j, pix);
					break;
				case 15: 
					pix = _getpixel16(ret, i, j);

					if (pix == mask) {
						pix = 0;
					}
					else {
						pix |= alpha;
					}
					
					_putpixel16(temp, i, j, pix);
					break;
				default:
					/* Shouldn't actually come here */
					ASSERT(0);
				}
			}
		}
	}


error:
	if (temp) {
		destroy_bitmap(temp);
	}

	return ret;
}



/* Perform the actual texture upload. Helper for agl_make_texture_ex().
 */
static GLuint do_texture_upload(BITMAP *bmp, GLuint tex, GLint internal_format,
                              GLint format, GLint type, int flags) {

	int bytes_per_pixel = BYTES_PER_PIXEL(bitmap_color_depth(bmp));
	GLint saved_row_length;
	GLint saved_alignment;
	GLenum target = GL_TEXTURE_2D;

	glBindTexture(target, tex);
	

	/* Handle proxy texture checks */
	if (flags & AGL_TEXTURE_CHECK_VALID_INTERNAL) {	
		/* <bcoconni> allegro_gl_check_texture is broken with GL drivers based
		 *  on Mesa. It seems this is a Mesa bug...
		 */
		if (allegro_gl_info.is_mesa_driver) {
			AGL_LOG(1, "* Note * check_texture: Mesa driver detected: "
		           "PROXY_TEXTURE_2D tests are skipped\n");
			return tex;
		}
		else {
			glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internal_format,
			             bmp->w, bmp->h, 0, format, type, NULL);

			glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0,
			                         GL_TEXTURE_COMPONENTS, &internal_format);

			return (internal_format ? tex : 0);
		}
	}
	

	/* Set up pixel transfer mode */
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &saved_row_length);
	glGetIntegerv(GL_UNPACK_ALIGNMENT,  &saved_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	TRACE(PREFIX_I "do_texture_upload: Making texture: bpp: %i\n",
	      bitmap_color_depth(bmp));

	/* Generate mipmaps, if needed */
	if (flags & AGL_TEXTURE_MIPMAP) {
		
		if (allegro_gl_extensions_GL.SGIS_generate_mipmap) {
			/* Easy way out - let the driver do it ;) 
			 * We do need to set high-qual mipmap generation though.
			 */
			glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
			glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
			TRACE(PREFIX_I "do_texture_upload: Using SGIS_generate_mipmap for "
			      "mipmap generation.\n");
		}
		else if (allegro_gl_info.is_matrox_g200
		     && (flags & AGL_TEXTURE_MIPMAP) && (bitmap_color_depth(bmp) >= 24
		        || bitmap_color_depth(bmp) == 8)
		     && (bmp->w != bmp->h)) {
				
			/* Matrox G200 has issues with our mipmapping code. Use GLU if we
			 * can.
			 */
			TRACE(PREFIX_I "do_texture_upload: Using GLU for mipmaps.\n");
			glPixelStorei(GL_UNPACK_ROW_LENGTH, bmp->h > 1
			            ? (bmp->line[1] - bmp->line[0]) / bytes_per_pixel
			            : bmp->w);
			glPixelStorei(GL_UNPACK_ROW_LENGTH,
	                           (bmp->line[1] - bmp->line[0]) / bytes_per_pixel);
			gluBuild2DMipmaps(GL_TEXTURE_2D, internal_format, bmp->w, bmp->h,
			                  format, type, bmp->line[0]);
		}
		else {
			int w = bmp->w;
			int h = bmp->h;
			int depth = bitmap_color_depth(bmp);
			
			/* The driver can't generate mipmaps for us. We can't rely on GLU
			 * since the Win32 version doesn't support any of the new pixel
			 * formats. Instead, we'll use our own downsampler (which only
			 * has to work on Allegro BITMAPs)
			 */
			BITMAP *temp = create_bitmap_ex(depth, w / 2, h / 2);

			/* We need to generate mipmaps up to 1x1 - compute the number
			 * of levels we need.
			 */
			int num_levels = log2i(MAX(bmp->w, bmp->h));
			
			int i, x, y;

			BITMAP *src, *dest;

			TRACE(PREFIX_I "do_texture_upload: Using Allegro for "
			      "mipmap generation.\n");

			if (!temp) {
				TRACE(PREFIX_E "do_texture_upload: Unable to create "
				      "temporary bitmap sized %ix%ix%i for mipmap generation!",
				      w / 2, h / 2, depth);
				tex = 0;
				goto end;
			}

			src = bmp;
			dest = temp;
			
			for (i = 1; i <= num_levels; i++) {

				for (y = 0; y < h; y += 2) {
					for (x = 0; x < w; x += 2) {

						int r, g, b, a;
						int pix[4];
						int avg;
						
						pix[0] = getpixel(src, x,     y);
						pix[1] = getpixel(src, x + 1, y);
						pix[2] = getpixel(src, x,     y + 1);
						pix[3] = getpixel(src, x + 1, y + 1);

						if (w == 1) {
							pix[1] = pix[0];
							pix[3] = pix[2];
						}
						if (h == 1) {
							pix[2] = pix[0];
							pix[3] = pix[1];
						}

						if (flags & AGL_TEXTURE_ALPHA_ONLY) {
							avg = (pix[0] + pix[1] + pix[2] + pix[3] + 2) / 4;
						}
						else {
							r = (getr_depth(depth, pix[0])
							   + getr_depth(depth, pix[1])
							   + getr_depth(depth, pix[2])
							   + getr_depth(depth, pix[3]) + 2) / 4;
							g = (getg_depth(depth, pix[0])
							   + getg_depth(depth, pix[1])
							   + getg_depth(depth, pix[2])
							   + getg_depth(depth, pix[3]) + 2) / 4;
							b = (getb_depth(depth, pix[0])
							   + getb_depth(depth, pix[1])
							   + getb_depth(depth, pix[2])
							   + getb_depth(depth, pix[3]) + 2) / 4;
							a = (geta_depth(depth, pix[0])
							   + geta_depth(depth, pix[1])
							   + geta_depth(depth, pix[2])
							   + geta_depth(depth, pix[3]) + 2) / 4;

							avg = makeacol_depth(depth, r, g, b, a);
						}

						putpixel(dest, x / 2, y / 2, avg);
					}
				}
				src = temp;

				/* Note - we round down; we're still compatible with
				 * ARB_texture_non_power_of_two.
				 */
				w = MAX(w / 2, 1);
				h = MAX(h / 2, 1);

				TRACE(PREFIX_I "do_texture_upload: Unpack row length: %li.\n",
		              (temp->h > 1)
				     ? (long int)((temp->line[1] - temp->line[0]) / bytes_per_pixel)
	                 : temp->w);

				glPixelStorei(GL_UNPACK_ROW_LENGTH, temp->h > 1
				             ? (temp->line[1] - temp->line[0]) / bytes_per_pixel
				             : temp->w);	

				glTexImage2D(GL_TEXTURE_2D, i, internal_format,
				             w, h, 0, format, type, temp->line[0]);

				TRACE(PREFIX_I "do_texture_upload: Mipmap level: %i, "
				      "size: %i x %i\n", i, w, h);

				TRACE(PREFIX_I "do_texture_upload: Uploaded texture: level %i, "
				      "internalformat: %s, %ix%i, format: 0x%x, type: 0x%x."
				      "\n", i, __allegro_gl_get_format_description(internal_format),
				      bmp->w, bmp->h, format, type);
			}

			destroy_bitmap(temp);
		}
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, (bmp->h > 1)
	             ? (bmp->line[1] - bmp->line[0]) / bytes_per_pixel
	             : bmp->w);
	
	TRACE(PREFIX_I "do_texture_upload: Unpack row length: %li.\n",
      (bmp->h > 1) ? (long int)((bmp->line[1] - bmp->line[0]) / bytes_per_pixel)
                   : bmp->w);

	/* Upload the base texture */
	glGetError();
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
	             bmp->w, bmp->h, 0, format, type, bmp->line[0]);

	TRACE(PREFIX_I "do_texture_upload: Uploaded texture: level 0, "
	      "internalformat: %s, %ix%i, format: 0x%x, type: 0x%x.\n",
	      __allegro_gl_get_format_description(internal_format),
		  bmp->w, bmp->h, format, type);
	
	TRACE(PREFIX_I "do_texture_upload: GL Error code: 0x%x\n", glGetError());

	if (!(flags & AGL_TEXTURE_MIPMAP)) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	
end:
	/* Restore state */
	glPixelStorei(GL_UNPACK_ROW_LENGTH, saved_row_length);
	glPixelStorei(GL_UNPACK_ALIGNMENT,  saved_alignment);

	return tex;
}	



/* GLuint allegro_gl_make_texture_ex(int flag, BITMAP *bmp, GLint internal_format) */
/** \ingroup texture
 *  Uploads an Allegro BITMAP to the GL driver as a texture.
 *
 *  The bitmap must be a memory bitmap (note that it can be a subbitmap).
 *
 *  Each bitmap will be converted to a single texture object, with all its
 *  size limitations imposed by the video driver and hardware.
 *
 *  The bitmap should conform to the size limitations imposed by the video
 *  driver. That is, if ARB_texture_non_power_of_two is not supported,
 *  then the BITMAP must be power-of-two sized. Otherwise, AllegroGL
 *  will pick the best format for the bitmap.
 *
 *  The original bitmap will NOT be modified.
 * 
 *  The flags parameter controls how the texture is generated. It can be a
 *  logical OR (|) of any of the following:
 *
 *    #AGL_TEXTURE_MIPMAP
 *    #AGL_TEXTURE_HAS_ALPHA
 *    #AGL_TEXTURE_FLIP
 *    #AGL_TEXTURE_MASKED
 *    #AGL_TEXTURE_RESCALE
 *
 *  AllegroGL will create a texture with the specified texel format.
 *  The texel format should be any of the valid formats that can be
 *  specified to glTexImage2D(). No validity checks will be performed
 *  by AllegroGL. If you want AllegroGL to automatically determine the
 *  format to use based on the BITMAP, use -1 as the format specifier.
 *
 *  A valid GL Rendering Context must have been established, which means you
 *  cannot use this function before having called set_gfx_mode() with a valid
 *  OpenGL mode.
 *
 *  \b Important \b note: on 32 bit bitmap in RGBA mode, the alpha channel
 *  created by Allegro is set to all 0 by default. This will cause the
 *  texture to not show up in 32bpp modes if alpha is set. You will need
 *  to fill in the alpha channel manually if you need an alpha channel.
 *
 *  \param bmp             The bitmap to be converted to a texture.
 *  \param flags           The conversion flags.
 *  \param internal_format The texture format to convert to.
 *
 *  \return GLuint The texture handle, or 0 on failure
 */
GLuint allegro_gl_make_texture_ex(int flags, BITMAP *bmp, GLint internal_format)
{
	GLuint tex = 0, ret = 0;
	BITMAP *temp = NULL;
	GLint type;
	GLint format;
	GLint old_tex;

	/* Print the parameters */
#ifdef DEBUGMODE
	char buf[1024] = "";
#	define PFLAG(name) if (flags & name) strcat(buf, #name "|");
	PFLAG(AGL_TEXTURE_MIPMAP);
	PFLAG(AGL_TEXTURE_HAS_ALPHA);
	PFLAG(AGL_TEXTURE_FLIP);
	PFLAG(AGL_TEXTURE_MASKED);
	PFLAG(AGL_TEXTURE_RESCALE);
	PFLAG(AGL_TEXTURE_ALPHA_ONLY);
#	undef PFLAG

	TRACE(PREFIX_I "make_texture_ex: flags: %s, bitmap %ix%i, %i bpp.\n", buf,
	      bmp ? bmp->w : 0, bmp ? bmp->h : 0,
	      bmp ? bitmap_color_depth(bmp) : 0);
	if (internal_format == -1) {
		TRACE(PREFIX_I "internalformat: AUTO\n");
	}
	else {
		TRACE(PREFIX_I "internalformat: %s\n",
			__allegro_gl_get_format_description(internal_format));
	}
#endif	

	/* Basic parameter checks */
	if (!__allegro_gl_valid_context)
		return 0;

	if (!bmp) {
		return 0;
	}

	glGetIntegerv(GL_TEXTURE_2D_BINDING, &old_tex);

	/* Voodoo cards don't seem to support mipmaps for textures over 32x32...
	 */
	if ((bmp->w > 32 || bmp->h > 32) && (allegro_gl_info.is_voodoo)) {
		/* Disable mipmapping if the user didn't allow us to rescale */
		if (!(flags & AGL_TEXTURE_RESCALE)) {
			TRACE(PREFIX_I "make_texture_ex: Voodoo card detected && texture "
			      "size > 32 texels && no rescaling. Disabling mipmaps.\n");
			flags &= ~AGL_TEXTURE_MIPMAP;
		}
	}

	/* Check the maximum texture size */
	if (bmp->w > allegro_gl_info.max_texture_size
	 || bmp->h > allegro_gl_info.max_texture_size) {
		if ((flags & AGL_TEXTURE_RESCALE) == 0) {
			TRACE(PREFIX_I "make_texture_ex: Max texture size exceeded but no "
			      "rescaling allowed. Returning 0 (unsupported).\n");
			return 0;
		}
	}

	/* Check power-of-2 */
	if (((bmp->w & (bmp->w - 1)) || (bmp->h & (bmp->h - 1)))
	  && !(flags & AGL_TEXTURE_RESCALE)
	  && !allegro_gl_extensions_GL.ARB_texture_non_power_of_two) {
		TRACE(PREFIX_I "make_texture_ex: Non-power-of-2 sized bitmap provided, "
		      "no rescaling allowed, and ARB_texture_non_power_of_two "
		      "unsupported. Returning 0 (unsupported).\n");
		return 0;
	}
	
	
	/* Get OpenGL format and type for this pixel data */
	format = __allegro_gl_get_bitmap_color_format(bmp, flags);
	type = __allegro_gl_get_bitmap_type(bmp, flags);
	
	if (flags & AGL_TEXTURE_ALPHA_ONLY) {
		type   = GL_UNSIGNED_BYTE;
		if (internal_format == GL_ALPHA || internal_format == GL_ALPHA4
		 || internal_format == GL_ALPHA8) {
			format = GL_ALPHA;
		}
		else if (internal_format == GL_INTENSITY
		      || internal_format == GL_INTENSITY4
		      || internal_format == GL_INTENSITY8) {
			format = GL_RED;
		}
		else if (internal_format == GL_LUMINANCE
		      || internal_format == GL_LUMINANCE4
		      || internal_format == GL_LUMINANCE8) {
			format = GL_LUMINANCE;
		}

		/* Alpha bitmaps must be 8-bpp */
		if (bitmap_color_depth(bmp) != 8) {
			return 0;
		}
	}

	if (flags & AGL_TEXTURE_MASKED) {
		flags |= AGL_TEXTURE_FORCE_ALPHA_INTERNAL;
	}

	TRACE(PREFIX_I "make_texture_ex: Preselected texture format: %s, "
	      "type: 0x%x\n", __allegro_gl_get_format_description(format), type);
	
	/* Munge the bitmap if needed (rescaling, alpha channel, etc) */
	temp = __allegro_gl_munge_bitmap(flags, bmp, 0, 0, bmp->w, bmp->h,
	                                 &type, &format);
	if (temp) {
		bmp = temp;
	}
	
	if (internal_format == -1) {
		internal_format = __allegro_gl_get_texture_format_ex(bmp, flags);
		TRACE(PREFIX_I "make_texture_ex: Picked internalformat: %s\n",
		      __allegro_gl_get_format_description(internal_format));
	}

	if (internal_format == -1) {
		TRACE(PREFIX_E "make_texture_ex: Invalid internal format!: "
		      "%s\n", __allegro_gl_get_format_description(internal_format));
		goto end;
	}
	
	TRACE(PREFIX_I "make_texture_ex: dest format=%s, source format=%s, "
	      "type=0x%x\n", __allegro_gl_get_format_description(internal_format),
              __allegro_gl_get_format_description(format), (int)type);

	
	/* ATI Radeon 7000 inverts R and B components when generating mipmaps and
	 * the internal format is GL_RGB8, but only on mipmaps. Instead, we'll use
	 * GL_RGBA8. This works for bitmaps of depth <= 24. For 32-bpp bitmaps,
	 * some additional tricks are needed: We must fill in alpha with 255.
	 */
	if (allegro_gl_info.is_ati_radeon_7000 && (flags & AGL_TEXTURE_MIPMAP)
	 && internal_format == GL_RGB8
	 && allegro_gl_extensions_GL.SGIS_generate_mipmap) {

		int i, j;
		int depth = bitmap_color_depth(bmp);

		TRACE(PREFIX_I "make_texture_ex: ATI Radeon 7000 detected, mipmapping "
		      "used, SGIS_generate_mipmap available and selected "
		      "internalformat is GL_RGB8 but format is GL_RGBA. Working around "
		      "ATI driver bug by upgrading bitmap to 32-bpp and using GL_RGBA8 "
		      "instead.\n");

		if (depth == 32) {

			/* Create temp bitmap if not already there */
			if (!temp) {
				temp = create_bitmap_ex(depth, bmp->w, bmp->h);
				if (!temp) {
					TRACE(PREFIX_E "make_texture_ex: Unable to allocate "
					      "memory for temporary bitmap (Radeon 7000 path)!\n");
					goto end;
				}
				blit(bmp, temp, 0, 0, 0, 0, bmp->w, bmp->h);
				bmp = temp;
			}
			
			/* Slow path, until ATI finally gets around to fixing their
			 * drivers.
			 *
			 * Note: If destination internal format was GL_RGBx, then no masking
			 * code is needed.
			 */
			for (j = 0; j < bmp->h; j++) {
				for (i = 0; i < bmp->w; i++) {
					int pix = _getpixel32(bmp, i, j);
					_putpixel32(bmp, i, j,
					    makeacol32(getr32(pix), getg32(pix), getb32(pix), 255));
				}
			}
		}
		internal_format = GL_RGBA8;
	}
	

	/* Generate the texture */
	glGenTextures(1, &tex);
	if (!tex) {
		TRACE(PREFIX_E "make_texture_ex: Unable to create GL texture!\n");
		goto end;
	}

	ret = do_texture_upload(bmp, tex, internal_format, format, type, flags);

end:
	if (temp) {
		destroy_bitmap(temp);
	}

	if (!ret && tex) {
		glDeleteTextures(1, &tex);
	}

	glBindTexture(GL_TEXTURE_2D, old_tex);

	return tex;
}





/* GLuint allegro_gl_make_texture(BITMAP *bmp) */
/** \ingroup texture
 *
 *  Uploads an Allegro BITMAP to the GL driver as a texture.
 *
 *  \deprecated
 *
 *  \sa allegro_gl_make_texture_ex()
 */
GLuint allegro_gl_make_texture(BITMAP *bmp) {
		
	int flags = __allegro_gl_convert_flags(0);
	
	return allegro_gl_make_texture_ex(flags, bmp,
	                                 allegro_gl_opengl_internal_texture_format);
}



/* GLuint allegro_gl_make_masked_texture(BITMAP *bmp) */
/** \ingroup texture
 *
 *  Uploads an Allegro BITMAP to the GL driver as a texture.
 *
 *  \deprecated
 *
 *  \sa allegro_gl_make_texture_ex()
 */
GLuint allegro_gl_make_masked_texture(BITMAP *bmp) {
		
	int flags = __allegro_gl_convert_flags(AGL_TEXTURE_MASKED);

	return allegro_gl_make_texture_ex(flags, bmp,
	                                 allegro_gl_opengl_internal_texture_format);
}



/* GLenum allegro_gl_get_bitmap_type(BITMAP *bmp) */
/** \internal
 *  \ingroup texture
 *
 *  Returns the pixel packing format of the selected bitmap in OpenGL
 *  format. OpenGL 1.1 only understands color component sizes (bytes
 *  per color), whereas OpenGL 1.2+ can understand Allegro's packed
 *  formats (such as 16 bit color, with 5-6-5 bit components).
 *
 *  If the bitmap is not representable to the OpenGL implementation 
 *  (e.g. it cannot handle packed formats because it's earlier than 
 *  1.2 and does not have the extension) this function returns -1.
 *
 *  \sa allegro_gl_make_texture_ex()
 *
 *  \param bmp    The bitmap to get the OpenGL pixel type of.
 *
 *  \return The pixel type of the bitmap.
 *
 *  \deprecated This function is deprecated and should no longer be used.
 */
GLenum allegro_gl_get_bitmap_type(BITMAP *bmp) {

	int flags = __allegro_gl_convert_flags(0);
	return __allegro_gl_get_bitmap_type(bmp, flags);
}



/* GLenum allegro_gl_get_bitmap_color_format(BITMAP *bmp) */
/** \internal
 *  \ingroup texture
 *
 *  Returns the pixel color format of the selected Allegro bitmap in OpenGL
 *  format, given the pixel type.
 *
 *  \sa allegro_gl_make_texture_ex()
 *
 *  \param bmp   The bitmap to get the OpenGL pixel format of.
 *  
 *  \return The pixel type of the bitmap.
 *
 *  \deprecated This function is deprecated and should no longer be used.
 */
GLenum allegro_gl_get_bitmap_color_format(BITMAP *bmp) {

	int flags = __allegro_gl_convert_flags(0);
	return __allegro_gl_get_bitmap_color_format(bmp, flags);
}

