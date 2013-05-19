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

/* FIXME: Add the other format for gp2xwiz 1555 or 5551 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memblit.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_transform.h"
#include <math.h>

#if defined ALLEGRO_GP2XWIZ
#include "allegro5/internal/aintern_gp2xwiz.h"
#endif

#ifdef ALLEGRO_IPHONE
#define glGenerateMipmapEXT glGenerateMipmapOES
#endif

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
 */
#if !defined(ALLEGRO_GP2XWIZ) && !defined(ALLEGRO_IPHONE)
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
   {GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA} /* RGBA_4444 */
};
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
   {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA} /* RGBA_4444 */
};
#endif

static ALLEGRO_BITMAP_INTERFACE glbmp_vt;


#define SWAP(type, x, y) {type temp = x; x = y; y = temp;}

#define ERR(e) case e: return #e;
static char const *error_string(GLenum e)
{
   switch (e) {
      ERR(GL_NO_ERROR)
      ERR(GL_INVALID_ENUM)
      ERR(GL_INVALID_VALUE)
      ERR(GL_INVALID_OPERATION)
      ERR(GL_STACK_OVERFLOW)
      ERR(GL_STACK_UNDERFLOW)
      ERR(GL_OUT_OF_MEMORY)
      ERR(GL_INVALID_FRAMEBUFFER_OPERATION)
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
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
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
   ALLEGRO_BITMAP_OGL *ogl_target;
   ALLEGRO_DISPLAY *disp = target->display;

   /* For sub-bitmaps */
   if (target->parent) {
      target = target->parent;
   }

   ogl_target = (ALLEGRO_BITMAP_OGL *)target;

   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP) && !bitmap->locked &&
         !target->locked) {
#if !defined ALLEGRO_GP2XWIZ
      ALLEGRO_BITMAP_OGL *ogl_source = (void *)bitmap;
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
#elif defined ALLEGRO_GP2XWIZ
      /* FIXME: make this work somehow on Wiz */
      return;
#endif
   }
   if (disp->ogl_extras->opengl_target == ogl_target) {
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
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
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
   }
   glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glBindTexture for texture %d failed (%s).\n",
         ogl_bitmap->texture, error_string(e));
   }

   /* Wrap, Min/Mag should always come before glTexImage2D so the texture is "complete" */
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
            ogl_bitmap->texture, error_string(e));
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

#ifndef ALLEGRO_IPHONE
   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
#endif
   if (bitmap->memory == NULL) {
      /* If there's unused space around the bitmap, we need to clear it
       * else linear filtering will cause artifacts from the random
       * data there. We also clear for floating point formats because
       * NaN values in the texture cause some blending modes to fail on
       * those pixels
       */
      if (ogl_bitmap->true_w != bitmap->w ||
            ogl_bitmap->true_h != bitmap->h ||
            bitmap->format == ALLEGRO_PIXEL_FORMAT_ABGR_F32) {
         unsigned char *buf;
         #ifdef ALLEGRO_IPHONE
         {
            int pix_size = al_get_pixel_size(bitmap->format);
            buf = al_calloc(pix_size,
               ogl_bitmap->true_h * ogl_bitmap->true_w);
            glPixelStorei(GL_UNPACK_ALIGNMENT, pix_size);
            glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0],
               ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
               glformats[bitmap->format][2],
               glformats[bitmap->format][1], buf);
         }
         #else
         buf = al_calloc(ogl_bitmap->true_h, ogl_bitmap->true_w);
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0],
            ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
            GL_ALPHA, GL_UNSIGNED_BYTE, buf);
         #endif
         e = glGetError();
         al_free(buf);
      }
      else {
         glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0],
            ogl_bitmap->true_w, ogl_bitmap->true_h, 0,
            glformats[bitmap->format][2], glformats[bitmap->format][1],
            NULL);
         e = glGetError();
      }
   }
   else {
      glPixelStorei(GL_UNPACK_ALIGNMENT, al_get_pixel_size(bitmap->format));
      glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0],
         ogl_bitmap->true_w, ogl_bitmap->true_h, 0, glformats[bitmap->format][2],
         glformats[bitmap->format][1], bitmap->memory);
      al_free(bitmap->memory);
      bitmap->memory = NULL;
      e = glGetError();
   }
