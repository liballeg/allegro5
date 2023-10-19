/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      OpenGL bitmap vtable
 *
 *      By Elias Pschernig.
 *      OpenGL ES 1.1 support by Trent Gamblin.
 *
 */

#include <math.h>
#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memblit.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_transform.h"

#if defined ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif

#include "ogl_helpers.h"

ALLEGRO_DEBUG_CHANNEL("opengl")

/* OpenGL does not support "locking", i.e. direct access to a memory
 * buffer with pixel data. Instead, the data can be copied from/to
 * client memory. Because OpenGL stores pixel data starting with the
 * pixel in the lower left corner, that's also how we return locked
 * data. Otherwise (and as was done in earlier versions of this code)
 * we would have to flip all locked memory after receiving and before
 * sending it to OpenGL.
 * 
 * Also, we do support old OpenGL drivers where textures must have
 * power-of-two dimensions. If a non-power-of-two bitmap is created in
 * such a case, we use a texture with the next larger POT dimensions,
 * and just keep some unused padding space to the right/bottom of the
 * pixel data.
 * 
 * Putting it all together, if we have an Allegro bitmap like this,
 * with Allegro's y-coordinates:
 * 0 ###########
 * 1 #111      #
 * 2 #222      #
 * 3 #333      #
 * 4 ###########
 *
 * Then the corresponding texture looks like this with OpenGL
 * y-coordinates (assuming we use an old driver which needs padding to
 * POT):
 * 7 ................
 * 6 ................
 * 5 ................
 * 4 ###########.....
 * 3 #333      #.....
 * 2 #222      #.....
 * 1 #111      #.....
 * 0 ###########.....
 */

/* Conversion table from Allegro's pixel formats to corresponding OpenGL
 * formats. The three entries are:
 * - GL internal format: the number of color components in the texture
 * - GL pixel type: Specifies the data type of the pixel data.
 * - GL format: Specifies the format of the pixel data.
 *
 * GL does not support RGB_555 and BGR_555 directly so we use
 * GL_UNSIGNED_SHORT_1_5_5_5_REV when transferring pixel data, and ensure that
 * the alpha bit (the "1" component) is present by setting GL_ALPHA_BIAS.
 * 
 * Desktop OpenGL 3.0+ has no GL_LUMINANCE, so we have to adjust depending on
 * the runtime version.
 */
