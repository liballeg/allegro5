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
 *      OpenGL framebuffer objects.
 *
 *      See LICENSE.txt for copyright information.
 */

#include <float.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"

#ifdef ALLEGRO_ANDROID
   #include <android/api-level.h>
   #include "allegro5/internal/aintern_android.h"
#elif defined ALLEGRO_IPHONE
   #include "allegro5/internal/aintern_iphone.h"
#endif

#include "ogl_helpers.h"

ALLEGRO_DEBUG_CHANNEL("opengl")


/* forward declarations */
static void setup_fbo_backbuffer(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap);
static void use_fbo_for_bitmap(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap, ALLEGRO_FBO_INFO *info);


/* glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT..) not supported on some Androids.
 * We keep track of it manually.
 */
#ifdef ALLEGRO_ANDROID

static GLint _al_gl_curr_fbo = 0;

GLint _al_android_get_curr_fbo(void)
{
   return _al_gl_curr_fbo;
}

void _al_android_set_curr_fbo(GLint fbo)
{
   _al_gl_curr_fbo = fbo;
}

GLint _al_ogl_bind_framebuffer(GLint fbo)
{
   GLint old_fbo = _al_android_get_curr_fbo();
   GLint e;

   if (ANDROID_PROGRAMMABLE_PIPELINE(al_get_current_display())) {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
   }
   else {
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
   }
   e = glGetError();
   if (e) {
      ALLEGRO_DEBUG("glBindFramebufferEXT failed (%s)",
         _al_gl_error_string(e));
   }
   _al_android_set_curr_fbo(fbo);
   return old_fbo;
}

#else /* !ALLEGRO_ANDROID */

GLint _al_ogl_bind_framebuffer(GLint fbo)
{
   GLint old_fbo;
   glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_fbo);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
   return old_fbo;
}

#endif /* !ALLEGRO_ANDROID */


void _al_ogl_reset_fbo_info(ALLEGRO_FBO_INFO *info)
{
   info->fbo_state = FBO_INFO_UNUSED;
   info->fbo = 0;
   info->buffers.depth_buffer = 0;
   info->buffers.multisample_buffer = 0;
   info->buffers.dw = 0;
   info->buffers.dh = 0;
   info->buffers.mw = 0;
   info->buffers.mh = 0;
   info->owner = NULL;
   info->last_use_time = 0.0;
}


#if (!defined ALLEGRO_CFG_OPENGLES) || defined ALLEGRO_CFG_OPENGLES3
static void check_gl_error(void)
{
   GLint e = glGetError();
   if (e) {
      ALLEGRO_ERROR("OpenGL call failed! (%s)\n",
       _al_gl_error_string(e));
   }
}
#endif


static void detach_depth_buffer(ALLEGRO_FBO_INFO *info)
{
#ifndef ALLEGRO_RASPBERRYPI
   if (info->buffers.depth_buffer == 0)
      return;
   ALLEGRO_DEBUG("Deleting depth render buffer: %u\n",
            info->buffers.depth_buffer);
   glDeleteRenderbuffersEXT(1, &info->buffers.depth_buffer);
   info->buffers.depth_buffer = 0;
    info->buffers.dw = 0;
   info->buffers.dh = 0;
   info->buffers.depth = 0;
#endif
}


static void detach_multisample_buffer(ALLEGRO_FBO_INFO *info)
{
#ifndef ALLEGRO_RASPBERRYPI
   if (info->buffers.multisample_buffer == 0)
      return;
   ALLEGRO_DEBUG("Deleting multisample render buffer: %u\n",
            info->buffers.depth_buffer);
   glDeleteRenderbuffersEXT(1, &info->buffers.multisample_buffer);
   info->buffers.multisample_buffer = 0;
   info->buffers.mw = 0;
   info->buffers.mh = 0;
   info->buffers.samples = 0;
#endif
}