#ifndef ALLEGRO_IPHONE
   glPopClientAttrib();
#endif

   if (e) {
      ALLEGRO_ERROR("glTexImage2D for format %s, size %dx%d failed (%s)\n",
         _al_pixel_format_name(bitmap->format),
         ogl_bitmap->true_w, ogl_bitmap->true_h,
         error_string(e));
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
      // FIXME: Should we convert it into a memory bitmap? Or if the size is
      // the problem try to use multiple textures?
      return false;
   }

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

// TODO: To support anisotropy, we would need an API for it. Something
// like:
// al_set_new_bitmap_option(ALLEGRO_ANISOTROPY, 16.0);
#if 0
   if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_texture_filter_anisotropic) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
   }
#endif

   if (post_generate_mipmap) {
      glGenerateMipmapEXT(GL_TEXTURE_2D);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glGenerateMipmapEXT for texture %d failed (%s).\n",
            ogl_bitmap->texture, error_string(e));
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
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;

   if (bitmap->parent) {
      ogl_bitmap = (ALLEGRO_BITMAP_OGL *)bitmap->parent;
   }

   if (ogl_disp->ogl_extras->opengl_target == ogl_bitmap) {
      _al_ogl_setup_bitmap_clipping(bitmap);
   }
}


static int ogl_pixel_alignment(int pixel_size)
{
   /* Valid alignments are: 1, 2, 4, 8 bytes. */
   switch (pixel_size) {
      case 1:
      case 2:
      case 4:
      case 8:
         return pixel_size;
      case 3:
         return 1;
      case 16: /* float32 */
         return 4;
      default:
         ASSERT(false);
         return 4;
   }
}


static int ogl_pitch(int w, int pixel_size)
{
   int pitch = w * pixel_size;
   return pitch;
}


static bool exactly_15bpp(int pixel_format)
{
   return pixel_format == ALLEGRO_PIXEL_FORMAT_RGB_555
      || pixel_format == ALLEGRO_PIXEL_FORMAT_BGR_555;
}


/* OpenGL cannot "lock" pixels, so instead we update our memory copy and
 * return a pointer into that.
 */
static ALLEGRO_LOCKED_REGION *ogl_lock_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int w, int h, int format, int flags)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   int pixel_size;
   int pixel_alignment;
   int pitch;
   ALLEGRO_DISPLAY *disp;
   ALLEGRO_DISPLAY *old_disp = NULL;
   GLint gl_y = bitmap->h - y - h;
   GLenum e;

   if (format == ALLEGRO_PIXEL_FORMAT_ANY)
      format = bitmap->format;

   disp = al_get_current_display();
   format = _al_get_real_pixel_format(disp, format);

   pixel_size = al_get_pixel_size(format);
   pixel_alignment = ogl_pixel_alignment(pixel_size);

   if (bitmap->display->ogl_extras->is_shared == false &&
         bitmap->display != disp) {
      old_disp = disp;
      _al_set_current_display_only(bitmap->display);
   }

   /* Set up the pixel store state.  We will need to match it when unlocking.
    * There may be other pixel store state we should be setting.
    * See also pitfalls 7 & 8 from:
    * http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
    */
#ifndef ALLEGRO_IPHONE
   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
