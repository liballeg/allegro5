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
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_system.h"
#include <math.h>

#if defined ALLEGRO_GP2XWIZ
#include "allegro5/internal/aintern_gp2xwiz.h"
#elif defined ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif

#ifdef ALLEGRO_IPHONE
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
#define GL_FRAMEBUFFER_BINDING_EXT GL_FRAMEBUFFER_BINDING_OES
#define glGenerateMipmapEXT glGenerateMipmapOES
#define glBindFramebufferEXT glBindFramebufferOES
#define glDeleteFramebuffersEXT glDeleteFramebuffersOES
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
 * 
 * Desktop OpenGL 3.0+ has no GL_LUMINANCE, so we have to adjust depending on
 * the runtime version.
 */
#define get_glformat(f, c) _al_ogl_get_glformat((f), (c))
int _al_ogl_get_glformat(int format, int component)
{
   #if !defined(ALLEGRO_GP2XWIZ) && !defined(ALLEGRO_IPHONE) && !defined(ALLEGRO_ANDROID)
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

   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
#if !defined ALLEGRO_GP2XWIZ
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
#elif defined ALLEGRO_GP2XWIZ
      /* FIXME: make this work somehow on Wiz */
      return;
#endif
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
                    _al_format_name(bitmap->format));
      }
   }
   glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glBindTexture for texture %d failed (%s).\n",
         ogl_bitmap->texture, _al_gl_error_string(e));
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
#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
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
#else
   {
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
#endif

   if (e) {
      ALLEGRO_ERROR("glTexImage2D for format %s, size %dx%d failed (%s)\n",
         _al_format_name(bitmap->format),
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



/* "Old" implementation of ogl_lock_region and ogl_unlock_region.
 * The refactored version is in ogl_lock.c.
 * The support for mobile platforms should be migrated there,
 * then this version can be deleted.
 */
#if defined(ALLEGRO_IPHONE) || defined(ALLEGRO_ANDROID) || defined(ALLEGRO_GP2XWIZ)

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
static ALLEGRO_LOCKED_REGION *ogl_lock_region_old(ALLEGRO_BITMAP *bitmap,
   int x, int y, int w, int h, int format, int flags)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   int pixel_size;
   int pixel_alignment;
   int pitch;
   ALLEGRO_DISPLAY *disp;
   ALLEGRO_DISPLAY *old_disp = NULL;
   ALLEGRO_BITMAP *old_target = NULL;
   bool need_to_restore_target = false;
   bool need_to_restore_display = false;
   GLint gl_y = bitmap->h - y - h;
   GLenum e;

   if (format == ALLEGRO_PIXEL_FORMAT_ANY)
      format = bitmap->format;

   disp = al_get_current_display();
   format = _al_get_real_pixel_format(disp, format);

   pixel_size = al_get_pixel_size(format);
   pixel_alignment = ogl_pixel_alignment(pixel_size);

   if (!disp || (bitmap->display->ogl_extras->is_shared == false &&
         bitmap->display != disp)) {
      need_to_restore_display = true;
      old_disp = disp;
      _al_set_current_display_only(bitmap->display);
   }

   /* Set up the pixel store state.  We will need to match it when unlocking.
    * There may be other pixel store state we should be setting.
    * See also pitfalls 7 & 8 from:
    * http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
    */

   glPixelStorei(GL_PACK_ALIGNMENT, pixel_alignment);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glPixelStorei(GL_PACK_ALIGNMENT, %d) failed (%s).\n",
         pixel_alignment, _al_gl_error_string(e));
   }

   if (ogl_bitmap->is_backbuffer) {
      ALLEGRO_DEBUG("Locking backbuffer\n");

      pitch = ogl_pitch(w, pixel_size);
      ogl_bitmap->lock_buffer = al_malloc(pitch * h);

      if (!(flags & ALLEGRO_LOCK_WRITEONLY)) {
         glReadPixels(x, gl_y, w, h,
            get_glformat(format, 2),
            get_glformat(format, 1),
            ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glReadPixels for format %s failed (%s).\n",
               _al_format_name(format), _al_gl_error_string(e));
         }
      }
      bitmap->locked_region.data = ogl_bitmap->lock_buffer +
         pitch * (h - 1);
   }
