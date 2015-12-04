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
   info->owner = NULL;
   info->last_use_time = 0.0;
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
   info->owner = bitmap;
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


static ALLEGRO_FBO_INFO *ogl_new_fbo(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_FBO_INFO *info;
   GLint e;

   info = ogl_find_unused_fbo(display);
   ASSERT(info->fbo_state != FBO_INFO_PERSISTENT);

   if (info->fbo_state == FBO_INFO_TRANSIENT) {
      ALLEGRO_BITMAP_EXTRA_OPENGL *extra = info->owner->extra;
      extra->fbo_info = NULL;
      ALLEGRO_DEBUG("Deleting FBO: %u\n", info->fbo);
      if (ANDROID_PROGRAMMABLE_PIPELINE(al_get_current_display())) {
         glDeleteFramebuffers(1, &info->fbo);
      }
      else {
         glDeleteFramebuffersEXT(1, &info->fbo);
      }
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

   if (ogl_bitmap->is_backbuffer)
      setup_fbo_backbuffer(display, bitmap);
   else
      _al_ogl_setup_fbo_non_backbuffer(display, bitmap);
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

   /* Attach the texture. */
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