#endif
   glPixelStorei(GL_PACK_ALIGNMENT, pixel_alignment);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glPixelStorei(GL_PACK_ALIGNMENT, %d) failed (%s).\n",
         pixel_alignment, error_string(e));
   }

   if (ogl_bitmap->is_backbuffer) {
      ALLEGRO_DEBUG("Locking backbuffer\n");

      pitch = ogl_pitch(w, pixel_size);
      ogl_bitmap->lock_buffer = al_malloc(pitch * h);

      if (!(flags & ALLEGRO_LOCK_WRITEONLY)) {
         glReadPixels(x, gl_y, w, h,
            glformats[format][2],
            glformats[format][1],
            ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glReadPixels for format %s failed (%s).\n",
               _al_pixel_format_name(format), error_string(e));
         }
      }
      bitmap->locked_region.data = ogl_bitmap->lock_buffer +
         pitch * (h - 1);
   }
#if !defined ALLEGRO_GP2XWIZ
   else {
      if (flags & ALLEGRO_LOCK_WRITEONLY) {
         ALLEGRO_DEBUG("Locking non-backbuffer WRITEONLY\n");

         pitch = ogl_pitch(w, pixel_size);
         ogl_bitmap->lock_buffer = al_malloc(pitch * h);
         bitmap->locked_region.data = ogl_bitmap->lock_buffer +
            pitch * (h - 1);
      }
      else {
         ALLEGRO_DEBUG("Locking non-backbuffer READWRITE\n");

         #ifdef ALLEGRO_IPHONE
            GLint current_fbo;
            unsigned char *tmpbuf;
            int tmp_pitch;

            /* Create an FBO if there isn't one. */
            if (!ogl_bitmap->fbo_info) {
               ALLEGRO_STATE state;
               al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
               bitmap->locked = false; // hack :(
               al_set_target_bitmap(bitmap); // This creates the fbo
               bitmap->locked = true;
               al_restore_state(&state);
            }

            glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &current_fbo);
            glBindFramebufferOES(GL_FRAMEBUFFER_OES, ogl_bitmap->fbo_info->fbo);
            e = glGetError();
            if (e) {
               ALLEGRO_ERROR("glBindFramebufferOES failed.\n");
            }

            pitch = ogl_pitch(w, pixel_size);
            ogl_bitmap->lock_buffer = al_malloc(pitch * h);
           
            tmp_pitch = ogl_pitch(w, 4);
            tmpbuf = al_malloc(tmp_pitch * h);

            /* NOTE: GLES 1.1 can only read 4 byte pixels, we have to convert */
            glReadPixels(x, gl_y, w, h,
               GL_RGBA, GL_UNSIGNED_BYTE,
               tmpbuf);
            e = glGetError();
            if (e) {
               ALLEGRO_ERROR("glReadPixels for format %s failed (%s).\n",
                  _al_pixel_format_name(format), error_string(e));
            }

            _al_convert_bitmap_data(
               tmpbuf,
               ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
               tmp_pitch,
               ogl_bitmap->lock_buffer,
               format,
               pitch,
               0, 0, 0, 0,
               w, h);

            al_free(tmpbuf);

            glBindFramebufferOES(GL_FRAMEBUFFER_OES, current_fbo);

            bitmap->locked_region.data = ogl_bitmap->lock_buffer +
               pitch * (h - 1);
         #else
         // FIXME: Using glGetTexImage means we always read the complete
         // texture - even when only a single pixel is locked. Likely
         // using FBO and glReadPixels to just read the locked part
         // would be faster.
         pitch = ogl_pitch(ogl_bitmap->true_w, pixel_size);
         ogl_bitmap->lock_buffer = al_malloc(pitch * ogl_bitmap->true_h);

         glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
         glGetTexImage(GL_TEXTURE_2D, 0, glformats[format][2],
            glformats[format][1], ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glGetTexImage for format %s failed (%s).\n",
               _al_pixel_format_name(format), error_string(e));
         }

         bitmap->locked_region.data = ogl_bitmap->lock_buffer +
            pitch * (gl_y + h - 1) + pixel_size * x;
         #endif
      }
   }