#if !defined ALLEGRO_GP2XWIZ
   else {
      if (flags & ALLEGRO_LOCK_WRITEONLY) {
         ALLEGRO_DEBUG("Locking non-backbuffer WRITEONLY\n");

         pitch = w * pixel_size;

         ogl_bitmap->lock_buffer = al_malloc(pitch * h);
         bitmap->locked_region.data = ogl_bitmap->lock_buffer +
            pitch * (h - 1);
      }
      else {
         ALLEGRO_DEBUG("Locking non-backbuffer %s\n", flags & ALLEGRO_LOCK_READONLY ? "READONLY" : "READWRITE");

         /* Create an FBO if there isn't one. */
         if (!ogl_bitmap->fbo_info) {
            old_target = al_get_target_bitmap();
            need_to_restore_target = true;
            bitmap->locked = false; // FIXME: hack :(
            if (al_is_bitmap_drawing_held())
               al_hold_bitmap_drawing(false);

            al_set_target_bitmap(bitmap); // This creates the fbo
            bitmap->locked = true;
         }
         
         if (ogl_bitmap->fbo_info) {
            GLint old_fbo;
            old_fbo = _al_ogl_bind_framebuffer(ogl_bitmap->fbo_info->fbo);

            e = glGetError();
            if (e) {
               ALLEGRO_ERROR("glBindFramebufferEXT failed (%s).\n",
                  _al_gl_error_string(e));
            }

            /* Some Android devices only want to read POT chunks with glReadPixels.
             * This adds yet more overhead, but AFAICT it fails any other way.
             * Test device was gen 1 Galaxy Tab. Also read 16x16 minimum.
             */
#if defined ALLEGRO_ANDROID
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            int start_h = h;
            w = pot(w);
            while (w < 16) w = pot(w+1);
            h = pot(h);
            while (h < 16) h = pot(h+1);
#endif

            pitch = ogl_pitch(w, pixel_size);
            /* Allocate a buffer big enough for both purposes. This requires more
             * memory to be held for the period of the lock, but overall less
             * memory is needed to complete the lock.
             */
            ogl_bitmap->lock_buffer = al_malloc(_ALLEGRO_MAX(pitch * h, ogl_pitch(w, 4) * h));


            /* NOTE: GLES (1.1?) can only read 4 byte pixels, we have to convert */
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
            glReadPixels(x, gl_y, w, h,
               GL_RGBA, GL_UNSIGNED_BYTE,
               ogl_bitmap->lock_buffer);
#else
            glReadPixels(x, gl_y, w, h, 
               get_glformat(format, 2),
               get_glformat(format, 1),
               ogl_bitmap->lock_buffer);
#endif

            e = glGetError();
            if (e) {
               ALLEGRO_ERROR("glReadPixels for format %s failed (%s).\n",
                  _al_format_name(format), _al_gl_error_string(e));
            }

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
            /* That's right, we convert in-place 8-) (safe as long as dst size <= src size, which it always is) */
            _al_convert_bitmap_data(
               ogl_bitmap->lock_buffer,
               ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
               ogl_pitch(w, 4),
               ogl_bitmap->lock_buffer,
               format,
               pitch,
               0, 0, 0, 0,
               w, h);
#endif

            bitmap->locked_region.data = ogl_bitmap->lock_buffer +
#ifdef ALLEGRO_ANDROID // See comments above about POT chunks
               pitch * (start_h - 1);
#else
               pitch * (h - 1);
#endif
      
            _al_ogl_bind_framebuffer(old_fbo);
         }
         else {
#if !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
            /* No FBO - fallback to reading the entire texture */
            pitch = ogl_pitch(ogl_bitmap->true_w, pixel_size);
            ogl_bitmap->lock_buffer = al_malloc(pitch * ogl_bitmap->true_h);

            glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
            glGetTexImage(GL_TEXTURE_2D, 0, get_glformat(format, 2),
               get_glformat(format, 1), ogl_bitmap->lock_buffer);
            e = glGetError();
            if (e) {
               ALLEGRO_ERROR("glGetTexImage for format %s failed (%s).\n",
                  _al_format_name(format), _al_gl_error_string(e));
            }

            bitmap->locked_region.data = ogl_bitmap->lock_buffer +
               pitch * (gl_y + h - 1) + pixel_size * x;
#else
            /* Unreachable on Android. */
            abort();
#endif /* !ALLEGRO_ANDROID */
         }
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

   bitmap->locked_region.format = format;
   bitmap->locked_region.pitch = -pitch;
   bitmap->locked_region.pixel_size = pixel_size;

   if (need_to_restore_target)
      al_set_target_bitmap(old_target);
   
   if (need_to_restore_display) {
      _al_set_current_display_only(old_disp);
   }
   
   return &bitmap->locked_region;
}

/* Synchronizes the texture back to the (possibly modified) bitmap data.
 */
static void ogl_unlock_region_old(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   const int lock_format = bitmap->locked_region.format;
   ALLEGRO_DISPLAY *disp;
   ALLEGRO_DISPLAY *old_disp = NULL;
   GLenum e;
   int w = bitmap->lock_w;
   int h = bitmap->lock_h;
   GLint gl_y = bitmap->h - bitmap->lock_y - h;
   int orig_format;
   int orig_pixel_size;
   int pixel_alignment;
   bool biased_alpha = false;
   (void)e;

   if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY) {
      ALLEGRO_DEBUG("Unlocking non-backbuffer READONLY\n");
      goto Done;
   }

   disp = al_get_current_display();
   orig_format = _al_get_real_pixel_format(disp, bitmap->format);

   if (!disp || (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != disp)) {
      old_disp = disp;
      _al_set_current_display_only(bitmap->display);
   }

   if (!disp->ogl_extras->ogl_info.is_intel_hd_graphics_3000 &&
         exactly_15bpp(lock_format)) {
      /* OpenGL does not support 15-bpp internal format without an alpha,
       * so when storing such data we must ensure the alpha bit is set.
       */
      #if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
      glPixelTransferf(GL_ALPHA_BIAS, 1.0);
      biased_alpha = true;
      #endif
   }

   orig_pixel_size = al_get_pixel_size(orig_format);

#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
   if (ogl_bitmap->is_backbuffer) {
      bool popmatrix = false;
      ALLEGRO_TRANSFORM tmp;
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
         al_copy_transform(&tmp, &disp->proj_transform);
         al_identity_transform(&disp->proj_transform);
         disp->vt->set_projection(disp);
         glRasterPos2f(bitmap->lock_x,
            bitmap->lock_y + h - 1e-4f);
         popmatrix = true;
      }
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      glDrawPixels(bitmap->lock_w, h,
         get_glformat(lock_format, 2),
         get_glformat(lock_format, 1),
         ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glDrawPixels for format %s failed (%s).\n",
            _al_format_name(lock_format), _al_gl_error_string(e));
      }
      if (popmatrix) {
         al_copy_transform(&disp->proj_transform, &tmp);
         disp->vt->set_projection(disp);
      }
   }