static void attach_depth_buffer(ALLEGRO_FBO_INFO *info)
{
#if !defined ALLEGRO_RASPBERRYPI
   GLuint rb;
   GLenum gldepth = GL_DEPTH_COMPONENT16;

   ALLEGRO_BITMAP *b = info->owner;
   int bits = al_get_bitmap_depth(b);

   if (info->buffers.depth_buffer != 0) {

      if (info->buffers.depth != bits ||
            info->buffers.dw != al_get_bitmap_width(b) ||
            info->buffers.dh != al_get_bitmap_height(b)) {
         detach_depth_buffer(info);
      }
   }

   if (!bits)
      return;

   if (info->buffers.depth_buffer == 0) {
      ALLEGRO_DISPLAY *display = _al_get_bitmap_display(info->owner);
      int w = al_get_bitmap_width(info->owner);
      int h = al_get_bitmap_height(info->owner);

#if !defined ALLEGRO_CFG_OPENGLES || defined ALLEGRO_CFG_OPENGLES3
      if (bits == 24) gldepth = GL_DEPTH_COMPONENT24;
#endif

      glGenRenderbuffersEXT(1, &rb);
      glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb);

      int samples = al_get_bitmap_samples(info->owner);

      bool extension_supported;
#ifdef ALLEGRO_CFG_OPENGLES
      (void)display;
      extension_supported = al_have_opengl_extension("EXT_multisampled_render_to_texture");
#else
      extension_supported = display->ogl_extras->extension_list->ALLEGRO_GL_EXT_framebuffer_multisample;
#endif

      if (samples == 0 || !extension_supported)
         glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, gldepth, w, h);
#if !defined ALLEGRO_CFG_OPENGLES || defined ALLEGRO_CFG_OPENGLES3
      else
         glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,
            samples, gldepth, w, h);
#else
      else {
         return;
      }
#endif

      info->buffers.depth_buffer = rb;
      info->buffers.dw = w;
      info->buffers.dh = h;
      info->buffers.depth = bits;
      GLint e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glRenderbufferStorage failed! bits=%d w=%d h=%d (%s)\n",
            bits, w, h, _al_gl_error_string(e));
      }
      else {
         ALLEGRO_DEBUG("Depth render buffer created: %u\n",
            info->buffers.depth_buffer);
      }

      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
          GL_RENDERBUFFER_EXT, rb);
      if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
         ALLEGRO_ERROR("attaching depth renderbuffer failed\n");
      }

      glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
   }
#endif
}


static void attach_multisample_buffer(ALLEGRO_FBO_INFO *info)
{
#if !defined ALLEGRO_CFG_OPENGLES || defined ALLEGRO_CFG_OPENGLES3
   ALLEGRO_BITMAP *b = info->owner;
   int samples = al_get_bitmap_samples(b);

   if (info->buffers.multisample_buffer != 0) {

      if (info->buffers.samples != samples ||
            info->buffers.mw != al_get_bitmap_width(b) ||
            info->buffers.mh != al_get_bitmap_height(b)) {
         detach_multisample_buffer(info);
      }
   }

   if (!samples)
      return;
   ALLEGRO_DISPLAY *display = _al_get_bitmap_display(info->owner);
   if (!display->ogl_extras->extension_list->ALLEGRO_GL_EXT_framebuffer_multisample)
      return;

#ifdef ALLEGRO_CFG_OPENGLES
   (void)display;
#else

   if (info->buffers.multisample_buffer == 0) {
      GLuint rb;
      GLint e;
      int w = al_get_bitmap_width(info->owner);
      int h = al_get_bitmap_height(info->owner);

      glGenRenderbuffersEXT(1, &rb);
      glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb);
      check_gl_error();

      glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,
         samples, _al_ogl_get_glformat(
            al_get_bitmap_format(info->owner), 0), w, h);
      info->buffers.multisample_buffer = rb;
      info->buffers.mw = w;
      info->buffers.mh = h;
      info->buffers.samples = samples;
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glRenderbufferStorage failed! samples=%d w=%d h=%d (%s)\n",
            samples, w, h, _al_gl_error_string(e));
      }
      else {
         ALLEGRO_DEBUG("Multisample render buffer created: %u\n",
            info->buffers.multisample_buffer);
      }

      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
         GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rb);

      if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
         ALLEGRO_ERROR("attaching multisample renderbuffer failed\n");
      }

      glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
   }
   #endif