#else
   else {
      if (flags & ALLEGRO_LOCK_WRITEONLY) {
         pitch = ogl_pitch(w, pixel_size);
         ogl_bitmap->lock_buffer = al_malloc(pitch * h);
         bitmap->locked_region.data = ogl_bitmap->lock_buffer;
         pitch = -pitch;
      }
      else {
         /* FIXME: implement */
         return NULL;
      }
   }
#endif
#ifndef ALLEGRO_IPHONE
   glPopClientAttrib();
#endif

   bitmap->locked_region.format = format;
   bitmap->locked_region.pitch = -pitch;
   bitmap->locked_region.pixel_size = pixel_size;

   if (old_disp) {
      _al_set_current_display_only(old_disp);
   }

   return &bitmap->locked_region;
}



/* Synchronizes the texture back to the (possibly modified) bitmap data.
 */
static void ogl_unlock_region(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   const int lock_format = bitmap->locked_region.format;
   ALLEGRO_DISPLAY *disp;
   ALLEGRO_DISPLAY *old_disp = NULL;
   GLenum e;
   GLint gl_y = bitmap->h - bitmap->lock_y - bitmap->lock_h;
   int orig_format;
#ifdef ALLEGRO_IPHONE
   int orig_pixel_size;
#endif
   int lock_pixel_size;
   int pixel_alignment;
   bool biased_alpha = false;
   (void)e;

   if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY) {
      goto Done;
   }

   disp = al_get_current_display();
   orig_format = _al_get_real_pixel_format(disp, bitmap->format);
   /* Not used in all code paths. */
   (void)orig_format;

   if (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != disp) {
      old_disp = disp;
      _al_set_current_display_only(bitmap->display);
   }

   /* Keep this in sync with ogl_lock_region. */
#ifndef ALLEGRO_IPHONE
   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
#endif
   lock_pixel_size = al_get_pixel_size(lock_format);
   pixel_alignment = ogl_pixel_alignment(lock_pixel_size);
   glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_alignment);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glPixelStorei(GL_UNPACK_ALIGNMENT, %d) failed (%s).\n",
         pixel_alignment, error_string(e));
   }

   if (exactly_15bpp(lock_format)) {
      /* OpenGL does not support 15-bpp internal format without an alpha,
       * so when storing such data we must ensure the alpha bit is set.
       */
      #ifndef ALLEGRO_IPHONE
      glPixelTransferi(GL_ALPHA_BIAS, 1);
      biased_alpha = true;
      #endif
   }

#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
   if (ogl_bitmap->is_backbuffer) {
      bool popmatrix = false;
      ALLEGRO_DEBUG("Unlocking backbuffer\n");

      /* glWindowPos2i may not be available. */
      if (al_get_opengl_version() >= _ALLEGRO_OPENGL_VERSION_1_4) {
         glWindowPos2i(bitmap->lock_x, gl_y);
      }
      else {
         /* glRasterPos is affected by the current modelview and projection
          * matrices (so maybe we actually need to reset both of them?).
          * The coordinate is also clipped; the small offset was required to
          * prevent it being culled on one of my machines. --pw
          *
          * Consider using glWindowPos2fMESAemulate from:
          * http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
          */
         glPushMatrix();
         glLoadIdentity();
         glRasterPos2f(bitmap->lock_x,
            bitmap->lock_y + bitmap->lock_h - 1e-4f);
         popmatrix = true;
      }
      glDisable(GL_BLEND);
      glDrawPixels(bitmap->lock_w, bitmap->lock_h,
         glformats[lock_format][2],
         glformats[lock_format][1],
         ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glDrawPixels for format %s failed (%s).\n",
            _al_pixel_format_name(lock_format), error_string(e));
      }
      if (popmatrix) {
         glPopMatrix();
      }
   }
   else {
      unsigned char *start_ptr;
      // FIXME: would FBO and glDrawPixels be any faster?
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      if (bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY) {
         ALLEGRO_DEBUG("Unlocking non-backbuffer WRITEONLY\n");
         start_ptr = ogl_bitmap->lock_buffer;
      }
      else {
         ALLEGRO_DEBUG("Unlocking non-backbuffer READWRITE\n");
         glPixelStorei(GL_UNPACK_ROW_LENGTH, ogl_bitmap->true_w);
         start_ptr = (unsigned char *)bitmap->locked_region.data
               + (bitmap->lock_h - 1) * bitmap->locked_region.pitch;
      }
      glTexSubImage2D(GL_TEXTURE_2D, 0,
         bitmap->lock_x, gl_y,
         bitmap->lock_w, bitmap->lock_h,
         glformats[lock_format][2],
         glformats[lock_format][1],
         start_ptr);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glTexSubImage2D for format %s failed (%s).\n",
            _al_pixel_format_name(lock_format), error_string(e));
      }

      if (bitmap->flags & ALLEGRO_MIPMAP) {
         /* If using FBOs, we need to regenerate mipmaps explicitly now. */
         if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object) {
            glGenerateMipmapEXT(GL_TEXTURE_2D);
            e = glGetError();
            if (e) {
               ALLEGRO_ERROR("glGenerateMipmapEXT for texture %d failed (%s).\n",
                  ogl_bitmap->texture, error_string(e));
            }
         }
      }
   }
