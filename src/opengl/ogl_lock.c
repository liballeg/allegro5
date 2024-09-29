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
 *      OpenGL bitmap locking.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"

/*
 * This is an attempt to refactor ogl_lock_region and ogl_unlock_region.
 * To begin with it only supports desktop OpenGL.  Support for mobile platforms
 * should be migrated here gradually, but PLEASE try not to do it by inserting
 * #ifdefs everywhere.  Combined with huge functions, that made the previous
 * version very hard to follow and prone to break.
 */
#if !defined(A5O_CFG_OPENGLES)

A5O_DEBUG_CHANNEL("opengl")

#define get_glformat(f, c) _al_ogl_get_glformat((f), (c))


/*
 * Helpers - duplicates code in ogl_bitmap.c for now
 */

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
   return pixel_format == A5O_PIXEL_FORMAT_RGB_555
      || pixel_format == A5O_PIXEL_FORMAT_BGR_555;
}



/*
 * Locking
 */

static bool ogl_lock_region_backbuffer(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format, int flags);
static bool ogl_lock_region_nonbb_writeonly(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format);
static bool ogl_lock_region_nonbb_readwrite(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format, bool* restore_fbo);
static bool ogl_lock_region_nonbb_readwrite_fbo(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format);
static bool ogl_lock_region_nonbb_readwrite_nonfbo(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format);


A5O_LOCKED_REGION *_al_ogl_lock_region_new(A5O_BITMAP *bitmap,
   int x, int y, int w, int h, int format, int flags)
{
   A5O_BITMAP_EXTRA_OPENGL * const ogl_bitmap = bitmap->extra;
   const GLint gl_y = bitmap->h - y - h;
   A5O_DISPLAY *disp;
   A5O_DISPLAY *old_disp = NULL;
   A5O_BITMAP *old_target = al_get_target_bitmap();
   GLenum e;
   bool ok;
   bool restore_fbo = false;
   bool reset_alignment = false;

   if (format == A5O_PIXEL_FORMAT_ANY) {
      /* Never pick compressed formats with ANY, as it interacts weirdly with
       * existing code (e.g. al_get_pixel_size() etc) */
      int bitmap_format = al_get_bitmap_format(bitmap);
      if (_al_pixel_format_is_compressed(bitmap_format)) {
         // XXX Get a good format from the driver?
         format = A5O_PIXEL_FORMAT_ABGR_8888_LE;
      }
      else {
         format = bitmap_format;
      }
   }

   disp = al_get_current_display();
   format = _al_get_real_pixel_format(disp, format);

   /* Change OpenGL context if necessary. */
   if (!disp ||
      (_al_get_bitmap_display(bitmap)->ogl_extras->is_shared == false &&
       _al_get_bitmap_display(bitmap) != disp))
   {
      old_disp = disp;
      _al_set_current_display_only(_al_get_bitmap_display(bitmap));
   }

   ok = true;

   /* Set up the pixel store state.  We will need to match it when unlocking.
    * There may be other pixel store state we should be setting.
    * See also pitfalls 7 & 8 from:
    * http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
    */
   int previous_alignment;
   glGetIntegerv(GL_PACK_ALIGNMENT, &previous_alignment);
   {
      const int pixel_size = al_get_pixel_size(format);
      const int pixel_alignment = ogl_pixel_alignment(pixel_size);
      if (previous_alignment != pixel_alignment) {
         reset_alignment = true;
         glPixelStorei(GL_PACK_ALIGNMENT, pixel_alignment);
         e = glGetError();
         if (e) {
            A5O_ERROR("glPixelStorei(GL_PACK_ALIGNMENT, %d) failed (%s).\n",
               pixel_alignment, _al_gl_error_string(e));
            ok = false;
         }
      }
   }

   if (ok) {
      if (ogl_bitmap->is_backbuffer) {
         A5O_DEBUG("Locking backbuffer\n");
         ok = ogl_lock_region_backbuffer(bitmap, ogl_bitmap,
            x, gl_y, w, h, format, flags);
      }
      else if (flags & A5O_LOCK_WRITEONLY) {
         A5O_DEBUG("Locking non-backbuffer WRITEONLY\n");
         ok = ogl_lock_region_nonbb_writeonly(bitmap, ogl_bitmap,
            x, gl_y, w, h, format);
      }
      else {
         A5O_DEBUG("Locking non-backbuffer READWRITE\n");
         ok = ogl_lock_region_nonbb_readwrite(bitmap, ogl_bitmap,
            x, gl_y, w, h, format, &restore_fbo);
      }
   }

   if (reset_alignment) {
      glPixelStorei(GL_PACK_ALIGNMENT, previous_alignment);
   }

   /* Restore state after switching FBO. */
   if (restore_fbo) {
      if (!old_target) {
         /* Old target was NULL; release the context. */
         _al_set_current_display_only(NULL);
      }
      else if (!_al_get_bitmap_display(old_target)) {
         /* Old target was memory bitmap; leave the current display alone. */
      }
      else if (old_target != bitmap) {
         /* Old target was another OpenGL bitmap. */
         _al_ogl_setup_fbo(_al_get_bitmap_display(old_target), old_target);
      }
   }

   ASSERT(al_get_target_bitmap() == old_target);

   if (old_disp != NULL) {
      _al_set_current_display_only(old_disp);
   }

   if (ok) {
      return &bitmap->locked_region;
   }

   A5O_ERROR("Failed to lock region\n");
   ASSERT(ogl_bitmap->lock_buffer == NULL);
   return NULL;
}


