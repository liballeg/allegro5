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
   };
  
   if (al_get_opengl_version() >= _ALLEGRO_OPENGL_VERSION_3_0) {
      glformats[ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8][0] = GL_RED;
      glformats[ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8][2] = GL_RED;
   }
   #else
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
      ERR(GL_STACK_OVERFLOW)
      ERR(GL_STACK_UNDERFLOW)
      ERR(GL_OUT_OF_MEMORY)
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      ERR(GL_INVALID_FRAMEBUFFER_OPERATION)
#endif
   }
   return "UNKNOWN";
}
#undef ERR

static INLINE void transform_vertex(float* x, float* y)
{
   al_transform_coordinates(al_get_current_transform(), x, y);
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
   verts[0].tx = tex_l;
   verts[0].ty = tex_b;
   verts[0].r = tint.r;
   verts[0].g = tint.g;
   verts[0].b = tint.b;
   verts[0].a = tint.a;
   
   verts[1].x = 0;
   verts[1].y = 0;
   verts[1].tx = tex_l;
   verts[1].ty = tex_t;
   verts[1].r = tint.r;
   verts[1].g = tint.g;
   verts[1].b = tint.b;
   verts[1].a = tint.a;
   
   verts[2].x = dw;
   verts[2].y = dh;
   verts[2].tx = tex_r;
   verts[2].ty = tex_b;
   verts[2].r = tint.r;
   verts[2].g = tint.g;
   verts[2].b = tint.b;
   verts[2].a = tint.a;
   
   verts[4].x = dw;
   verts[4].y = 0;
   verts[4].tx = tex_r;
   verts[4].ty = tex_t;
   verts[4].r = tint.r;
   verts[4].g = tint.g;
   verts[4].b = tint.b;
   verts[4].a = tint.a;
   
   if (disp->cache_enabled) {
      /* If drawing is batched, we apply transformations manually. */
      transform_vertex(&verts[0].x, &verts[0].y);
      transform_vertex(&verts[1].x, &verts[1].y);
      transform_vertex(&verts[2].x, &verts[2].y);
      transform_vertex(&verts[4].x, &verts[4].y);
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
   ALLEGRO_DISPLAY *disp = target->display;

   /* For sub-bitmaps */
   if (target->parent) {
      target = target->parent;
   }

   ogl_target = target->extra;

   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP) && !bitmap->locked &&
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
      if (_al_opengl_set_blender(disp)) {
         draw_quad(bitmap, tint, sx, sy, sw, sh, flags);
         return;
      }
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


// FIXME: need to do all the logic AllegroGL does, checking extensions,
// proxy textures, formats, limits ...
static bool ogl_upload_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   int w = bitmap->w;
   int h = bitmap->h;
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
                    _al_pixel_format_name(bitmap->format));
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
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   filter = (bitmap->flags & ALLEGRO_MIPMAP) ? 2 : 0;
   if (bitmap->flags & ALLEGRO_MIN_LINEAR) {
      filter++;
   }
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filters[filter]);

   filter = 0;
   if (bitmap->flags & ALLEGRO_MAG_LINEAR) {
      filter++;
   }
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filters[filter]);

   if (post_generate_mipmap) {
      glGenerateMipmapEXT(GL_TEXTURE_2D);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glGenerateMipmapEXT for texture %d failed (%s).\n",
            ogl_bitmap->texture, _al_gl_error_string(e));
      }
   }

// TODO: To support anisotropy, we would need an API for it. Something
// like:
// al_set_new_bitmap_option(ALLEGRO_ANISOTROPY, 16.0);
#if 0
   if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_texture_filter_anisotropic) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
   }