#else /* ALLEGRO_GP2XWIZ or ALLEGRO_IPHONE */
   orig_pixel_size = al_get_pixel_size(orig_format);
   if (ogl_bitmap->is_backbuffer) {
      ALLEGRO_DEBUG("Unlocking backbuffer\n");
      GLuint tmp_tex;
      glGenTextures(1, &tmp_tex);
      glBindTexture(GL_TEXTURE_2D, tmp_tex);
      glTexImage2D(GL_TEXTURE_2D, 0, glformats[lock_format][0], bitmap->lock_w, bitmap->lock_h,
                   0, glformats[lock_format][2], glformats[lock_format][1],
                   ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         int printf(const char *, ...);
         printf("glTexImage2D failed: %d\n", e);
      }
      glDrawTexiOES(bitmap->lock_x, bitmap->lock_y, 0, bitmap->lock_w, bitmap->lock_h);
      e = glGetError();
      if (e) {
         int printf(const char *, ...);
         printf("glDrawTexiOES failed: %d\n", e);
      }
      glDeleteTextures(1, &tmp_tex);
   }
   else {
      ALLEGRO_DEBUG("Unlocking non-backbuffer\n");
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      if (bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY) {
         int dst_pitch = bitmap->lock_w * orig_pixel_size;
	 unsigned char *tmpbuf = al_malloc(dst_pitch * bitmap->lock_h);
         _al_convert_bitmap_data(
            ogl_bitmap->lock_buffer,
            bitmap->locked_region.format,
            -bitmap->locked_region.pitch,
            tmpbuf,
            orig_format,
            dst_pitch,
            0, 0, 0, 0,
            bitmap->lock_w, bitmap->lock_h);
         #ifdef ALLEGRO_IPHONE
         glPixelStorei(GL_UNPACK_ALIGNMENT, ogl_pixel_alignment(orig_pixel_size));
         glTexSubImage2D(GL_TEXTURE_2D, 0,
            bitmap->lock_x, gl_y,
            bitmap->lock_w, bitmap->lock_h,
            glformats[orig_format][2],
            glformats[orig_format][1],
            tmpbuf);
         al_free(tmpbuf);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glTexSubImage2D for format %d failed (%s).\n",
               lock_format, error_string(e));
         }
         #endif
      }
      else {
         glPixelStorei(GL_UNPACK_ALIGNMENT, ogl_pixel_alignment(orig_pixel_size));
         glTexSubImage2D(GL_TEXTURE_2D, 0, bitmap->lock_x, gl_y,
            bitmap->lock_w, bitmap->lock_h,
            glformats[lock_format][2],
            glformats[lock_format][1],
            ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            GLint tex_internalformat;
            ALLEGRO_ERROR("glTexSubImage2D for format %s failed (%s).\n",
               _al_pixel_format_name(lock_format), error_string(e));
#ifndef ALLEGRO_IPHONE
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
               GL_TEXTURE_INTERNAL_FORMAT, &tex_internalformat);

            ALLEGRO_DEBUG("x/y/w/h: %d/%d/%d/%d, internal format: %d\n",
               bitmap->lock_x, gl_y, bitmap->lock_w, bitmap->lock_h,
               tex_internalformat);
#else
            (void)tex_internalformat;
            ALLEGRO_DEBUG("x/y/w/h: %d/%d/%d/%d\n",
               bitmap->lock_x, gl_y, bitmap->lock_w, bitmap->lock_h);
#endif
         }
      }
   }