static bool ogl_lock_region_backbuffer(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format, int flags)
{
   const int pixel_size = al_get_pixel_size(format);
   const int pitch = ogl_pitch(w, pixel_size);
   GLenum e;

   ogl_bitmap->lock_buffer = al_malloc(pitch * h);
   if (ogl_bitmap->lock_buffer == NULL) {
      return false;
   }

   if (!(flags & A5O_LOCK_WRITEONLY)) {
      glReadPixels(x, gl_y, w, h,
         get_glformat(format, 2),
         get_glformat(format, 1),
         ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         A5O_ERROR("glReadPixels for format %s failed (%s).\n",
            _al_pixel_format_name(format), _al_gl_error_string(e));
         al_free(ogl_bitmap->lock_buffer);
         ogl_bitmap->lock_buffer = NULL;
         return false;
      }
   }

   bitmap->locked_region.data = ogl_bitmap->lock_buffer + pitch * (h - 1);
   bitmap->locked_region.format = format;
   bitmap->locked_region.pitch = -pitch;
   bitmap->locked_region.pixel_size = pixel_size;
   return true;
}


static bool ogl_lock_region_nonbb_writeonly(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format)
{
   const int pixel_size = al_get_pixel_size(format);
   const int pitch = ogl_pitch(w, pixel_size);
   (void) x;
   (void) gl_y;

   ogl_bitmap->lock_buffer = al_malloc(pitch * h);
   if (ogl_bitmap->lock_buffer == NULL) {
      return false;
   }

   bitmap->locked_region.data = ogl_bitmap->lock_buffer + pitch * (h - 1);
   bitmap->locked_region.format = format;
   bitmap->locked_region.pitch = -pitch;
   bitmap->locked_region.pixel_size = pixel_size;
   return true;
}


static bool ogl_lock_region_nonbb_readwrite(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format, bool* restore_fbo)
{
   bool ok;

   ASSERT(bitmap->parent == NULL);
   ASSERT(bitmap->locked == false);
   ASSERT(_al_get_bitmap_display(bitmap) == al_get_current_display());

   /* Try to create an FBO if there isn't one. */
   *restore_fbo =
      _al_ogl_setup_fbo_non_backbuffer(_al_get_bitmap_display(bitmap), bitmap);

   if (ogl_bitmap->fbo_info) {
      A5O_DEBUG("Locking non-backbuffer READWRITE with fbo\n");
      ok = ogl_lock_region_nonbb_readwrite_fbo(bitmap, ogl_bitmap,
         x, gl_y, w, h, format);
   }
   else {
      A5O_DEBUG("Locking non-backbuffer READWRITE no fbo\n");
      ok = ogl_lock_region_nonbb_readwrite_nonfbo(bitmap, ogl_bitmap,
         x, gl_y, w, h, format);
   }

   return ok;
}