#else
   (void)info;
#endif
}


bool _al_ogl_create_persistent_fbo(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap;
   ALLEGRO_FBO_INFO *info;
   GLint old_fbo, e;

   if (bitmap->parent)
      bitmap = bitmap->parent;
   ogl_bitmap = bitmap->extra;

   /* Don't continue if the bitmap does not belong to the current display. */
   if (_al_get_bitmap_display(bitmap)->ogl_extras->is_shared == false &&
         _al_get_bitmap_display(bitmap) != al_get_current_display()) {
      return false;
   }

   if (ogl_bitmap->is_backbuffer) {
      return false;
   }

   ASSERT(!ogl_bitmap->fbo_info);

   info = al_malloc(sizeof(ALLEGRO_FBO_INFO));
   info->owner = bitmap;
   if (ANDROID_PROGRAMMABLE_PIPELINE(al_get_current_display())) {
      glGenFramebuffers(1, &info->fbo);
   }
   else {
      glGenFramebuffersEXT(1, &info->fbo);
   }
   if (info->fbo == 0) {
      al_free(info);
      return false;
   }

   old_fbo = _al_ogl_bind_framebuffer(info->fbo);

   if (ANDROID_PROGRAMMABLE_PIPELINE(al_get_current_display())) {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
         GL_TEXTURE_2D, ogl_bitmap->texture, 0);
   }
   else {
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
         GL_TEXTURE_2D, ogl_bitmap->texture, 0);
   }

   e = glGetError();
   if (e) {
      ALLEGRO_DEBUG("glFrameBufferTexture2DEXT failed! fbo=%d texture=%d (%s)\n",
         info->fbo, ogl_bitmap->texture, _al_gl_error_string(e));
   }

   attach_depth_buffer(info);

   /* You'll see this a couple times in this file: some ES 1.1 functions aren't
    * implemented on Android. This is an ugly workaround.
    */
   if (UNLESS_ANDROID_OR_RPI(
         glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT))
   {
      ALLEGRO_ERROR("FBO incomplete.\n");
      _al_ogl_bind_framebuffer(old_fbo);
      glDeleteFramebuffersEXT(1, &info->fbo);
      al_free(info);
      return false;
   }

   _al_ogl_bind_framebuffer(old_fbo);

   info->fbo_state = FBO_INFO_PERSISTENT;
   info->last_use_time = al_get_time();
   ogl_bitmap->fbo_info = info;
   ALLEGRO_DEBUG("Persistent FBO: %u\n", info->fbo);
   return true;
}


ALLEGRO_FBO_INFO *_al_ogl_persist_fbo(ALLEGRO_DISPLAY *display,
   ALLEGRO_FBO_INFO *transient_fbo_info)
{
   ALLEGRO_OGL_EXTRAS *extras = display->ogl_extras;
   int i;
   ASSERT(transient_fbo_info->fbo_state == FBO_INFO_TRANSIENT);

   for (i = 0; i < ALLEGRO_MAX_OPENGL_FBOS; i++) {
      if (transient_fbo_info == &extras->fbos[i]) {
         ALLEGRO_FBO_INFO *new_info = al_malloc(sizeof(ALLEGRO_FBO_INFO));
         *new_info = *transient_fbo_info;
         new_info->fbo_state = FBO_INFO_PERSISTENT;
         _al_ogl_reset_fbo_info(transient_fbo_info);
         ALLEGRO_DEBUG("Persistent FBO: %u\n", new_info->fbo);
         return new_info;
      }
   }

   ALLEGRO_ERROR("Could not find FBO %u in pool\n", transient_fbo_info->fbo);
   return transient_fbo_info;
}


static ALLEGRO_FBO_INFO *ogl_find_unused_fbo(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_OGL_EXTRAS *extras = display->ogl_extras;
   double min_time = DBL_MAX;
   int min_time_index = -1;
   int i;

   for (i = 0; i < ALLEGRO_MAX_OPENGL_FBOS; i++) {
      if (extras->fbos[i].fbo_state == FBO_INFO_UNUSED)
         return &extras->fbos[i];
      if (extras->fbos[i].last_use_time < min_time) {
         min_time = extras->fbos[i].last_use_time;
         min_time_index = i;
      }
   }

   return &extras->fbos[min_time_index];
}


