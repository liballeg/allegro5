#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_wleglconfig.h"
#include "allegro5/internal/aintern_wlsystem.h"

ALLEGRO_DEBUG_CHANNEL("wleglconfig")


/* Read an EGL config into an ALLEGRO_EXTRA_DISPLAY_SETTINGS.  Returns false
 * if the config cannot render desktop OpenGL to a window surface.
 */
static bool read_config(ALLEGRO_SYSTEM_WAYLAND *system, EGLConfig config,
                        ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   EGLint renderable_type, surface_type, color_buffer_type;

   /* Hardware-accelerated OpenGL. */
   eds->settings[ALLEGRO_RENDER_METHOD] = 2;

   if (eglGetConfigAttrib(system->egl_display, config, EGL_RENDERABLE_TYPE,
         &renderable_type) == EGL_FALSE
    || eglGetConfigAttrib(system->egl_display, config, EGL_SURFACE_TYPE,
         &surface_type) == EGL_FALSE
    || eglGetConfigAttrib(system->egl_display, config, EGL_COLOR_BUFFER_TYPE,
         &color_buffer_type) == EGL_FALSE) {
      ALLEGRO_DEBUG("Failed to read EGL config attributes.\n");
      return false;
   }

   if (!(renderable_type & EGL_OPENGL_BIT)) {
      ALLEGRO_DEBUG("Config does not support desktop OpenGL.\n");
      return false;
   }

   if (!(surface_type & EGL_WINDOW_BIT)) {
      ALLEGRO_DEBUG("Config cannot render to a window.\n");
      return false;
   }

   if (color_buffer_type != EGL_RGB_BUFFER) {
      ALLEGRO_DEBUG("Config is not an RGB buffer.\n");
      return false;
   }

   eglGetConfigAttrib(system->egl_display, config, EGL_RED_SIZE,
      &eds->settings[ALLEGRO_RED_SIZE]);
   eglGetConfigAttrib(system->egl_display, config, EGL_GREEN_SIZE,
      &eds->settings[ALLEGRO_GREEN_SIZE]);
   eglGetConfigAttrib(system->egl_display, config, EGL_BLUE_SIZE,
      &eds->settings[ALLEGRO_BLUE_SIZE]);
   eglGetConfigAttrib(system->egl_display, config, EGL_ALPHA_SIZE,
      &eds->settings[ALLEGRO_ALPHA_SIZE]);
   eglGetConfigAttrib(system->egl_display, config, EGL_BUFFER_SIZE,
      &eds->settings[ALLEGRO_COLOR_SIZE]);
   eglGetConfigAttrib(system->egl_display, config, EGL_DEPTH_SIZE,
      &eds->settings[ALLEGRO_DEPTH_SIZE]);
   eglGetConfigAttrib(system->egl_display, config, EGL_STENCIL_SIZE,
      &eds->settings[ALLEGRO_STENCIL_SIZE]);
   eglGetConfigAttrib(system->egl_display, config, EGL_SAMPLE_BUFFERS,
      &eds->settings[ALLEGRO_SAMPLE_BUFFERS]);
   eglGetConfigAttrib(system->egl_display, config, EGL_SAMPLES,
      &eds->settings[ALLEGRO_SAMPLES]);

   /* Wayland is always double buffered and vsync is controlled by the
    * compositor; EGL has no accumulation, aux or stereo buffers here. */
   eds->settings[ALLEGRO_SINGLE_BUFFER] = false;
   eds->settings[ALLEGRO_SWAP_METHOD]   = 2;
   eds->settings[ALLEGRO_VSYNC]         = 2;
   eds->settings[ALLEGRO_STEREO]        = 0;
   eds->settings[ALLEGRO_AUX_BUFFERS]   = 0;
   eds->settings[ALLEGRO_ACC_RED_SIZE]   = 0;
   eds->settings[ALLEGRO_ACC_GREEN_SIZE] = 0;
   eds->settings[ALLEGRO_ACC_BLUE_SIZE]  = 0;
   eds->settings[ALLEGRO_ACC_ALPHA_SIZE] = 0;
   eds->settings[ALLEGRO_FLOAT_COLOR]    = 0;
   eds->settings[ALLEGRO_FLOAT_DEPTH]    = 0;

   /* EGL configs don't expose channel bit offsets the way GLX visuals do.
    * The offsets are however fixed by the GL framebuffer byte order used
    * by glReadPixels(): for 8-8-8-8 buffers this comes out as ABGR when
    * read as little-endian integers.  These are what _al_deduce_color_format
    * uses to pick the backbuffer pixel format. */
   if (eds->settings[ALLEGRO_RED_SIZE] == 8
    && eds->settings[ALLEGRO_GREEN_SIZE] == 8
    && eds->settings[ALLEGRO_BLUE_SIZE] == 8) {
      eds->settings[ALLEGRO_RED_SHIFT]   = 0;
      eds->settings[ALLEGRO_GREEN_SHIFT] = 8;
      eds->settings[ALLEGRO_BLUE_SHIFT]  = 16;
      if (eds->settings[ALLEGRO_ALPHA_SIZE] == 8)
         eds->settings[ALLEGRO_ALPHA_SHIFT] = 24;
   }
   else if (eds->settings[ALLEGRO_RED_SIZE] == 5
         && eds->settings[ALLEGRO_GREEN_SIZE] == 6
         && eds->settings[ALLEGRO_BLUE_SIZE] == 5) {
      eds->settings[ALLEGRO_RED_SHIFT]   = 11;
      eds->settings[ALLEGRO_GREEN_SHIFT] = 5;
      eds->settings[ALLEGRO_BLUE_SHIFT]  = 0;
   }

   /* Like the Android EGL backend, we always claim to be a compatible
    * display rather than trying to deduce a pixel format from
    * information EGL simply doesn't provide. */
   eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;

   return true;
}


