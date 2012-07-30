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
 *      OpenGL routines common to all OpenGL drivers.
 *
 *      By Elias Pschernig and Milan Mimica.
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/transformations.h"	

#ifdef ALLEGRO_GP2XWIZ
#include "allegro5/internal/aintern_gp2xwiz.h"
#endif

#ifdef ALLEGRO_IPHONE
#include "allegro5/internal/aintern_iphone.h"
#endif

#ifdef ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif

#include <float.h>

ALLEGRO_DEBUG_CHANNEL("opengl")

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
   #define glGenFramebuffersEXT glGenFramebuffersOES
   #define glBindFramebufferEXT glBindFramebufferOES
   #define GL_FRAMEBUFFER_BINDING_EXT GL_FRAMEBUFFER_BINDING_OES
   #define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
   #define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0_OES
   #define glCheckFramebufferStatusEXT glCheckFramebufferStatusOES
   #define glFramebufferTexture2DEXT glFramebufferTexture2DOES
   #define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE_OES
   #define glDeleteFramebuffersEXT glDeleteFramebuffersOES
   #define glOrtho glOrthof
#endif


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
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
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
   if (bitmap->display->ogl_extras->is_shared == false &&
         bitmap->display != al_get_current_display()) {
      return false;
   }

   if (ogl_bitmap->is_backbuffer) {
      return false;
   }

   ASSERT(!ogl_bitmap->fbo_info);

   info = al_malloc(sizeof(ALLEGRO_FBO_INFO));
   glGenFramebuffersEXT(1, &info->fbo);
   if (info->fbo == 0) {
      al_free(info);
      return false;
   }

   old_fbo = _al_ogl_bind_framebuffer(info->fbo);

   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
      GL_TEXTURE_2D, ogl_bitmap->texture, 0);

   e = glGetError();
   if (e) {
      ALLEGRO_DEBUG("glFrameBufferTexture2DEXT failed! fbo=%d texture=%d (%s)", info->fbo, ogl_bitmap->texture, _al_gl_error_string(e));
   }

   /* You'll see this a couple times in this file: some ES 1.1 functions aren't implemented on
    * Android. This is an ugly workaround.
    */
   if (
#ifdef ALLEGRO_ANDROID
      (bitmap->display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) &&
#endif
      glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT
   ) {
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


static void setup_fbo(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_bitmap;
   GLint e;

   if (bitmap->parent)
      bitmap = bitmap->parent;
   ogl_bitmap = bitmap->extra;

#if !defined ALLEGRO_GP2XWIZ
   /* We can't return here. Target's FBO can be taken away by locking
      a lot of bitmaps consecutively.
   if (display->ogl_extras->opengl_target == bitmap) {
   }
   */

   if (!ogl_bitmap->is_backbuffer) {
      ALLEGRO_FBO_INFO *info = NULL;

      /* When a bitmap is set as target bitmap, we try to create an FBO for it.
       */
      if (ogl_bitmap->fbo_info == NULL && !(bitmap->flags & ALLEGRO_FORCE_LOCKING)) {

         if (
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
            /* FIXME This is quite a hack but I don't know how the Allegro
             * extension manager works to fix this properly (getting extensions
             * properly reported on iphone). All iOS devices support FBOs though
             * (currently.)
             */
            true
#else
            al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object ||
            al_get_opengl_extension_list()->ALLEGRO_GL_OES_framebuffer_object
#endif
         ) {
            info = ogl_find_unused_fbo(display);
            ASSERT(info->fbo_state != FBO_INFO_PERSISTENT);

            if (info->fbo_state == FBO_INFO_TRANSIENT) {
               ALLEGRO_BITMAP_EXTRA_OPENGL *extra = info->owner->extra;
               extra->fbo_info = NULL;
               ALLEGRO_DEBUG("Deleting FBO: %u\n", info->fbo);
               glDeleteFramebuffersEXT(1, &info->fbo);
               _al_ogl_reset_fbo_info(info);
            }

            glGenFramebuffersEXT(1, &info->fbo);
            e = glGetError();
            if (e) {
               ALLEGRO_DEBUG("glGenFramebuffersEXT failed");
            }
            else {
               ALLEGRO_DEBUG("Created FBO: %u\n", info->fbo);
            }
         }
      }
      else {
         info = ogl_bitmap->fbo_info;
      }

      if (info && info->fbo) {
         /* Bind to the FBO. */
#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
         ASSERT(display->ogl_extras->extension_list->ALLEGRO_GL_EXT_framebuffer_object ||
            display->ogl_extras->extension_list->ALLEGRO_GL_OES_framebuffer_object);
#endif

         if (info->fbo_state == FBO_INFO_UNUSED)
            info->fbo_state = FBO_INFO_TRANSIENT;
         info->owner = bitmap;
         info->last_use_time = al_get_time();
         ogl_bitmap->fbo_info = info;

         _al_ogl_bind_framebuffer(info->fbo);

         /* Attach the texture. */
         glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
            GL_TEXTURE_2D, ogl_bitmap->texture, 0);

         e = glGetError();
         if (e) {
            ALLEGRO_DEBUG("glFrameBufferTexture2DEXT failed! fbo=%d texture=%d (%s)", info->fbo, ogl_bitmap->texture, _al_gl_error_string(e));
         }

         /* See comment about unimplemented functions on Android above */
         if (
#ifdef ALLEGRO_ANDROID
            (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) &&
#endif
            glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT
         ) {
            /* For some reason, we cannot use the FBO with this
             * texture. So no reason to keep re-trying, output a log
             * message and switch to (extremely slow) software mode.
             */
            ALLEGRO_ERROR("Could not use FBO for bitmap with format %s.\n",
               _al_format_name(bitmap->format));
            ALLEGRO_ERROR("*** SWITCHING TO SOFTWARE MODE ***\n");
            _al_ogl_bind_framebuffer(0);
            glDeleteFramebuffersEXT(1, &info->fbo);
            _al_ogl_reset_fbo_info(info);
            ogl_bitmap->fbo_info = NULL;
         }
         else {
            bool set_projection = true;

            display->ogl_extras->opengl_target = bitmap;

            glViewport(0, 0, bitmap->w, bitmap->h);

            if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
               if (display->ogl_extras->program_object <= 0) {
                  set_projection = false;
               }
            }

            if (set_projection) {
               al_identity_transform(&display->proj_transform);
               al_orthographic_transform(&display->proj_transform,
                  0, 0, -1, bitmap->w, bitmap->h, 1);
               display->vt->set_projection(display);
            }
         }
      }
   }
   else {
      display->ogl_extras->opengl_target = bitmap;

      if (
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
         true // Hack
#else
         display->ogl_extras->extension_list->ALLEGRO_GL_EXT_framebuffer_object ||
         display->ogl_extras->extension_list->ALLEGRO_GL_OES_framebuffer_object
#endif
      ) {
         _al_ogl_bind_framebuffer(0);
      }

#ifndef ALLEGRO_IPHONE
      glViewport(0, 0, display->w, display->h);

      al_identity_transform(&display->proj_transform);
      /* We use upside down coordinates compared to OpenGL, so the bottommost
       * coordinate is display->h not 0.
       */
      al_orthographic_transform(&display->proj_transform, 0, 0, -1, display->w, display->h, 1);
#else
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      _al_iphone_setup_opengl_view(display);
#endif
      display->vt->set_projection(display);
   }