static bool ogl_lock_region_nonbb_readwrite_fbo(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format)
{
   const int pixel_size = al_get_pixel_size(format);
   const int pitch = ogl_pitch(w, pixel_size);
   GLint old_fbo;
   GLenum e;
   bool ok;

   glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_fbo);
   e = glGetError();
   if (e) {
      A5O_ERROR("glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT) failed (%s).\n",
         _al_gl_error_string(e));
      return false;
   }

   ok = true;

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ogl_bitmap->fbo_info->fbo);
   e = glGetError();
   if (e) {
      A5O_ERROR("glBindFramebufferEXT failed (%s).\n",
         _al_gl_error_string(e));
      ok = false;
   }

   if (ok) {
      ogl_bitmap->lock_buffer = al_malloc(pitch * h);
      if (ogl_bitmap->lock_buffer == NULL) {
         ok = false;
      }
   }

   if (ok) {
      glReadPixels(x, gl_y, w, h,
         get_glformat(format, 2),
         get_glformat(format, 1),
         ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         A5O_ERROR("glReadPixels for format %s failed (%s).\n",
            _al_pixel_format_name(format), _al_gl_error_string(e));
      }
   }

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fbo);

   if (ok) {
      bitmap->locked_region.data = ogl_bitmap->lock_buffer + pitch * (h - 1);
      bitmap->locked_region.format = format;
      bitmap->locked_region.pitch = -pitch;
      bitmap->locked_region.pixel_size = pixel_size;
      return true;
   }

   al_free(ogl_bitmap->lock_buffer);
   ogl_bitmap->lock_buffer = NULL;
   return ok;
}


static bool ogl_lock_region_nonbb_readwrite_nonfbo(
   A5O_BITMAP *bitmap, A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap,
   int x, int gl_y, int w, int h, int format)
{
   /* No FBO - fallback to reading the entire texture */
   const int pixel_size = al_get_pixel_size(format);
   const int pitch = ogl_pitch(ogl_bitmap->true_w, pixel_size);
   GLenum e;
   bool ok;
   (void) w;

   ogl_bitmap->lock_buffer = al_malloc(pitch * ogl_bitmap->true_h);
   if (ogl_bitmap->lock_buffer == NULL) {
      return false;
   }

   ok = true;

   glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   glGetTexImage(GL_TEXTURE_2D, 0,
      get_glformat(format, 2),
      get_glformat(format, 1),
      ogl_bitmap->lock_buffer);

   e = glGetError();
   if (e) {
      A5O_ERROR("glGetTexImage for format %s failed (%s).\n",
         _al_pixel_format_name(format), _al_gl_error_string(e));
      al_free(ogl_bitmap->lock_buffer);
      ogl_bitmap->lock_buffer = NULL;
      ok = false;
   }

   if (ok) {
      bitmap->locked_region.data = ogl_bitmap->lock_buffer +
         pitch * (gl_y + h - 1) + pixel_size * x;
      bitmap->locked_region.format = format;
      bitmap->locked_region.pitch = -pitch;
      bitmap->locked_region.pixel_size = pixel_size;
   }

   return ok;
}



/*
 * Unlocking
 */

static void ogl_unlock_region_non_readonly(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap);
static void ogl_unlock_region_backbuffer(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y);
static void ogl_unlock_region_nonbb_fbo(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y, int orig_format);
static void ogl_unlock_region_nonbb_fbo_writeonly(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y, int orig_format);
static void ogl_unlock_region_nonbb_fbo_readwrite(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y);
static void ogl_unlock_region_nonbb_nonfbo(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y);


void _al_ogl_unlock_region_new(A5O_BITMAP *bitmap)
{
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;

   if (bitmap->lock_flags & A5O_LOCK_READONLY) {
      A5O_DEBUG("Unlocking non-backbuffer READONLY\n");
   }
   else {
      ogl_unlock_region_non_readonly(bitmap, ogl_bitmap);
   }

   al_free(ogl_bitmap->lock_buffer);
   ogl_bitmap->lock_buffer = NULL;
}