void _al_ogl_del_fbo(ALLEGRO_FBO_INFO *info)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra = info->owner->extra;
   extra->fbo_info = NULL;
   ALLEGRO_DEBUG("Deleting FBO: %u\n", info->fbo);
   if (ANDROID_PROGRAMMABLE_PIPELINE(al_get_current_display())) {
      glDeleteFramebuffers(1, &info->fbo);
   }
   else {
      glDeleteFramebuffersEXT(1, &info->fbo);
   }

   detach_depth_buffer(info);
   detach_multisample_buffer(info);

   info->fbo = 0;
}


static ALLEGRO_FBO_INFO *ogl_new_fbo(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_FBO_INFO *info;
   GLint e;

   info = ogl_find_unused_fbo(display);
   ASSERT(info->fbo_state != FBO_INFO_PERSISTENT);

   if (info->fbo_state == FBO_INFO_TRANSIENT) {
      _al_ogl_del_fbo(info);
      _al_ogl_reset_fbo_info(info);
   }
   else {
      /* FBO_INFO_UNUSED */
   }

   if (ANDROID_PROGRAMMABLE_PIPELINE(al_get_current_display())) {
      glGenFramebuffers(1, &info->fbo);
   }
   else {
      glGenFramebuffersEXT(1, &info->fbo);
   }
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glGenFramebuffersEXT failed\n");
      _al_ogl_reset_fbo_info(info);
      return NULL;
   }

   ALLEGRO_DEBUG("Created FBO: %u\n", info->fbo);
   return info;
}


void _al_ogl_setup_fbo(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap;

   if (bitmap->parent)
      bitmap = bitmap->parent;
   ogl_bitmap = bitmap->extra;

   /* We can't return here. Target's FBO can be taken away by locking
    * a lot of bitmaps consecutively.
    * Also affects ex_multiwin; resizing one window affects the other.
    */
   if (false && display->ogl_extras->opengl_target == bitmap)
      return;

   _al_ogl_unset_target_bitmap(display, display->ogl_extras->opengl_target);

   if (ogl_bitmap->is_backbuffer)
      setup_fbo_backbuffer(display, bitmap);
   else
      _al_ogl_setup_fbo_non_backbuffer(display, bitmap);
}


/* With the framebuffer_multisample extension, the absolutely one and
 * only way to ever access the multisample buffer is with the
 * framebuffer_blit extension. [1]
 *
 * This is what we do in this function - if there is a multisample
 * buffer, downsample it back into the texture.
 *
 * [1] https://www.opengl.org/registry/specs/EXT/framebuffer_multisample.txt
 */
void _al_ogl_finalize_fbo(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra = bitmap->extra;
   if (!extra)
      return;
   ALLEGRO_FBO_INFO *info = extra->fbo_info;
   (void)display;
   if (!info)
      return;
   if (!info->buffers.multisample_buffer)
      return;
   #ifndef ALLEGRO_CFG_OPENGLES
   int w = al_get_bitmap_width(bitmap);
   int h = al_get_bitmap_height(bitmap);

   GLuint blit_fbo;
   glGenFramebuffersEXT(1, &blit_fbo);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, blit_fbo);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
      GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, extra->texture, 0);

   glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, info->fbo);
   glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, blit_fbo);
   glBlitFramebufferEXT(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
   check_gl_error();

   glDeleteFramebuffersEXT(1, &blit_fbo);
   #else
   (void)bitmap;
   #endif
}


static void setup_fbo_backbuffer(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap)
{
   display->ogl_extras->opengl_target = bitmap;

   // The IS_OPENGLES part is a hack.
   if (IS_OPENGLES ||
      display->ogl_extras->extension_list->ALLEGRO_GL_EXT_framebuffer_object ||
      display->ogl_extras->extension_list->ALLEGRO_GL_OES_framebuffer_object)
   {
      _al_ogl_bind_framebuffer(0);
   }

#ifdef ALLEGRO_IPHONE
   _al_iphone_setup_opengl_view(display, false);
#endif
}