#define get_glformat(f, c) _al_ogl_get_glformat((f), (c))
int _al_ogl_get_glformat(int format, int component)
{
   #if !defined ALLEGRO_CFG_OPENGLES
   static int glformats[ALLEGRO_NUM_PIXEL_FORMATS][3] = {
      /* Skip pseudo formats */
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      /* Actual formats */
      {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* ARGB_8888 */
      {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBA_8888 */
      {GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_BGRA}, /* ARGB_4444 */
      {GL_RGB8, GL_UNSIGNED_BYTE, GL_BGR}, /* RGB_888 */
      {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB}, /* RGB_565 */
      {GL_RGB5, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_BGRA}, /* RGB_555 - see above */
      {GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA}, /* RGBA_5551 */
      {GL_RGB5_A1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_BGRA}, /* ARGB_1555 */
      {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* ABGR_8888 */
      {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* XBGR_8888 */
      {GL_RGB8, GL_UNSIGNED_BYTE, GL_RGB}, /* BGR_888 */
      {GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, GL_RGB}, /* BGR_565 */
      {GL_RGB5, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_RGBA}, /* BGR_555 - see above */
      {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBX_8888 */
      {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* XRGB_8888 */
      {GL_RGBA32F_ARB, GL_FLOAT, GL_RGBA}, /* ABGR_F32 */
      {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA}, /* ABGR_8888_LE */
      {GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA}, /* RGBA_4444 */
      {GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE}, /* SINGLE_CHANNEL_8 */
      {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBA_DXT1 */
      {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBA_DXT3 */
      {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBA_DXT5 */
   };
  
   if (al_get_opengl_version() >= _ALLEGRO_OPENGL_VERSION_3_0) {
      glformats[ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8][0] = GL_RED;
      glformats[ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8][2] = GL_RED;
   }
   #else
   // TODO: Check supported formats by various GLES versions
   static const int glformats[ALLEGRO_NUM_PIXEL_FORMATS][3] = {
      /* Skip pseudo formats */
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      /* Actual formats */
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB}, /* RGB_565 */
      {0, 0, 0},
      {GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA}, /* RGBA_5551 */
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
      {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA}, /* ABGR_8888_LE */
      {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA}, /* RGBA_4444 */
      {GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE}, /* SINGLE_CHANNEL_8 */
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
   };
   #endif
   
   return glformats[format][component];
}

static ALLEGRO_BITMAP_INTERFACE glbmp_vt;


#define SWAP(type, x, y) {type temp = x; x = y; y = temp;}

#define ERR(e) case e: return #e;
char const *_al_gl_error_string(GLenum e)
{
   switch (e) {
      ERR(GL_NO_ERROR)
      ERR(GL_INVALID_ENUM)
      ERR(GL_INVALID_VALUE)
      ERR(GL_INVALID_OPERATION)
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      ERR(GL_STACK_OVERFLOW)
      ERR(GL_STACK_UNDERFLOW)
#endif
      ERR(GL_OUT_OF_MEMORY)
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      ERR(GL_INVALID_FRAMEBUFFER_OPERATION)
#endif
   }
   return "UNKNOWN";
}
#undef ERR

static INLINE void transform_vertex(float* x, float* y, float* z)
{
   al_transform_coordinates_3d(al_get_current_transform(), x, y, z);
}

static void draw_quad(ALLEGRO_BITMAP *bitmap,
    ALLEGRO_COLOR tint,
    float sx, float sy, float sw, float sh,
    int flags)
{
   float tex_l, tex_t, tex_r, tex_b, w, h, true_w, true_h;
   float dw = sw, dh = sh;
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   ALLEGRO_OGL_BITMAP_VERTEX *verts;
   ALLEGRO_DISPLAY *disp = al_get_current_display();
   
   (void)flags;

   if (disp->num_cache_vertices != 0 && ogl_bitmap->texture != disp->cache_texture) {
      disp->vt->flush_vertex_cache(disp);
   }
   disp->cache_texture = ogl_bitmap->texture;

   verts = disp->vt->prepare_vertex_cache(disp, 6);

   tex_l = ogl_bitmap->left;
   tex_r = ogl_bitmap->right;
   tex_t = ogl_bitmap->top;
   tex_b = ogl_bitmap->bottom;

   w = bitmap->w;
   h = bitmap->h;
   true_w = ogl_bitmap->true_w;
   true_h = ogl_bitmap->true_h;

   tex_l += sx / true_w;
   tex_t -= sy / true_h;
   tex_r -= (w - sx - sw) / true_w;
   tex_b += (h - sy - sh) / true_h;

   verts[0].x = 0;
   verts[0].y = dh;
   verts[0].z = 0;
   verts[0].tx = tex_l;
   verts[0].ty = tex_b;
   verts[0].r = tint.r;
   verts[0].g = tint.g;
   verts[0].b = tint.b;
   verts[0].a = tint.a;
   
   verts[1].x = 0;
   verts[1].y = 0;
   verts[1].z = 0;
   verts[1].tx = tex_l;
   verts[1].ty = tex_t;
   verts[1].r = tint.r;
   verts[1].g = tint.g;
   verts[1].b = tint.b;
   verts[1].a = tint.a;
   
   verts[2].x = dw;
   verts[2].y = dh;
   verts[2].z = 0;
   verts[2].tx = tex_r;
   verts[2].ty = tex_b;
   verts[2].r = tint.r;
   verts[2].g = tint.g;
   verts[2].b = tint.b;
   verts[2].a = tint.a;
   
   verts[4].x = dw;
   verts[4].y = 0;
   verts[4].z = 0;
   verts[4].tx = tex_r;
   verts[4].ty = tex_t;
   verts[4].r = tint.r;
   verts[4].g = tint.g;
   verts[4].b = tint.b;
   verts[4].a = tint.a;
   
   if (disp->cache_enabled) {
      /* If drawing is batched, we apply transformations manually. */
      transform_vertex(&verts[0].x, &verts[0].y, &verts[0].z);
      transform_vertex(&verts[1].x, &verts[1].y, &verts[1].z);
      transform_vertex(&verts[2].x, &verts[2].y, &verts[2].z);
      transform_vertex(&verts[4].x, &verts[4].y, &verts[4].z);
   }
   verts[3] = verts[1];
   verts[5] = verts[2];
   
   if (!disp->cache_enabled)
      disp->vt->flush_vertex_cache(disp);
}
#undef SWAP


static void ogl_draw_bitmap_region(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint, float sx, float sy,
   float sw, float sh, int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_target;
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);

   /* For sub-bitmaps */
   if (target->parent) {
      target = target->parent;
   }

   ogl_target = target->extra;

   if (!(al_get_bitmap_flags(bitmap) & ALLEGRO_MEMORY_BITMAP) && !bitmap->locked &&
         !target->locked) {
      ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_source = bitmap->extra;
      if (ogl_source->is_backbuffer) {
         /* Our source bitmap is the OpenGL backbuffer, the target
          * is an OpenGL texture.
          */
         float xtrans, ytrans;

         /* Source and target cannot both be the back-buffer. */
         ASSERT(!ogl_target->is_backbuffer);

         /* If we only translate, we can do this fast. */
         if (_al_transform_is_translation(al_get_current_transform(),
            &xtrans, &ytrans)) {
            /* In general, we can't modify the texture while it's
             * FBO bound - so we temporarily disable the FBO.
             */
            if (ogl_target->fbo_info)
               _al_ogl_set_target_bitmap(disp, bitmap);
            
            /* We need to do clipping because glCopyTexSubImage2D
             * fails otherwise.
             */
            if (xtrans < target->cl) {
               sx -= xtrans - target->cl;
               sw += xtrans - target->cl;
               xtrans = target->cl;
            }
            if (ytrans < target->ct) {
               sy -= ytrans - target->ct;
               sh += ytrans - target->ct;
               ytrans = target->ct;
            }
            if (xtrans + sw > target->cr_excl) {
               sw = target->cr_excl - xtrans;
            }
            if (ytrans + sh > target->cb_excl) {
               sh = target->cb_excl - ytrans;
            }

            /* Note: Allegro 5.0.0 does not support blending or
             * tinting if the source bitmap is the screen. So it is
             * correct to ignore them here.
             */

            glBindTexture(GL_TEXTURE_2D, ogl_target->texture);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                xtrans, target->h - ytrans - sh,
                sx, bitmap->h - sy - sh,
                sw, sh);
            /* Fix up FBO again after the copy. */
            if (ogl_target->fbo_info)
               _al_ogl_set_target_bitmap(disp, target);
            return;
         }
         
         /* Drawing a deformed backbuffer is not supported. */
         ASSERT(0);
      }
   }
   if (disp->ogl_extras->opengl_target == target) {
      draw_quad(bitmap, tint, sx, sy, sw, sh, flags);
      return;
   }

   /* If all else fails, fall back to software implementation. */
   _al_draw_bitmap_region_memory(bitmap, tint, sx, sy, sw, sh, 0, 0, flags);
}


/* Helper to get smallest fitting power of two. */
static int pot(int x)
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
}



static GLint ogl_bitmap_wrap(ALLEGRO_BITMAP_WRAP wrap)
{
   switch (wrap) {
      default:
      case ALLEGRO_BITMAP_WRAP_DEFAULT:
         return GL_CLAMP_TO_EDGE;
      case ALLEGRO_BITMAP_WRAP_REPEAT:
         return GL_REPEAT;
      case ALLEGRO_BITMAP_WRAP_CLAMP:
         return GL_CLAMP_TO_EDGE;
      case ALLEGRO_BITMAP_WRAP_MIRROR:
         return GL_MIRRORED_REPEAT;
   }
}



// FIXME: need to do all the logic AllegroGL does, checking extensions,
// proxy textures, formats, limits ...
static bool ogl_upload_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   int w = bitmap->w;
   int h = bitmap->h;
   int bitmap_format = al_get_bitmap_format(bitmap);
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   bool post_generate_mipmap = false;
   GLenum e;
   int filter;
   int gl_filters[] = {
      GL_NEAREST, GL_LINEAR,
      GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
   };

   if (ogl_bitmap->texture == 0) {
      glGenTextures(1, &ogl_bitmap->texture);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glGenTextures failed: %s\n", _al_gl_error_string(e));
      }
      else {
         ALLEGRO_DEBUG("Created new OpenGL texture %d (%dx%d, format %s)\n",
                    ogl_bitmap->texture,
                    ogl_bitmap->true_w, ogl_bitmap->true_h,
                    _al_pixel_format_name(bitmap_format));
      }
   }
   glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glBindTexture for texture %d failed (%s).\n",
         ogl_bitmap->texture, _al_gl_error_string(e));
   }

   /* Wrap, Min/Mag should always come before glTexImage2D so the texture is "complete" */
   // NOTE: on OGLES CLAMP_TO_EDGE is only one supported with NPOT textures
   ALLEGRO_BITMAP_WRAP wrap_u, wrap_v;
   _al_get_bitmap_wrap(bitmap, &wrap_u, &wrap_v);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ogl_bitmap_wrap(wrap_u));
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ogl_bitmap_wrap(wrap_v));

   filter = (bitmap_flags & ALLEGRO_MIPMAP) ? 2 : 0;
   if (bitmap_flags & ALLEGRO_MIN_LINEAR) {
      filter++;
   }
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filters[filter]);

   filter = 0;
   if (bitmap_flags & ALLEGRO_MAG_LINEAR) {
      filter++;
   }
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filters[filter]);