static void ogl_unlock_region_non_readonly(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap)
{
   const int lock_format = bitmap->locked_region.format;
   const int gl_y = bitmap->h - bitmap->lock_y - bitmap->lock_h;
   A5O_DISPLAY *old_disp = NULL;
   A5O_DISPLAY *disp;
   int orig_format;
   bool biased_alpha = false;
   bool reset_alignment = false;
   GLenum e;

   disp = al_get_current_display();
   orig_format = _al_get_real_pixel_format(disp, _al_get_bitmap_memory_format(bitmap));

   /* Change OpenGL context if necessary. */
   if (!disp ||
      (_al_get_bitmap_display(bitmap)->ogl_extras->is_shared == false &&
       _al_get_bitmap_display(bitmap) != disp))
   {
      old_disp = disp;
      _al_set_current_display_only(_al_get_bitmap_display(bitmap));
   }

   /* Keep this in sync with ogl_lock_region. */
   int previous_alignment;
   glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_alignment);
   {
      const int lock_pixel_size = al_get_pixel_size(lock_format);
      const int pixel_alignment = ogl_pixel_alignment(lock_pixel_size);
      if (pixel_alignment != previous_alignment) {
         reset_alignment = true;
         glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_alignment);
         e = glGetError();
         if (e) {
            A5O_ERROR("glPixelStorei(GL_UNPACK_ALIGNMENT, %d) failed (%s).\n",
               pixel_alignment, _al_gl_error_string(e));
         }
      }
   }
   if (exactly_15bpp(lock_format)) {
      /* OpenGL does not support 15-bpp internal format without an alpha,
       * so when storing such data we must ensure the alpha bit is set.
       */
      glPixelTransferi(GL_ALPHA_BIAS, 1);
      biased_alpha = true;
   }

   if (ogl_bitmap->is_backbuffer) {
      A5O_DEBUG("Unlocking backbuffer\n");
      ogl_unlock_region_backbuffer(bitmap, ogl_bitmap, gl_y);
   }
   else {
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      if (ogl_bitmap->fbo_info) {
         A5O_DEBUG("Unlocking non-backbuffer (FBO)\n");
         ogl_unlock_region_nonbb_fbo(bitmap, ogl_bitmap, gl_y, orig_format);
      }
      else {
         A5O_DEBUG("Unlocking non-backbuffer (non-FBO)\n");
         ogl_unlock_region_nonbb_nonfbo(bitmap, ogl_bitmap, gl_y);
      }

      /* If using FBOs, we need to regenerate mipmaps explicitly now. */
      /* XXX why don't we check ogl_bitmap->fbo_info? */
      if ((al_get_bitmap_flags(bitmap) & A5O_MIPMAP) &&
         al_get_opengl_extension_list()->A5O_GL_EXT_framebuffer_object)
      {
         glGenerateMipmapEXT(GL_TEXTURE_2D);
         e = glGetError();
         if (e) {
            A5O_ERROR("glGenerateMipmapEXT for texture %d failed (%s).\n",
               ogl_bitmap->texture, _al_gl_error_string(e));
         }
      }
   }

   if (biased_alpha) {
      glPixelTransferi(GL_ALPHA_BIAS, 0);
   }
   if (reset_alignment) {
      glPixelStorei(GL_UNPACK_ALIGNMENT, previous_alignment);
   }

   if (old_disp) {
      _al_set_current_display_only(old_disp);
   }
}


static void ogl_unlock_region_backbuffer(A5O_BITMAP *bitmap,
      A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y)
{
   const int lock_format = bitmap->locked_region.format;
   bool popmatrix = false;
   GLenum e;
   GLint program = 0;
   A5O_DISPLAY *display = al_get_current_display();

   if (display->flags & A5O_PROGRAMMABLE_PIPELINE) {
      // FIXME: This is a hack where we temporarily disable the active shader.
      // It will only work on Desktop OpenGL in non-strict mode where we even
      // can switch back to the fixed pipeline. The correct way would be to not
      // use any OpenGL 2 functions (like glDrawPixels). Probably we will want
      // separate OpenGL <= 2 (including OpenGL ES 1) and OpenGL >= 3 (including
      // OpenGL ES >= 2) drivers at some point.
      glGetIntegerv(GL_CURRENT_PROGRAM, &program);
      glUseProgram(0);
   }

   /* glWindowPos2i may not be available. */
   if (al_get_opengl_version() >= _A5O_OPENGL_VERSION_1_4) {
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
      glRasterPos2f(bitmap->lock_x, bitmap->lock_y + bitmap->lock_h - 1e-4f);
      popmatrix = true;
   }

   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glDrawPixels(bitmap->lock_w, bitmap->lock_h,
      get_glformat(lock_format, 2),
      get_glformat(lock_format, 1),
      ogl_bitmap->lock_buffer);
   e = glGetError();
   if (e) {
      A5O_ERROR("glDrawPixels for format %s failed (%s).\n",
         _al_pixel_format_name(lock_format), _al_gl_error_string(e));
   }

   if (popmatrix) {
      glPopMatrix();
   }

   if (program != 0) {
      glUseProgram(program);
   }
}