#else
   if (display->ogl_extras->opengl_target == ogl_bitmap)
      return;

   ALLEGRO_DISPLAY_GP2XWIZ_OGL *wiz_disp = (ALLEGRO_DISPLAY_GP2XWIZ_OGL *)display;
   display->ogl_extras->opengl_target = ogl_bitmap;

   if (!ogl_bitmap->is_backbuffer) {
      /* FIXME: implement */
   }
   else {
      eglMakeCurrent(wiz_disp->egl_display, wiz_disp->egl_surface, wiz_disp->egl_surface, wiz_disp->egl_context); 

      glViewport(0, 0, display->w, display->h);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      /* We use upside down coordinates compared to OpenGL, so the bottommost
       * coordinate is display->h not 0.
       */
      glOrtho(0, display->w, display->h, 0, -1, 1);
   }
#endif
}


void _al_ogl_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *target = bitmap;
   if (bitmap->parent)
      target = bitmap->parent;

   /* if either this bitmap or its parent (in the case of subbitmaps)
    * is locked then don't do anything
    */
   if (!(bitmap->locked ||
        (bitmap->parent && bitmap->parent->locked))) {
      setup_fbo(display, bitmap);
      if (display->ogl_extras->opengl_target == target) {
         _al_ogl_setup_bitmap_clipping(bitmap);
      }
   }
}