#endif

   if (bitmap->flags & ALLEGRO_MIPMAP) {
      /* If using FBOs, use glGenerateMipmapEXT instead of the GL_GENERATE_MIPMAP
       * texture parameter.  GL_GENERATE_MIPMAP is deprecated in GL 3.0 so we
       * may want to use the new method in other cases as well.
       */
      if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object) {
         post_generate_mipmap = true;
      }
      else {
         glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
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
            bitmap->format == ALLEGRO_PIXEL_FORMAT_ABGR_F32) {
         unsigned char *buf;
         buf = al_calloc(ogl_bitmap->true_h, ogl_bitmap->true_w);
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         glTexImage2D(GL_TEXTURE_2D, 0, get_glformat(bitmap->format, 0),
            ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
            GL_ALPHA, GL_UNSIGNED_BYTE, buf);
         e = glGetError();
         al_free(buf);
      }
      else {
         glTexImage2D(GL_TEXTURE_2D, 0, get_glformat(bitmap->format, 0),
            ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
            get_glformat(bitmap->format, 2), get_glformat(bitmap->format, 1),
            NULL);
         e = glGetError();
      }
   }
   else {
      unsigned char *buf;
      int pix_size = al_get_pixel_size(bitmap->format);
      buf = al_calloc(pix_size,
         ogl_bitmap->true_h * ogl_bitmap->true_w);
      glPixelStorei(GL_UNPACK_ALIGNMENT, pix_size);
      glTexImage2D(GL_TEXTURE_2D, 0, get_glformat(bitmap->format, 0),
         ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
         get_glformat(bitmap->format, 2),
         get_glformat(bitmap->format, 1), buf);
      al_free(buf);
   }

   if (e) {
      ALLEGRO_ERROR("glTexImage2D for format %s, size %dx%d failed (%s)\n",
         _al_pixel_format_name(bitmap->format),
         ogl_bitmap->true_w, ogl_bitmap->true_h,
         _al_gl_error_string(e));
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
      // FIXME: Should we convert it into a memory bitmap? Or if the size is
      // the problem try to use multiple textures?
      return false;
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
   ALLEGRO_DISPLAY *old_disp = NULL;
   
   if (bitmap->parent) {
      al_free(ogl_bitmap);
      return;
   }

   disp = al_get_current_display();
   if (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != disp) {
      old_disp = disp;
      _al_set_current_display_only(bitmap->display);
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

   return &glbmp_vt;
}



ALLEGRO_BITMAP *_al_ogl_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h,
   int format, int flags)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra;
   int true_w;
   int true_h;
   int pitch;
   (void)d;

   /* Android included because some devices require POT FBOs */
   if (!IS_OPENGLES &&
      d->extra_settings.settings[ALLEGRO_SUPPORT_NPOT_BITMAP])
   {
      true_w = w;
      true_h = h;
   }
   else {
      true_w = pot(w);
      true_h = pot(h);
   }

   /* This used to be an iOS/Android only workaround - but
    * Intel is making GPUs with the same chips now. Very
    * small textures can have garbage pixels and FBOs don't
    * work with them on some of these chips. This is a
    * workaround.
    */
   if (true_w < 16) true_w = 16;
   if (true_h < 16) true_h = 16;

   /* glReadPixels requires 32 byte aligned rows */
   if (IS_ANDROID) {
      int mod = true_w % 32;
      if (mod != 0) {
         true_w += 32 - mod;
      }
   }

   format = _al_get_real_pixel_format(d, format);

   ASSERT(_al_pixel_format_is_real(format));

   pitch = true_w * al_get_pixel_size(format);

   bitmap = al_calloc(1, sizeof *bitmap);
   ASSERT(bitmap);
   bitmap->extra = al_calloc(1, sizeof(ALLEGRO_BITMAP_EXTRA_OPENGL));
   ASSERT(bitmap->extra);
   extra = bitmap->extra;

   bitmap->vt = ogl_bitmap_driver();
   bitmap->pitch = pitch;
   bitmap->format = format;
   bitmap->flags = flags | _ALLEGRO_INTERNAL_OPENGL;

   extra->true_w = true_w;
   extra->true_h = true_h;

   if (!(flags & ALLEGRO_NO_PRESERVE_TEXTURE)) {
      bitmap->memory = al_calloc(1, al_get_pixel_size(format)*w*h);
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

   ASSERT(ptr);
   ASSERT(al_get_current_display() == bitmap->display);

   tmp = _al_create_bitmap_params(bitmap->display, w, h, format,
      bitmap->flags);
   ASSERT(tmp);

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
   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL))
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
   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL))
      return;
   ogl_bitmap = bitmap->extra;
   if (!ogl_bitmap->fbo_info)
      return;

   ASSERT(ogl_bitmap->fbo_info->fbo_state > FBO_INFO_UNUSED);
   ASSERT(ogl_bitmap->fbo_info->fbo != 0);
   ALLEGRO_DEBUG("Deleting FBO: %u\n", ogl_bitmap->fbo_info->fbo);

   glDeleteFramebuffersEXT(1, &ogl_bitmap->fbo_info->fbo);
   ogl_bitmap->fbo_info->fbo = 0;

   if (ogl_bitmap->fbo_info->fbo_state == FBO_INFO_PERSISTENT) {
      al_free(ogl_bitmap->fbo_info);
   }
   else {
      _al_ogl_reset_fbo_info(ogl_bitmap->fbo_info);
   }
   ogl_bitmap->fbo_info = NULL;
}

/* Function: al_get_opengl_fbo
 */
GLuint al_get_opengl_fbo(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap;
   if (bitmap->parent)
      bitmap = bitmap->parent;

   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL))
      return 0;

   ogl_bitmap = bitmap->extra;

   if (!ogl_bitmap->fbo_info) {
      if (!_al_ogl_create_persistent_fbo(bitmap)) {
         return 0;
      }
   }

   if (ogl_bitmap->fbo_info->fbo_state == FBO_INFO_TRANSIENT) {
      ogl_bitmap->fbo_info = _al_ogl_persist_fbo(bitmap->display,
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
   
   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL)) {
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

void _al_opengl_backup_dirty_bitmaps(ALLEGRO_DISPLAY *d, bool flip)
{
   int i, y;

   for (i = 0; i < (int)d->bitmaps._size; i++) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref(&d->bitmaps, i);
      ALLEGRO_BITMAP *b = *bptr;
      ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = b->extra;
      ALLEGRO_LOCKED_REGION *lr;
      if (b->parent)
         continue;
      if ((b->flags & ALLEGRO_MEMORY_BITMAP) ||
         (b->flags & ALLEGRO_NO_PRESERVE_TEXTURE) ||
         !b->dirty ||
         ogl_bitmap->is_backbuffer)
         continue;
      ALLEGRO_DEBUG("Backing up dirty bitmap %p\n", b);
      lr = al_lock_bitmap(
         b,
         ALLEGRO_PIXEL_FORMAT_ANY,
         ALLEGRO_LOCK_READONLY
      );
      if (lr) {
         int line_size = al_get_pixel_size(b->format) * b->w;
         for (y = 0; y < b->h; y++) {
            unsigned char *p = ((unsigned char *)lr->data) + lr->pitch * y;
            unsigned char *p2;
            if (flip) {
               p2 = ((unsigned char *)b->memory) + line_size * (b->h-1-y);
            }
            else {
               p2 = ((unsigned char *)b->memory) + line_size * y;
            }
            memcpy(p2, p, line_size);
         }
         al_unlock_bitmap(b);
         b->dirty = false;
      }
      else {
         ALLEGRO_WARN("Failed to lock dirty bitmap %p\n", b);
      }
   }
}

/* vim: set sts=3 sw=3 et: */