bool _al_ogl_setup_fbo_non_backbuffer(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   ALLEGRO_FBO_INFO *info;

   ASSERT(bitmap->parent == NULL);

   /* When a bitmap is set as target bitmap, we try to create an FBO for it. */
   info = ogl_bitmap->fbo_info;
   if (!info) {
      /* FIXME The IS_OPENGLES part is quite a hack but I don't know how the
       * Allegro extension manager works to fix this properly (getting
       * extensions properly reported on iphone). All iOS devices support
       * FBOs though (currently.)
       */
      if (IS_OPENGLES ||
         al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object ||
         al_get_opengl_extension_list()->ALLEGRO_GL_OES_framebuffer_object)
      {
         info = ogl_new_fbo(display);
      }
   }

   if (!info || info->fbo == 0) {
      return false;
   }

   use_fbo_for_bitmap(display, bitmap, info);
   return true; /* state changed */
}


static void use_fbo_for_bitmap(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap, ALLEGRO_FBO_INFO *info)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap = bitmap->extra;
   GLint e;

   if (info->fbo_state == FBO_INFO_UNUSED)
      info->fbo_state = FBO_INFO_TRANSIENT;
   info->owner = bitmap;
   info->last_use_time = al_get_time();
   ogl_bitmap->fbo_info = info;

   /* Bind to the FBO. */
   _al_ogl_bind_framebuffer(info->fbo);

   attach_multisample_buffer(info);
   attach_depth_buffer(info);

   /* If we have a multisample renderbuffer, we can only syncronize
    * it back to the texture once we stop drawing into it - i.e.
    * when the target bitmap is changed to something else.
    */
   if (!info->buffers.multisample_buffer) {

      /* Attach the texture. */
#ifdef ALLEGRO_CFG_OPENGLES
      if (ANDROID_PROGRAMMABLE_PIPELINE(al_get_current_display())) {
         if (al_get_bitmap_samples(bitmap) == 0 || !al_have_opengl_extension("EXT_multisampled_render_to_texture")) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
               GL_TEXTURE_2D, ogl_bitmap->texture, 0);
         }
#if ((!defined ALLEGRO_CFG_OPENGLES || defined ALLEGRO_CFG_OPENGLES3) && !defined ALLEGRO_IPHONE)
#if (!defined ALLEGRO_ANDROID) || (__ANDROID_API__ >= 28) /* Android: glFramebufferTexture2DMultisampleEXT exists in newer libGLESv[23].so */
         else {
            glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER,
               GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ogl_bitmap->texture,
               0, al_get_bitmap_samples(bitmap));

         }
#endif
#endif
      }
      else
#endif
      {
         glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
            GL_TEXTURE_2D, ogl_bitmap->texture, 0);
      }

      e = glGetError();
      if (e) {
         ALLEGRO_DEBUG("glFrameBufferTexture2DEXT failed! fbo=%d texture=%d (%s)\n",
            info->fbo, ogl_bitmap->texture, _al_gl_error_string(e));
      }
   }

   /* See comment about unimplemented functions on Android above */
   if (UNLESS_ANDROID_OR_RPI(
         glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT))
   {
      /* For some reason, we cannot use the FBO with this
       * texture. So no reason to keep re-trying, output a log
       * message and switch to (extremely slow) software mode.
       */
      ALLEGRO_ERROR("Could not use FBO for bitmap with format %s.\n",
         _al_pixel_format_name(al_get_bitmap_format(bitmap)));
      ALLEGRO_ERROR("*** SWITCHING TO SOFTWARE MODE ***\n");
      _al_ogl_bind_framebuffer(0);
      glDeleteFramebuffersEXT(1, &info->fbo);
      _al_ogl_reset_fbo_info(info);
      ogl_bitmap->fbo_info = NULL;
   }
   else {
      display->ogl_extras->opengl_target = bitmap;
   }
}


/* vim: set sts=3 sw=3 et: */