/* Function: al_set_current_opengl_context
 */
void al_set_current_opengl_context(ALLEGRO_DISPLAY *display)
{
   ASSERT(display);

   if (!(display->flags & ALLEGRO_OPENGL))
      return;

   if (display) {
      ALLEGRO_BITMAP *bmp = al_get_target_bitmap();
      if (bmp && bmp->display && bmp->display != display) {
         al_set_target_bitmap(NULL);
      }
   }

   _al_set_current_display_only(display);
}


void _al_ogl_setup_bitmap_clipping(const ALLEGRO_BITMAP *bitmap)
{
   int x_1, y_1, x_2, y_2, h;
   bool use_scissor = true;

   x_1 = bitmap->cl;
   y_1 = bitmap->ct;
   x_2 = bitmap->cr_excl;
   y_2 = bitmap->cb_excl;
   h = bitmap->h;

   /* Drawing onto the sub bitmap is handled by clipping the parent. */
   if (bitmap->parent) {
      x_1 += bitmap->xofs;
      y_1 += bitmap->yofs;
      x_2 += bitmap->xofs;
      y_2 += bitmap->yofs;
      h = bitmap->parent->h;
   }

   if (x_1 == 0 &&  y_1 == 0 && x_2 == bitmap->w && y_2 == bitmap->h) {
      if (bitmap->parent) {
         /* Can only disable scissor if the sub-bitmap covers the
          * complete parent.
          */
         if (bitmap->xofs == 0 && bitmap->yofs == 0 && bitmap->w ==
            bitmap->parent->w && bitmap->h == bitmap->parent->h) {
               use_scissor = false;
            }
      }
      else
         use_scissor = false;
   }
   if (!use_scissor) {
      glDisable(GL_SCISSOR_TEST);
   }
   else {
      glEnable(GL_SCISSOR_TEST);
       
      #ifdef ALLEGRO_IPHONE
      _al_iphone_clip(bitmap, x_1, y_1, x_2, y_2);
      #else
      /* OpenGL is upside down, so must adjust y_2 to the height. */
      glScissor(x_1, h - y_2, x_2 - x_1, y_2 - y_1);
      #endif
   }
}


ALLEGRO_BITMAP *_al_ogl_get_backbuffer(ALLEGRO_DISPLAY *d)
{
   return (ALLEGRO_BITMAP *)d->ogl_extras->backbuffer;
}


bool _al_ogl_resize_backbuffer(ALLEGRO_BITMAP *b, int w, int h)
{
   int pitch;
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra = b->extra;
         
   pitch = w * al_get_pixel_size(b->format);

   b->w = w;
   b->h = h;
   b->pitch = pitch;
   b->cl = 0;
   b->ct = 0;
   b->cr_excl = w;
   b->cb_excl = h;

   /* There is no texture associated with the backbuffer so no need to care
    * about texture size limitations. */
   extra->true_w = w;
   extra->true_h = h;

   /* This code below appears to be unneccessary on platforms other than
    * OS X.
    */
#ifdef ALLEGRO_MACOSX
   b->display->vt->set_projection(b->display);
#endif

#if !defined(ALLEGRO_IPHONE) && !defined(ALLEGRO_GP2XWIZ)
   b->memory = NULL;
#else
   /* iPhone/Wiz ports still expect the buffer to be present. */
   {
      /* FIXME: lazily manage memory */
      size_t bytes = pitch * h;
      al_free(b->memory);
      b->memory = al_calloc(1, bytes);
   }
#endif

   return true;
}