#else /* ALLEGRO_GP2XWIZ or ALLEGRO_IPHONE or ALLEGRO_ANDROID */
   if (ogl_bitmap->is_backbuffer) {
      ALLEGRO_DEBUG("Unlocking backbuffer\n");
      GLuint tmp_tex;
      glGenTextures(1, &tmp_tex);
      glBindTexture(GL_TEXTURE_2D, tmp_tex);
      glTexImage2D(GL_TEXTURE_2D, 0, get_glformat(lock_format, 0), bitmap->lock_w, h,
                   0, get_glformat(lock_format, 2), get_glformat(lock_format, 1),
                   ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         int printf(const char *, ...);
         printf("glTexImage2D failed: %d\n", e);
      }
      glDrawTexiOES(bitmap->lock_x, bitmap->lock_y, 0, bitmap->lock_w, h);
      e = glGetError();
      if (e) {
         int printf(const char *, ...);
         printf("glDrawTexiOES failed: %d\n", e);
      }
      glDeleteTextures(1, &tmp_tex);
   }
#endif
   else {
      GLint fbo;
#ifdef ALLEGRO_ANDROID
      fbo = _al_android_get_curr_fbo();
#else
      glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fbo);
#endif
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#ifdef ALLEGRO_ANDROID
      _al_android_set_curr_fbo(0);
#endif

      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);

#ifdef ALLEGRO_ANDROID
      if (!(bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY)) {
         w = pot(w);
      }
#endif
         
      if (!ogl_bitmap->fbo_info ||
            (bitmap->locked_region.format != orig_format)) {

         int dst_pitch = w * orig_pixel_size;
         unsigned char *tmpbuf = al_malloc(dst_pitch * h);

         ALLEGRO_DEBUG("Unlocking non-backbuffer with conversion\n");
         
         _al_convert_bitmap_data(
            ogl_bitmap->lock_buffer,
            bitmap->locked_region.format,
            -bitmap->locked_region.pitch,
            tmpbuf,
            orig_format,
            dst_pitch,
            0, 0, 0, 0,
            w, h);
	    
         pixel_alignment = ogl_pixel_alignment(orig_pixel_size);
         glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_alignment);

         glTexSubImage2D(GL_TEXTURE_2D, 0,
            bitmap->lock_x, gl_y,
            w, h,
            get_glformat(orig_format, 2),
            get_glformat(orig_format, 1),
            tmpbuf);
         al_free(tmpbuf);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glTexSubImage2D for format %d failed (%s).\n",
               lock_format, _al_gl_error_string(e));
         }
      }
      else {
         ALLEGRO_DEBUG("Unlocking non-backbuffer without conversion\n");

         pixel_alignment = ogl_pixel_alignment(orig_pixel_size);
         glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_alignment);

         glTexSubImage2D(GL_TEXTURE_2D, 0, bitmap->lock_x, gl_y,
            w, h,
            get_glformat(lock_format, 2),
            get_glformat(lock_format, 1),
            ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            GLint tex_internalformat;
            (void)tex_internalformat;
            ALLEGRO_ERROR("glTexSubImage2D for format %s failed (%s).\n",
               _al_format_name(lock_format), _al_gl_error_string(e));
#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
               GL_TEXTURE_INTERNAL_FORMAT, &tex_internalformat);

            ALLEGRO_DEBUG("x/y/w/h: %d/%d/%d/%d, internal format: %d\n",
               bitmap->lock_x, gl_y, w, h,
               tex_internalformat);