#endif

   if (biased_alpha) {
#ifndef ALLEGRO_IPHONE
      glPixelTransferi(GL_ALPHA_BIAS, 0);
#endif
   }
#ifndef ALLEGRO_IPHONE
   glPopClientAttrib();
#endif

   if (old_disp) {
      _al_set_current_display_only(old_disp);
   }

Done:

   al_free(ogl_bitmap->lock_buffer);
   ogl_bitmap->lock_buffer = NULL;
}



static void ogl_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   ALLEGRO_DISPLAY *disp;
   ALLEGRO_DISPLAY *old_disp = NULL;

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
   glbmp_vt.lock_region = ogl_lock_region;
   glbmp_vt.unlock_region = ogl_unlock_region;

   return &glbmp_vt;
}



ALLEGRO_BITMAP *_al_ogl_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_BITMAP_OGL *bitmap;
   int format = al_get_new_bitmap_format();
   const int flags = al_get_new_bitmap_flags();
   int true_w;
   int true_h;
   int pitch;
   (void)d;

   ALLEGRO_DEBUG("Creating OpenGL bitmap\n");

#if !defined ALLEGRO_GP2XWIZ
   if (d->ogl_extras->extension_list->ALLEGRO_GL_ARB_texture_non_power_of_two) {
      true_w = w;
      true_h = h;
   }
   else {
      true_w = pot(w);
      true_h = pot(h);
   }
#else
   true_w = pot(w);
   true_h = pot(h);
#endif

/* FBOs are 16x16 minimum on iPhone, this is a workaround */
#ifdef ALLEGRO_IPHONE
   if (true_w < 16) true_w = 16;
   if (true_h < 16) true_h = 16;
#endif

   ALLEGRO_DEBUG("Using dimensions: %d %d\n", true_w, true_h);

#if !defined ALLEGRO_GP2XWIZ
   format = _al_get_real_pixel_format(d, format);
#else
   if (format != ALLEGRO_PIXEL_FORMAT_RGB_565 && format != ALLEGRO_PIXEL_FORMAT_RGBA_4444)
      format = ALLEGRO_PIXEL_FORMAT_RGBA_4444;
#endif

   ALLEGRO_DEBUG("Chose format %s for OpenGL bitmap\n", _al_pixel_format_name(format));
   ASSERT(_al_pixel_format_is_real(format));

   pitch = true_w * al_get_pixel_size(format);

   bitmap = al_malloc(sizeof *bitmap);
   ASSERT(bitmap);
   memset(bitmap, 0, sizeof *bitmap);
   bitmap->bitmap.size = sizeof *bitmap;
   bitmap->bitmap.vt = ogl_bitmap_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.pitch = pitch;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags | _ALLEGRO_INTERNAL_OPENGL;
   bitmap->bitmap.cl = 0;
   bitmap->bitmap.ct = 0;
   bitmap->bitmap.cr_excl = w;
   bitmap->bitmap.cb_excl = h;
   /* Back and front buffers should share a transform, which they do
    * because our OpenGL driver returns the same pointer for both.
    */
   al_identity_transform(&bitmap->bitmap.transform);

   bitmap->true_w = true_w;
   bitmap->true_h = true_h;