ALLEGRO_BITMAP* _al_ogl_create_backbuffer(ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_backbuffer;
   ALLEGRO_BITMAP *backbuffer;
   ALLEGRO_STATE backup;
   int format;

   ALLEGRO_DEBUG("Creating backbuffer\n");

   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);

   // FIXME: _al_deduce_color_format would work fine if the display paramerers
   // are filled in, for WIZ and IPOD
#ifdef ALLEGRO_GP2XWIZ
   format = ALLEGRO_PIXEL_FORMAT_RGB_565; /* Only support display format */
#elif defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
   if (disp->extra_settings.settings[ALLEGRO_COLOR_SIZE] == 16) {
      format = ALLEGRO_PIXEL_FORMAT_RGB_565;
   }
   else {
      format = ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE;
   }
#else
   format = _al_deduce_color_format(&disp->extra_settings);
   /* Eww. No OpenGL hardware in the world does that - let's just
    * switch to some default.
    */
   if (al_get_pixel_size(format) == 3) {
      /* Or should we use RGBA? Maybe only if not Nvidia cards? */
      format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
   }
#endif
   ALLEGRO_TRACE_CHANNEL_LEVEL("display", 1)("Deduced format %s for backbuffer.\n",
      _al_format_name(format));

   /* Now that the display backbuffer has a format, update extra_settings so
    * the user can query it back.
    */
   _al_set_color_components(format, &disp->extra_settings, ALLEGRO_REQUIRE);
   disp->backbuffer_format = format;

   ALLEGRO_DEBUG("Creating backbuffer bitmap\n");
   al_set_new_bitmap_format(format);
   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   backbuffer = _al_ogl_create_bitmap(disp, disp->w, disp->h);
   al_restore_state(&backup);

   if (!backbuffer) {
      ALLEGRO_DEBUG("Backbuffer bitmap creation failed.\n");
      return NULL;
   }

   backbuffer->w = disp->w;
   backbuffer->h = disp->h;
   backbuffer->cl = 0;
   backbuffer->ct = 0;
   backbuffer->cr_excl = disp->w;
   backbuffer->cb_excl = disp->h;

   ALLEGRO_TRACE_CHANNEL_LEVEL("display", 1)(
      "Created backbuffer bitmap (actual format: %s)\n",
      _al_format_name(backbuffer->format));

   ogl_backbuffer = backbuffer->extra;
   ogl_backbuffer->is_backbuffer = 1;
   backbuffer->display = disp;

   al_identity_transform(&disp->view_transform);

   return backbuffer;
}


void _al_ogl_destroy_backbuffer(ALLEGRO_BITMAP *b)
{
   al_destroy_bitmap(b);
}


#ifndef ALLEGRO_CFG_NO_GLES2
/* Function: al_set_opengl_program_object
 */
void al_set_opengl_program_object(ALLEGRO_DISPLAY *display, GLuint program_object)
{
   GLint handle;
   ALLEGRO_TRANSFORM t;
   
   display->ogl_extras->program_object = program_object;

   glUseProgram(program_object);
      
   handle = glGetUniformLocation(program_object, "projview_matrix");
   if (handle >= 0) {
      al_copy_transform(&t, &display->view_transform);
      al_compose_transform(&t, &display->proj_transform);
      glUniformMatrix4fv(handle, 1, false, (float *)t.m);
   }

   display->ogl_extras->pos_loc = glGetAttribLocation(program_object, "pos");
   display->ogl_extras->color_loc = glGetAttribLocation(program_object, "color");
   display->ogl_extras->texcoord_loc = glGetAttribLocation(program_object, "texcoord");
   display->ogl_extras->use_tex_loc = glGetUniformLocation(program_object, "use_tex");
   display->ogl_extras->tex_loc = glGetUniformLocation(program_object, "tex");
   display->ogl_extras->use_tex_matrix_loc = glGetUniformLocation(program_object, "use_tex_matrix");
   display->ogl_extras->tex_matrix_loc = glGetUniformLocation(program_object, "tex_matrix");
}
#endif

/* vi: set sts=3 sw=3 et: */