static void ogl_unlock_region_nonbb_fbo(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y, int orig_format)
{
   if (bitmap->lock_flags & A5O_LOCK_WRITEONLY) {
      A5O_DEBUG("Unlocking non-backbuffer FBO WRITEONLY\n");
      ogl_unlock_region_nonbb_fbo_writeonly(bitmap, ogl_bitmap, gl_y,
         orig_format);
   }
   else {
      A5O_DEBUG("Unlocking non-backbuffer FBO READWRITE\n");
      ogl_unlock_region_nonbb_fbo_readwrite(bitmap, ogl_bitmap, gl_y);
   }
}


static void ogl_unlock_region_nonbb_fbo_writeonly(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y, int orig_format)
{
   const int lock_format = bitmap->locked_region.format;
   const int orig_pixel_size = al_get_pixel_size(orig_format);
   const int dst_pitch = bitmap->lock_w * orig_pixel_size;
   unsigned char * const tmpbuf = al_malloc(dst_pitch * bitmap->lock_h);
   GLenum e;

   _al_convert_bitmap_data(
      ogl_bitmap->lock_buffer,
      bitmap->locked_region.format,
      -bitmap->locked_region.pitch,
      tmpbuf,
      orig_format,
      dst_pitch,
      0, 0, 0, 0,
      bitmap->lock_w, bitmap->lock_h);

   glTexSubImage2D(GL_TEXTURE_2D, 0,
      bitmap->lock_x, gl_y,
      bitmap->lock_w, bitmap->lock_h,
      get_glformat(orig_format, 2),
      get_glformat(orig_format, 1),
      tmpbuf);
   e = glGetError();
   if (e) {
      A5O_ERROR("glTexSubImage2D for format %d failed (%s).\n",
         lock_format, _al_gl_error_string(e));
   }

   al_free(tmpbuf);
}


static void ogl_unlock_region_nonbb_fbo_readwrite(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y)
{
   const int lock_format = bitmap->locked_region.format;
   GLenum e;
   GLint tex_internalformat;

   glTexSubImage2D(GL_TEXTURE_2D, 0, bitmap->lock_x, gl_y,
      bitmap->lock_w, bitmap->lock_h,
      get_glformat(lock_format, 2),
      get_glformat(lock_format, 1),
      ogl_bitmap->lock_buffer);

   e = glGetError();
   if (e) {
      A5O_ERROR("glTexSubImage2D for format %s failed (%s).\n",
         _al_pixel_format_name(lock_format), _al_gl_error_string(e));
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
         GL_TEXTURE_INTERNAL_FORMAT, &tex_internalformat);
      A5O_DEBUG("x/y/w/h: %d/%d/%d/%d, internal format: %d\n",
         bitmap->lock_x, gl_y, bitmap->lock_w, bitmap->lock_h,
         tex_internalformat);
   }
}


static void ogl_unlock_region_nonbb_nonfbo(A5O_BITMAP *bitmap,
   A5O_BITMAP_EXTRA_OPENGL *ogl_bitmap, int gl_y)
{
   const int lock_format = bitmap->locked_region.format;
   unsigned char *start_ptr;
   GLenum e;

   if (bitmap->lock_flags & A5O_LOCK_WRITEONLY) {
      A5O_DEBUG("Unlocking non-backbuffer non-FBO WRITEONLY\n");
      start_ptr = ogl_bitmap->lock_buffer;
   }
   else {
      A5O_DEBUG("Unlocking non-backbuffer non-FBO READWRITE\n");
      glPixelStorei(GL_UNPACK_ROW_LENGTH, ogl_bitmap->true_w);
      start_ptr = (unsigned char *)bitmap->lock_data
            + (bitmap->lock_h - 1) * bitmap->locked_region.pitch;
   }

   glTexSubImage2D(GL_TEXTURE_2D, 0,
      bitmap->lock_x, gl_y,
      bitmap->lock_w, bitmap->lock_h,
      get_glformat(lock_format, 2),
      get_glformat(lock_format, 1),
      start_ptr);

   e = glGetError();
   if (e) {
      A5O_ERROR("glTexSubImage2D for format %s failed (%s).\n",
         _al_pixel_format_name(lock_format), _al_gl_error_string(e));
   }
}


#endif

/* vim: set sts=3 sw=3 et: */