// TODO: To support anisotropy, we would need an API for it. Something
// like:
// al_set_new_bitmap_option(ALLEGRO_ANISOTROPY, 16.0);
#if 0
   if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_texture_filter_anisotropic) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
   }
#endif

   if (bitmap_flags & ALLEGRO_MIPMAP) {
      /* If using FBOs, use glGenerateMipmapEXT instead of the GL_GENERATE_MIPMAP
       * texture parameter.  GL_GENERATE_MIPMAP is deprecated in GL 3.0 so we
       * may want to use the new method in other cases as well.
       */
      if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object ||
          al_get_opengl_extension_list()->ALLEGRO_GL_OES_framebuffer_object ||
          IS_OPENGLES /* FIXME */) {
         post_generate_mipmap = true;
      }
      else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
         glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
          e = glGetError();
          if (e) {
              ALLEGRO_ERROR("glTexParameteri for texture %d failed (%s).\n",
                            ogl_bitmap->texture, _al_gl_error_string(e));
          }
#endif
      }
   }

   /* If there's unused space around the bitmap, we need to clear it
    * else linear filtering will cause artifacts from the random
    * data there. We also clear for floating point formats because
    * NaN values in the texture cause some blending modes to fail on
    * those pixels
    */
   if (!IS_OPENGLES) {
      if (ogl_bitmap->true_w != bitmap->w ||
            ogl_bitmap->true_h != bitmap->h ||
            bitmap_format == ALLEGRO_PIXEL_FORMAT_ABGR_F32) {
         unsigned char *buf;
         buf = al_calloc(ogl_bitmap->true_h, ogl_bitmap->true_w);
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         glTexImage2D(GL_TEXTURE_2D, 0, get_glformat(bitmap_format, 0),
            ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
            GL_ALPHA, GL_UNSIGNED_BYTE, buf);
         e = glGetError();
         al_free(buf);
      }
      else {
         glTexImage2D(GL_TEXTURE_2D, 0, get_glformat(bitmap_format, 0),
            ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
            get_glformat(bitmap_format, 2), get_glformat(bitmap_format, 1),
            NULL);
         e = glGetError();
      }
   }
   else {
      unsigned char *buf;
      int pix_size = al_get_pixel_size(bitmap_format);
      buf = al_calloc(pix_size,
         ogl_bitmap->true_h * ogl_bitmap->true_w);
      glPixelStorei(GL_UNPACK_ALIGNMENT, pix_size);
      glTexImage2D(GL_TEXTURE_2D, 0, get_glformat(bitmap_format, 0),
         ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
         get_glformat(bitmap_format, 2),
         get_glformat(bitmap_format, 1), buf);
      e = glGetError();
      al_free(buf);
   }

   if (e) {
      ALLEGRO_ERROR("glTexImage2D for format %s, size %dx%d failed (%s)\n",
         _al_pixel_format_name(bitmap_format),
         ogl_bitmap->true_w, ogl_bitmap->true_h,
         _al_gl_error_string(e));
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
      // FIXME: Should we convert it into a memory bitmap? Or if the size is
      // the problem try to use multiple textures?
      return false;
   }

   if (post_generate_mipmap) {
      glGenerateMipmapEXT(GL_TEXTURE_2D);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glGenerateMipmapEXT for texture %d failed (%s).\n",
            ogl_bitmap->texture, _al_gl_error_string(e));
      }
   }
   
   ogl_bitmap->left = 0;
   ogl_bitmap->right = (float) w / ogl_bitmap->true_w;
   ogl_bitmap->top = (float) h / ogl_bitmap->true_h;
   ogl_bitmap->bottom = 0;

   return true;
}