#endif
         }
      }

      if (bitmap->flags & ALLEGRO_MIPMAP) {
         /* If using FBOs, we need to regenerate mipmaps explicitly now. */
         if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object) {
            glGenerateMipmapEXT(GL_TEXTURE_2D);
            e = glGetError();
            if (e) {
               ALLEGRO_ERROR("glGenerateMipmapEXT for texture %d failed (%s).\n",
                  ogl_bitmap->texture, _al_gl_error_string(e));
            }
         }
      }
      
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
#ifdef ALLEGRO_ANDROID
      _al_android_set_curr_fbo(fbo);
#endif
   }

   if (biased_alpha) {
#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
      glPixelTransferi(GL_ALPHA_BIAS, 0);
#endif
   }

   if (old_disp) {
      _al_set_current_display_only(old_disp);
   }

Done:

   al_free(ogl_bitmap->lock_buffer);
   ogl_bitmap->lock_buffer = NULL;
}

#endif /* old locking implementation - only for mobile now */



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
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
   }

   if (old_disp) {
      _al_set_current_display_only(old_disp);
   }
   
   al_free(ogl_bitmap);
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
#if defined(ALLEGRO_ANDROID) || defined(ALLEGRO_IPHONE) || defined(ALLEGRO_GP2XWIZ)
   glbmp_vt.lock_region = ogl_lock_region_old;
   glbmp_vt.unlock_region = ogl_unlock_region_old;
#else
   glbmp_vt.lock_region = _al_ogl_lock_region_new;
   glbmp_vt.unlock_region = _al_ogl_unlock_region_new;
#endif

   return &glbmp_vt;
}



ALLEGRO_BITMAP *_al_ogl_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra;
   int format = al_get_new_bitmap_format();
   const int flags = al_get_new_bitmap_flags();
   int true_w;
   int true_h;
   int pitch;
   (void)d;

/* Android included because some devices require POT FBOs */
#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
   if (d->extra_settings.settings[ALLEGRO_SUPPORT_NPOT_BITMAP]) {
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

/* FBOs are 16x16 minimum on iPhone, this is a workaround. (Some Androids too) */
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
   if (true_w < 16) true_w = 16;
   if (true_h < 16) true_h = 16;
#endif

/* glReadPixels requires 32 byte aligned rows */
#if defined ALLEGRO_ANDROID
   int mod = true_w % 32;
   if (mod != 0) {
      true_w += 32 - mod;
   }
#endif

#if !defined ALLEGRO_GP2XWIZ
   format = _al_get_real_pixel_format(d, format);
#else
   if (format != ALLEGRO_PIXEL_FORMAT_RGB_565 && format != ALLEGRO_PIXEL_FORMAT_RGBA_4444)
      format = ALLEGRO_PIXEL_FORMAT_RGBA_4444;
#endif

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
 * if it's unsessesary
 * 'ptr' should be tightly packed or NULL if no texture data
 * upload is desired.
 */
void _al_ogl_upload_bitmap_memory(ALLEGRO_BITMAP *bitmap, int format, void *ptr)
{
   int w = bitmap->w;
   int h = bitmap->h;
   int pixsize = al_get_pixel_size(format);
   int y;
   ALLEGRO_STATE state;
   ALLEGRO_BITMAP *tmp;
   ALLEGRO_LOCKED_REGION *lr;
   uint8_t *dst;
   uint8_t *src;

   ASSERT(ptr);
  
   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_format(format);
   al_set_new_bitmap_flags(bitmap->flags);
   tmp = al_create_bitmap(w, h);
   ASSERT(tmp);
   al_restore_state(&state);

   lr = al_lock_bitmap(tmp, format, ALLEGRO_LOCK_WRITEONLY);

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
void al_get_opengl_texture_size(ALLEGRO_BITMAP *bitmap, int *w, int *h)
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
      return;
   }

   ogl_bitmap = bitmap->extra;
   *w = ogl_bitmap->true_w;
   *h = ogl_bitmap->true_h;
}

/* Function: al_get_opengl_texture_position
 */
void al_get_opengl_texture_position(ALLEGRO_BITMAP *bitmap, int *u, int *v)
{
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
   }
}

/* vim: set sts=3 sw=3 et: */