#if !defined(ALLEGRO_IPHONE) && !defined(ALLEGRO_GP2XWIZ)
   bitmap->bitmap.memory = NULL;
#else
   /* iPhone/Wiz ports still expect the buffer to be present. */
   {
      size_t bytes = pitch * true_h;

      /* We never allow un-initialized memory for OpenGL bitmaps, if it
       * is uploaded to a floating point texture it can lead to Inf and
       * NaN values which break all subsequent blending.
       */
      bitmap->bitmap.memory = al_calloc(1, bytes);
   }
#endif

   return &bitmap->bitmap;
}



ALLEGRO_BITMAP *_al_ogl_create_sub_bitmap(ALLEGRO_DISPLAY *d,
                                          ALLEGRO_BITMAP *parent,
                                          int x, int y, int w, int h)
{
   ALLEGRO_BITMAP_OGL* ogl_bmp;
   ALLEGRO_BITMAP_OGL* ogl_parent = (void*)parent;
   (void)d;

   ogl_bmp = al_malloc(sizeof *ogl_bmp);
   memset(ogl_bmp, 0, sizeof *ogl_bmp);

   ogl_bmp->true_w = ogl_parent->true_w;
   ogl_bmp->true_h = ogl_parent->true_h;
   ogl_bmp->texture = ogl_parent->texture;

#if !defined ALLEGRO_GP2XWIZ
   ogl_bmp->fbo_info = ogl_parent->fbo_info;
#endif

   ogl_bmp->left = x / (float)ogl_parent->true_w;
   ogl_bmp->right = (x + w) / (float)ogl_parent->true_w;
   ogl_bmp->top = (parent->h - y) / (float)ogl_parent->true_h;
   ogl_bmp->bottom = (parent->h - y - h) / (float)ogl_parent->true_h;

   ogl_bmp->is_backbuffer = ogl_parent->is_backbuffer;

   ogl_bmp->bitmap.vt = parent->vt;
   
   ogl_bmp->bitmap.flags |= _ALLEGRO_INTERNAL_OPENGL;

   return (void*)ogl_bmp;
}

/* Function: al_get_opengl_texture
 */
GLuint al_get_opengl_texture(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL))
      return 0;
   return ogl_bitmap->texture;
}

/* Function: al_remove_opengl_fbo
 */
void al_remove_opengl_fbo(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;

   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL))
      return;
   if (!ogl_bitmap->fbo_info)
      return;

   ASSERT(ogl_bitmap->fbo_info->fbo_state > FBO_INFO_UNUSED);
   ASSERT(ogl_bitmap->fbo_info->fbo != 0);
   ALLEGRO_DEBUG("Deleting FBO: %u\n", ogl_bitmap->fbo_info->fbo);

#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
   glDeleteFramebuffersEXT(1, &ogl_bitmap->fbo_info->fbo);
#elif defined ALLEGRO_IPHONE
   glDeleteFramebuffersOES(1, &ogl_bitmap->fbo_info->fbo);
#endif
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
#if !defined ALLEGRO_GP2XWIZ
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;

   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL))
      return 0;

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
#else
   (void)bitmap;
   return 0;
#endif
}

/* Function: al_get_opengl_texture_size
 */
void al_get_opengl_texture_size(ALLEGRO_BITMAP *bitmap, int *w, int *h)
{
   /* The designers of OpenGL ES 1.0 forgot to add a function to query
    * texture sizes, so this will be the only way there to get the texture
    * size. On normal OpenGL also glGetTexLevelParameter could be used.
    */
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   if (!(bitmap->flags & _ALLEGRO_INTERNAL_OPENGL)) {
      *w = 0;
      *h = 0;
      return;
   }
   *w = ogl_bitmap->true_w;
   *h = ogl_bitmap->true_h;
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