static void ogl_update_clipping_rectangle(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY *ogl_disp = al_get_current_display();
   ALLEGRO_BITMAP *target_bitmap = bitmap;

   if (bitmap->parent) {
      target_bitmap = bitmap->parent;
   }

   if (ogl_disp->ogl_extras->opengl_target == target_bitmap) {
      _al_ogl_setup_bitmap_clipping(bitmap);
   }
}



static void ogl_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   ALLEGRO_DISPLAY *disp;
   ALLEGRO_DISPLAY *bmp_disp;
   ALLEGRO_DISPLAY *old_disp = NULL;

   ASSERT(!al_is_sub_bitmap(bitmap));

   bmp_disp = _al_get_bitmap_display(bitmap);
   disp = al_get_current_display();
   if (bmp_disp->ogl_extras->is_shared == false &&
       bmp_disp != disp) {
      old_disp = disp;
      _al_set_current_display_only(bmp_disp);
   }

   if (bmp_disp->ogl_extras->opengl_target == bitmap) {
     bmp_disp->ogl_extras->opengl_target = NULL;
   }

   al_remove_opengl_fbo(bitmap);

   if (ogl_bitmap->texture) {
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
   }

   if (old_disp) {
      _al_set_current_display_only(old_disp);
   }

   al_free(ogl_bitmap);
}



static void ogl_bitmap_pointer_changed(ALLEGRO_BITMAP *bitmap,
      ALLEGRO_BITMAP *old)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra = bitmap->extra;
   if (extra && extra->fbo_info) {
      ASSERT(extra->fbo_info->owner == old);
      extra->fbo_info->owner = bitmap;
   }
}


static bool can_flip_blocks(ALLEGRO_PIXEL_FORMAT format)
{
   switch (format) {
      case ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1:
      case ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3:
      case ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5:
         return true;
      default:
         return false;
   }
}