/* Internal function: _al_wlegl_config_select_visual
 * Picks a suitable EGL config for the display and copies the resulting
 * display settings into display->extra_settings.
 */
void _al_wlegl_config_select_visual(ALLEGRO_DISPLAY_WAYLAND *d)
{
   ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref = _al_get_new_display_settings();
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds;
   EGLConfig *configs;
   EGLint config_count = 0;
   EGLint i, j;

   if (eglGetConfigs(system->egl_display, NULL, 0, &config_count) == EGL_FALSE
         || config_count == 0) {
      ALLEGRO_ERROR("Failed to get any EGL configs.\n");
      return;
   }

   configs = al_malloc(config_count * sizeof(EGLConfig));
   eglGetConfigs(system->egl_display, configs, config_count, &config_count);

   eds = al_calloc(config_count, sizeof(*eds));

   ALLEGRO_INFO("%i EGL configs.\n", config_count);

   for (i = j = 0; i < config_count; i++) {
      eds[j] = al_calloc(1, sizeof(*eds[j]));
      if (!read_config(system, configs[i], eds[j])) {
         al_free(eds[j]);
         continue;
      }

      eds[j]->score = _al_score_display_settings(eds[j], ref);
      if (eds[j]->score == -1) {
         al_free(eds[j]);
         continue;
      }

      eds[j]->index = j;
      eds[j]->info = al_malloc(sizeof(EGLConfig));
      memcpy(eds[j]->info, &configs[i], sizeof(EGLConfig));
      j++;
   }

   ALLEGRO_INFO("%i usable EGL configs.\n", j);

   if (j > 0) {
      ALLEGRO_EXTRA_DISPLAY_SETTINGS *best = NULL;
      int i;

      qsort(eds, j, sizeof(*eds), _al_display_settings_sorter);

      /* Several configs can tie for the top score (eg. 8-8-8-8 vs
       * 10-10-10-2).  Prefer a config whose pixel format Allegro can
       * deduce from the channel shifts (such as 8-8-8-8), so the
       * backbuffer format resolves to something concrete instead of ANY.
       * Fall back to the best-scoring config if none qualifies. */
      for (i = 0; i < j; i++) {
         if (_al_deduce_color_format(eds[i]) != ALLEGRO_PIXEL_FORMAT_ANY) {
            best = eds[i];
            break;
         }
      }
      if (!best)
         best = eds[0];

      d->egl_config = *(EGLConfig *)best->info;
      memcpy(&display->extra_settings, best,
         sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));

      ALLEGRO_INFO("Chose EGL config %p.\n", (void *)d->egl_config);
   }
   else {
      ALLEGRO_ERROR("No usable EGL configs found.\n");
   }

   for (i = 0; i < j; i++) {
      if (eds[i]) {
         al_free(eds[i]->info);
         al_free(eds[i]);
      }
   }
   al_free(eds);
   al_free(configs);
}