static void ogl_flip_blocks(ALLEGRO_LOCKED_REGION *lr, int wc, int hc)
{
#define SWAP(x, y) do { unsigned char t = x; x = y; y = t; } while (0)
   int x, y;
   unsigned char* data = lr->data;
   ASSERT(can_flip_blocks(lr->format));
   switch (lr->format) {
      case ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1: {
         for (y = 0; y < hc; y++) {
            unsigned char* row = data;
            for (x = 0; x < wc; x++) {
               /* Skip color table */
               row += 4;

               /* Swap colors */
               SWAP(row[0], row[3]);
               SWAP(row[2], row[1]);

               /* Skip bit-map */
               row += 4;
            }
            data += lr->pitch;
         }
         break;
      }
      case ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3: {
         for (y = 0; y < hc; y++) {
            unsigned char* row = data;
            for (x = 0; x < wc; x++) {
               /* Swap alpha */
               SWAP(row[0], row[6]);
               SWAP(row[1], row[7]);
               SWAP(row[2], row[4]);
               SWAP(row[3], row[5]);

               /* Skip alpha bit-map */
               row += 8;

               /* Skip color table */
               row += 4;

               /* Swap colors */
               SWAP(row[0], row[3]);
               SWAP(row[2], row[1]);

               /* Skip bit-map */
               row += 4;
            }
            data += lr->pitch;
         }
         break;
      }
      case ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5: {
         for (y = 0; y < hc; y++) {
            unsigned char* row = data;
            for (x = 0; x < wc; x++) {
               uint16_t bit_row0, bit_row1, bit_row2, bit_row3;

               /* Skip the alpha table */
               row += 2;

               bit_row0 = (((uint16_t)row[0]) | (uint16_t)row[1] << 8) << 4;
               bit_row1 = (((uint16_t)row[1]) | (uint16_t)row[2] << 8) >> 4;
               bit_row2 = (((uint16_t)row[3]) | (uint16_t)row[4] << 8) << 4;
               bit_row3 = (((uint16_t)row[4]) | (uint16_t)row[5] << 8) >> 4;

               row[0] = (unsigned char)(bit_row3 & 0x00ff);
               row[1] = (unsigned char)((bit_row2 & 0x00ff) | ((bit_row3 & 0xff00) >> 8));
               row[2] = (unsigned char)((bit_row2 & 0xff00) >> 8);

               row[3] = (unsigned char)(bit_row1 & 0x00ff);
               row[4] = (unsigned char)((bit_row0 & 0x00ff) | ((bit_row1 & 0xff00) >> 8));
               row[5] = (unsigned char)((bit_row0 & 0xff00) >> 8);

               /* Skip the alpha bit-map */
               row += 6;

               /* Skip color table */
               row += 4;

               /* Swap colors */
               SWAP(row[0], row[3]);
               SWAP(row[2], row[1]);

               /* Skip bit-map */
               row += 4;
            }
            data += lr->pitch;
         }
         break;
      }
      default:
         (void)x;
         (void)y;
         (void)data;
         (void)wc;
         (void)hc;
   }
#undef SWAP
}

static ALLEGRO_LOCKED_REGION *ogl_lock_compressed_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int w, int h, int flags)
{
#if !defined ALLEGRO_CFG_OPENGLES
   ALLEGRO_BITMAP_EXTRA_OPENGL *const ogl_bitmap = bitmap->extra;
   ALLEGRO_DISPLAY *disp;
   ALLEGRO_DISPLAY *old_disp = NULL;
   GLenum e;
   bool ok = true;
   int bitmap_format = al_get_bitmap_format(bitmap);
   int block_width = al_get_pixel_block_width(bitmap_format);
   int block_height = al_get_pixel_block_height(bitmap_format);
   int block_size = al_get_pixel_block_size(bitmap_format);
   int xc = x / block_width;
   int yc = y / block_width;
   int wc = w / block_width;
   int hc = h / block_width;
   int true_wc = ogl_bitmap->true_w / block_width;
   int true_hc = ogl_bitmap->true_h / block_height;
   int gl_yc = _al_get_least_multiple(bitmap->h, block_height) / block_height - yc - hc;

   if (!can_flip_blocks(bitmap_format)) {
      return NULL;
   }

   if (flags & ALLEGRO_LOCK_WRITEONLY) {
      int pitch = wc * block_size;
      ogl_bitmap->lock_buffer = al_malloc(pitch * hc);
      if (ogl_bitmap->lock_buffer == NULL) {
         return NULL;
      }

      bitmap->locked_region.data = ogl_bitmap->lock_buffer + pitch * (hc - 1);
      bitmap->locked_region.format = bitmap_format;
      bitmap->locked_region.pitch = -pitch;
      bitmap->locked_region.pixel_size = block_size;
      return &bitmap->locked_region;
   }

   disp = al_get_current_display();

   /* Change OpenGL context if necessary. */
   if (!disp ||
       (_al_get_bitmap_display(bitmap)->ogl_extras->is_shared == false &&
        _al_get_bitmap_display(bitmap) != disp))
   {
      old_disp = disp;
      _al_set_current_display_only(_al_get_bitmap_display(bitmap));
   }

   /* Set up the pixel store state.  We will need to match it when unlocking.
    * There may be other pixel store state we should be setting.
    * See also pitfalls 7 & 8 from:
    * http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
    */
   int previous_alignment;
   glGetIntegerv(GL_PACK_ALIGNMENT, &previous_alignment);
   if (previous_alignment != 1) {
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glPixelStorei(GL_PACK_ALIGNMENT, %d) failed (%s).\n",
            1, _al_gl_error_string(e));
         ok = false;
      }
   }

   if (ok) {
      ogl_bitmap->lock_buffer = al_malloc(true_wc * true_hc * block_size);

      if (ogl_bitmap->lock_buffer != NULL) {
         glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
         glGetCompressedTexImage(GL_TEXTURE_2D, 0, ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glGetCompressedTexImage for format %s failed (%s).\n",
               _al_pixel_format_name(bitmap_format), _al_gl_error_string(e));
            al_free(ogl_bitmap->lock_buffer);
            ogl_bitmap->lock_buffer = NULL;
            ok = false;
         }
         else {
            if (flags == ALLEGRO_LOCK_READWRITE) {
               /* Need to make the locked memory contiguous, as
                * glCompressedTexSubImage2D cannot read strided
                * memory. */
               int y;
               int src_pitch = true_wc * block_size;
               int dest_pitch = wc * block_size;
               char* dest_ptr = (char*)ogl_bitmap->lock_buffer;
               char* src_ptr = (char*)ogl_bitmap->lock_buffer +
                  src_pitch * gl_yc + block_size * xc;
               for (y = 0; y < hc; y++) {
                  memmove(dest_ptr, src_ptr, dest_pitch);
                  src_ptr += src_pitch;
                  dest_ptr += dest_pitch;
               }
               bitmap->locked_region.data = ogl_bitmap->lock_buffer +
                  dest_pitch * (hc - 1);
               bitmap->locked_region.pitch = -dest_pitch;
            }
            else {
               int pitch = true_wc * block_size;
               bitmap->locked_region.data = ogl_bitmap->lock_buffer +
                  pitch * (gl_yc + hc - 1) + block_size * xc;
               bitmap->locked_region.pitch = -pitch;
            }
            bitmap->locked_region.format = bitmap_format;
            bitmap->locked_region.pixel_size = block_size;
         }
      }
      else {
         ok = false;
      }
   }

   if (previous_alignment != 1) {
      glPixelStorei(GL_PACK_ALIGNMENT, previous_alignment);
   }

   if (old_disp != NULL) {
      _al_set_current_display_only(old_disp);
   }

   if (ok) {
      ogl_flip_blocks(&bitmap->locked_region, wc, hc);
      return &bitmap->locked_region;
   }

   ALLEGRO_ERROR("Failed to lock region\n");
   ASSERT(ogl_bitmap->lock_buffer == NULL);
   return NULL;
#else
   (void)bitmap;
   (void)x;
   (void)y;
   (void)w;
   (void)h;
   (void)flags;
   return NULL;
#endif
}


static void ogl_unlock_compressed_region(ALLEGRO_BITMAP *bitmap)
{
#if !defined ALLEGRO_CFG_OPENGLES
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   int lock_format = bitmap->locked_region.format;
   ALLEGRO_DISPLAY *old_disp = NULL;
   ALLEGRO_DISPLAY *disp;
   GLenum e;
   int block_size = al_get_pixel_block_size(lock_format);
   int block_width = al_get_pixel_block_width(lock_format);
   int block_height = al_get_pixel_block_height(lock_format);
   int data_size = bitmap->lock_h * bitmap->lock_w /
      (block_width * block_height) * block_size;
   int gl_y = _al_get_least_multiple(bitmap->h, block_height) - bitmap->lock_y - bitmap->lock_h;

   /* It shouldn't be possible for this to fail, as we wouldn't have been able
    * to lock earlier */
   ASSERT(can_flip_blocks(bitmap->locked_region.format));

   if ((bitmap->lock_flags & ALLEGRO_LOCK_READONLY)) {
      goto EXIT;
   }

   ogl_flip_blocks(&bitmap->locked_region, bitmap->lock_w / block_width,
      bitmap->lock_h / block_height);

   disp = al_get_current_display();

   /* Change OpenGL context if necessary. */
   if (!disp ||
      (_al_get_bitmap_display(bitmap)->ogl_extras->is_shared == false &&
       _al_get_bitmap_display(bitmap) != disp))
   {
      old_disp = disp;
      _al_set_current_display_only(_al_get_bitmap_display(bitmap));
   }

   /* Keep this in sync with ogl_lock_compressed_region. */
   int previous_alignment;
   glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_alignment);
   if (previous_alignment != 1) {
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glPixelStorei(GL_UNPACK_ALIGNMENT, %d) failed (%s).\n",
            1, _al_gl_error_string(e));
      }
   }

   glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   glCompressedTexSubImage2D(GL_TEXTURE_2D, 0,
      bitmap->lock_x, gl_y,
      bitmap->lock_w, bitmap->lock_h,
      get_glformat(lock_format, 0),
      data_size,
      ogl_bitmap->lock_buffer);

   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glCompressedTexSubImage2D for format %s failed (%s).\n",
         _al_pixel_format_name(lock_format), _al_gl_error_string(e));
   }

   if (previous_alignment != 1) {
      glPixelStorei(GL_UNPACK_ALIGNMENT, previous_alignment);
   }

   if (old_disp) {
      _al_set_current_display_only(old_disp);
   }

EXIT:
   al_free(ogl_bitmap->lock_buffer);
   ogl_bitmap->lock_buffer = NULL;
#else
   (void)bitmap;
#endif
}