/* Internal function: _al_wlegl_config_create_context
 * Creates the Wayland window and EGL context+surface for the display and
 * makes the context current for the current thread.  On return the context
 * is ready for OpenGL calls.
 */
bool _al_wlegl_config_create_context(ALLEGRO_DISPLAY_WAYLAND *d)
{
   ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;
   EGLContext existing_ctx = EGL_NO_CONTEXT;
   EGLint attribs[16];
   int n = 0;
   int major, minor;

   if (d->surface == NULL || d->egl_config == (EGLConfig)0) {
      ALLEGRO_ERROR("No Wayland surface or EGL config for display.\n");
      return false;
   }

   /* Share GPU resources with any previously created display, as GLX does. */
   if (_al_vector_size(&system->system.displays) > 1) {
      ALLEGRO_DISPLAY_WAYLAND **existing_dpy;
      existing_dpy = _al_vector_ref_front(&system->system.displays);
      if (*existing_dpy != d)
         existing_ctx = (*existing_dpy)->egl_context;
   }

   if (display->flags & ALLEGRO_OPENGL_ES_PROFILE) {
      if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
         ALLEGRO_ERROR("eglBindAPI(EGL_OPENGL_ES_API) failed.\n");
         return false;
      }
   }
   else {
      if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
         ALLEGRO_ERROR("eglBindAPI(EGL_OPENGL_API) failed.\n");
         return false;
      }
   }

   major = al_get_new_display_option(ALLEGRO_OPENGL_MAJOR_VERSION, 0);
   minor = al_get_new_display_option(ALLEGRO_OPENGL_MINOR_VERSION, 0);

   if ((display->flags & ALLEGRO_OPENGL_3_0) || major != 0
         || (display->flags & ALLEGRO_OPENGL_CORE_PROFILE)) {
      /* Request a specific (3.0+) context version. */
      if (major == 0)
         major = 3;

      attribs[n++] = EGL_CONTEXT_MAJOR_VERSION;
      attribs[n++] = major;
      attribs[n++] = EGL_CONTEXT_MINOR_VERSION;
      attribs[n++] = minor;

      if (display->flags & ALLEGRO_OPENGL_CORE_PROFILE) {
         /* Core profile requires at least OpenGL 3.2. */
         if (major == 3 && minor < 2)
            minor = 2;
         attribs[n++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
         attribs[n++] = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;
      }

      if (display->flags & ALLEGRO_OPENGL_FORWARD_COMPATIBLE) {
         attribs[n++] = EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE;
         attribs[n++] = EGL_TRUE;
      }
   }
   attribs[n++] = EGL_NONE;

   d->egl_context = eglCreateContext(system->egl_display, d->egl_config,
      existing_ctx, attribs);
   if (d->egl_context == EGL_NO_CONTEXT) {
      ALLEGRO_ERROR("eglCreateContext failed: %#x\n", eglGetError());
      return false;
   }

   /* Create the wl_egl_window and the EGL surface backed by it.  The size
    * sent to the compositor is that requested at display creation; it may be
    * overridden later by xdg configure events. */
   d->egl_window = wl_egl_window_create(d->surface, display->w, display->h);
   if (!d->egl_window) {
      ALLEGRO_ERROR("wl_egl_window_create failed.\n");
      eglDestroyContext(system->egl_display, d->egl_context);
      d->egl_context = EGL_NO_CONTEXT;
      return false;
   }

   d->egl_surface = eglCreateWindowSurface(system->egl_display, d->egl_config,
      (EGLNativeWindowType)d->egl_window, NULL);
   if (d->egl_surface == EGL_NO_SURFACE) {
      ALLEGRO_ERROR("eglCreateWindowSurface failed: %#x\n", eglGetError());
      wl_egl_window_destroy(d->egl_window);
      d->egl_window = NULL;
      eglDestroyContext(system->egl_display, d->egl_context);
      d->egl_context = EGL_NO_CONTEXT;
      return false;
   }

   if (eglMakeCurrent(system->egl_display, d->egl_surface, d->egl_surface,
         d->egl_context) == EGL_FALSE) {
      ALLEGRO_ERROR("eglMakeCurrent failed: %#x\n", eglGetError());
      return false;
   }

   display->ogl_extras->is_shared = (existing_ctx != EGL_NO_CONTEXT);

   ALLEGRO_DEBUG("Got EGL context.\n");
   return true;
}