static void ogl_backup_dirty_bitmap(ALLEGRO_BITMAP *b)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = b->extra;
   ALLEGRO_LOCKED_REGION *lr;
   int bitmap_flags = al_get_bitmap_flags(b);

   if (b->parent)
      return;

   if ((bitmap_flags & ALLEGRO_MEMORY_BITMAP) ||
      (bitmap_flags & ALLEGRO_NO_PRESERVE_TEXTURE) ||
      !b->dirty ||
      ogl_bitmap->is_backbuffer)
      return;

   ALLEGRO_DEBUG("Backing up dirty bitmap %p\n", b);

   lr = al_lock_bitmap(
      b,
      _al_get_bitmap_memory_format(b),
      ALLEGRO_LOCK_READONLY
   );

   if (lr) {
      int line_size = al_get_pixel_size(lr->format) * b->w;
      int y;
      for (y = 0; y < b->h; y++) {
         unsigned char *p = ((unsigned char *)lr->data) + lr->pitch * y;
         unsigned char *p2;
         p2 = ((unsigned char *)b->memory) + line_size * (b->h-1-y);
         memcpy(p2, p, line_size);
      }
      al_unlock_bitmap(b);
      b->dirty = false;
   }
   else {
      ALLEGRO_WARN("Failed to lock dirty bitmap %p\n", b);
   }
}

/* Obtain a reference to this driver. */
static ALLEGRO_BITMAP_INTERFACE *ogl_bitmap_driver(void)
{
   if (glbmp_vt.draw_bitmap_region) {
      return &glbmp_vt;
   }

   glbmp_vt.draw_bitmap_region = ogl_draw_bitmap_region;
   glbmp_vt.upload_bitmap = ogl_upload_bitmap;
   glbmp_vt.update_clipping_rectangle = ogl_update_clipping_rectangle;
   glbmp_vt.destroy_bitmap = ogl_destroy_bitmap;
   glbmp_vt.bitmap_pointer_changed = ogl_bitmap_pointer_changed;
#if defined(ALLEGRO_CFG_OPENGLES)
   glbmp_vt.lock_region = _al_ogl_lock_region_gles;
   glbmp_vt.unlock_region = _al_ogl_unlock_region_gles;
#else
   glbmp_vt.lock_region = _al_ogl_lock_region_new;
   glbmp_vt.unlock_region = _al_ogl_unlock_region_new;
#endif
   glbmp_vt.lock_compressed_region = ogl_lock_compressed_region;
   glbmp_vt.unlock_compressed_region = ogl_unlock_compressed_region;
   glbmp_vt.backup_dirty_bitmap = ogl_backup_dirty_bitmap;

   return &glbmp_vt;
}



ALLEGRO_BITMAP *_al_ogl_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h,
   int format, int flags)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra;
   int true_w;
   int true_h;
   int block_width;
   int block_height;
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   (void)d;

   format = _al_get_real_pixel_format(d, format);
   ASSERT(_al_pixel_format_is_real(format));

   block_width = al_get_pixel_block_width(format);
   block_height = al_get_pixel_block_width(format);
   true_w = _al_get_least_multiple(w, block_width);
   true_h = _al_get_least_multiple(h, block_height);

   if (_al_pixel_format_is_compressed(format)) {
      if (!al_get_opengl_extension_list()->ALLEGRO_GL_EXT_texture_compression_s3tc) {
         ALLEGRO_DEBUG("Device does not support S3TC compressed textures.\n");
         return NULL;
      }
   }

   if (!d->extra_settings.settings[ALLEGRO_SUPPORT_NPOT_BITMAP]) {
      true_w = pot(true_w);
      true_h = pot(true_h);
   }

   /* This used to be an iOS/Android only workaround - but
    * Intel is making GPUs with the same chips now. Very
    * small textures can have garbage pixels and FBOs don't
    * work with them on some of these chips. This is a
    * workaround.
    */
   if (true_w < system->min_bitmap_size) true_w = system->min_bitmap_size;
   if (true_h < system->min_bitmap_size) true_h = system->min_bitmap_size;

   /* glReadPixels requires 32 byte aligned rows */
   if (IS_ANDROID) {
      int mod = true_w % 32;
      if (mod != 0) {
         true_w += 32 - mod;
      }
   }

   ASSERT(true_w % block_width == 0);
   ASSERT(true_h % block_height == 0);

   bitmap = al_calloc(1, sizeof *bitmap);
   ASSERT(bitmap);
   bitmap->extra = al_calloc(1, sizeof(ALLEGRO_BITMAP_EXTRA_OPENGL));
   ASSERT(bitmap->extra);
   extra = bitmap->extra;

   bitmap->vt = ogl_bitmap_driver();
   bitmap->_memory_format =
      _al_pixel_format_is_compressed(format) ? ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE : format;
   bitmap->pitch = true_w * al_get_pixel_size(bitmap->_memory_format);
   bitmap->_format = format;
   bitmap->_flags = flags | _ALLEGRO_INTERNAL_OPENGL;

   extra->true_w = true_w;
   extra->true_h = true_h;

   if (!(flags & ALLEGRO_NO_PRESERVE_TEXTURE)) {
      bitmap->memory = al_calloc(1, al_get_pixel_size(bitmap->_memory_format)*w*h);
   }

   return bitmap;
}



/* lets you setup the memory pointer to skip a lock/unlock copy
 * if it's unnecessary
 * 'ptr' should be tightly packed or NULL if no texture data
 * upload is desired.
 */
void _al_ogl_upload_bitmap_memory(ALLEGRO_BITMAP *bitmap, int format, void *ptr)
{
   int w = bitmap->w;
   int h = bitmap->h;
   int pixsize = al_get_pixel_size(format);
   int y;
   ALLEGRO_BITMAP *tmp;
   ALLEGRO_LOCKED_REGION *lr;
   uint8_t *dst;
   uint8_t *src;

   ASSERT(al_get_current_display() == _al_get_bitmap_display(bitmap));

   tmp = _al_create_bitmap_params(_al_get_bitmap_display(bitmap), w, h, format,
      al_get_bitmap_flags(bitmap), 0, 0);
   ASSERT(tmp);

   if (ptr != NULL) {
      lr = al_lock_bitmap(tmp, format, ALLEGRO_LOCK_WRITEONLY);
      ASSERT(lr);

      dst = (uint8_t *)lr->data;
      // we need to flip it
      src = ((uint8_t *)ptr) + (pixsize * w * (h-1));

      for (y = 0; y < h; y++) {
         memcpy(dst, src, pixsize * w);
         dst += lr->pitch;
         src -= pixsize * w; // minus because it's flipped
      }

      al_unlock_bitmap(tmp);
   }

   ((ALLEGRO_BITMAP_EXTRA_OPENGL *)bitmap->extra)->texture =
      ((ALLEGRO_BITMAP_EXTRA_OPENGL *)tmp->extra)->texture;
   ((ALLEGRO_BITMAP_EXTRA_OPENGL *)tmp->extra)->texture = 0;
   al_destroy_bitmap(tmp);
}

/* Function: al_get_opengl_texture
 */
GLuint al_get_opengl_texture(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra;
   if (bitmap->parent)
      bitmap = bitmap->parent;
   if (!(al_get_bitmap_flags(bitmap) & _ALLEGRO_INTERNAL_OPENGL))
      return 0;
   extra = bitmap->extra;
   return extra->texture;
}

/* Function: al_remove_opengl_fbo
 */
void al_remove_opengl_fbo(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap;
   if (bitmap->parent)
      bitmap = bitmap->parent;
   if (!(al_get_bitmap_flags(bitmap) & _ALLEGRO_INTERNAL_OPENGL))
      return;
   ogl_bitmap = bitmap->extra;
   if (!ogl_bitmap->fbo_info)
      return;

   ASSERT(ogl_bitmap->fbo_info->fbo_state > FBO_INFO_UNUSED);
   ASSERT(ogl_bitmap->fbo_info->fbo != 0);

   ALLEGRO_FBO_INFO *info = ogl_bitmap->fbo_info;
   _al_ogl_del_fbo(info);

   if (info->fbo_state == FBO_INFO_PERSISTENT) {
      al_free(info);
   }
   else {
      _al_ogl_reset_fbo_info(info);
   }
}

/* Function: al_get_opengl_fbo
 */
GLuint al_get_opengl_fbo(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap;
   if (bitmap->parent)
      bitmap = bitmap->parent;

   if (!(al_get_bitmap_flags(bitmap) & _ALLEGRO_INTERNAL_OPENGL))
      return 0;

   ogl_bitmap = bitmap->extra;

   if (!ogl_bitmap->fbo_info) {
      if (!_al_ogl_create_persistent_fbo(bitmap)) {
         return 0;
      }
   }

   if (ogl_bitmap->fbo_info->fbo_state == FBO_INFO_TRANSIENT) {
      ogl_bitmap->fbo_info = _al_ogl_persist_fbo(_al_get_bitmap_display(bitmap),
         ogl_bitmap->fbo_info);
   }
   return ogl_bitmap->fbo_info->fbo;
}

/* Function: al_get_opengl_texture_size
 */
bool al_get_opengl_texture_size(ALLEGRO_BITMAP *bitmap, int *w, int *h)
{
   /* The designers of OpenGL ES 1.0 forgot to add a function to query
    * texture sizes, so this will be the only way there to get the texture
    * size. On normal OpenGL also glGetTexLevelParameter could be used.
    */
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap;
   if (bitmap->parent)
      bitmap = bitmap->parent;
   
   if (!(al_get_bitmap_flags(bitmap) & _ALLEGRO_INTERNAL_OPENGL)) {
      *w = 0;
      *h = 0;
      return false;
   }

   ogl_bitmap = bitmap->extra;
   *w = ogl_bitmap->true_w;
   *h = ogl_bitmap->true_h;
   return true;
}

/* Function: al_get_opengl_texture_position
 */
void al_get_opengl_texture_position(ALLEGRO_BITMAP *bitmap, int *u, int *v)
{
   ASSERT(bitmap);
   ASSERT(u);
   ASSERT(v);

   *u = bitmap->xofs;
   *v = bitmap->yofs;
}

/* vim: set sts=3 sw=3 et: */
