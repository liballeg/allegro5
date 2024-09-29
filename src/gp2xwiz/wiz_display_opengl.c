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
 *      GP2X Wiz OpenGL display driver
 *
 *      By Trent Gamblin.
 *
 */

#include "allegro5/internal/aintern_gp2xwiz.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"

A5O_DEBUG_CHANNEL("display")

static A5O_DISPLAY_INTERFACE *gp2xwiz_vt;

static bool set_gfx_mode = false;

/* Helper to set up GL state as we want it. */
static void setup_gl(A5O_DISPLAY *d)
{
   A5O_OGL_EXTRAS *ogl = d->ogl_extras;

   glViewport(0, 0, d->w, d->h);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, d->w, d->h, 0, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   
   if (!ogl->backbuffer)
      ogl->backbuffer = _al_ogl_create_backbuffer(d);
}


/* Create a new X11 display, which maps directly to a GLX window. */
static A5O_DISPLAY *gp2xwiz_create_display_ogl(int w, int h)
{
   (void)w;
   (void)h;

   /* Only one display allowed at a time */
   if (set_gfx_mode)
      return NULL;

   A5O_DISPLAY_GP2XWIZ_OGL *d = al_calloc(1, sizeof *d);
   A5O_DISPLAY *display = (void*)d;
   A5O_OGL_EXTRAS *ogl = al_calloc(1, sizeof *ogl);
   EGLint numConfigs;

   display->ogl_extras = ogl;

   A5O_SYSTEM_GP2XWIZ *system = (void *)al_get_system_driver();

   display->w = 320;
   display->h = 240;
   display->vt = _al_display_gp2xwiz_opengl_driver();
   display->refresh_rate = 60;
   display->flags = al_get_new_display_flags();

   // FIXME: default? Is this the right place to set this?
   display->flags |= A5O_OPENGL;
#ifdef A5O_CFG_OPENGLES2
   display->flags |= A5O_PROGRAMMABLE_PIPELINE;
#endif
#ifdef A5O_CFG_OPENGLES
   display->flags |= A5O_OPENGL_ES_PROFILE;
#endif
   display->flags |= A5O_FULLSCREEN;

   /* Add ourself to the list of displays. */
   A5O_DISPLAY_GP2XWIZ_OGL **add;
   add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   EGLint attrib_list[] =
   {
     EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
     EGL_RED_SIZE, 5,
     EGL_GREEN_SIZE, 6,
     EGL_BLUE_SIZE, 5,
     EGL_DEPTH_SIZE, 16,
     EGL_NONE
   };

   EGLint majorVersion, minorVersion;

   nanoGL_Init();

   d->hNativeWnd = OS_CreateWindow();                      

   d->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

   eglInitialize(d->egl_display, &majorVersion, &minorVersion);

   A5O_DEBUG("EGL Version: %d.%d\n", majorVersion, minorVersion);

   eglChooseConfig(d->egl_display, attrib_list, &d->egl_config, 1, &numConfigs);

   d->egl_surface = eglCreateWindowSurface(d->egl_display, d->egl_config, d->hNativeWnd, NULL);

   d->egl_context = eglCreateContext(d->egl_display, d->egl_config, EGL_NO_CONTEXT, NULL);

   eglMakeCurrent(d->egl_display, d->egl_surface, d->egl_surface, d->egl_context);    


   //eglSwapInterval(d->egl_display, EGL_MAX_SWAP_INTERVAL);

   A5O_DEBUG("GP2X Wiz window created.\n");

   // FIXME:
   A5O_DEBUG("Calling _al_ogl_manage_extensions\n");
   _al_ogl_manage_extensions(display);
   A5O_DEBUG("Calling _al_ogl_set_extensions\n");
   _al_ogl_set_extensions(ogl->extension_api);

   // FIXME
   // We don't have this extra_settings stuff set up right
   //if (display->extra_settings.settings[A5O_COMPATIBLE_DISPLAY])
      setup_gl(display);

   set_gfx_mode = true;

   A5O_DEBUG("Display created successfully\n");

   return display;
}


static void gp2xwiz_destroy_display_ogl(A5O_DISPLAY *d)
{
   A5O_SYSTEM_GP2XWIZ *s = (void *)al_get_system_driver();
   A5O_DISPLAY_GP2XWIZ_OGL *wiz_disp = (void *)d;

   while (d->bitmaps._size > 0) {
      A5O_BITMAP **bptr = _al_vector_ref_back(&d->bitmaps);
      A5O_BITMAP *b = *bptr;
      _al_convert_to_memory_bitmap(b);
   }

   _al_ogl_unmanage_extensions(d);

   _al_vector_find_and_delete(&s->system.displays, &d);

   eglMakeCurrent(wiz_disp->egl_display, 0, 0, wiz_disp->egl_context);

   eglDestroySurface(wiz_disp->egl_display, wiz_disp->egl_surface);
   eglDestroyContext(wiz_disp->egl_display, wiz_disp->egl_context);
   eglTerminate(wiz_disp->egl_display);
   al_free(wiz_disp->hNativeWnd);
   nanoGL_Destroy();

   _al_vector_free(&d->bitmaps);
   _al_event_source_free(&d->es);

   al_free(d->ogl_extras);
   al_free(d->vertex_cache);
   al_free(d);

   set_gfx_mode = false;
}


static bool gp2xwiz_set_current_display_ogl(A5O_DISPLAY *d)
{
   (void)d;
   return true;
}


static void gp2xwiz_flip_display_ogl(A5O_DISPLAY *d)
{
   A5O_DISPLAY_GP2XWIZ_OGL *wiz_disp = (A5O_DISPLAY_GP2XWIZ_OGL *)d;
   eglSwapBuffers(wiz_disp->egl_display, wiz_disp->egl_surface);
}

static void gp2xwiz_update_display_region_ogl(A5O_DISPLAY *d, int x, int y,
   int w, int h)
{
   (void)x;
   (void)y;
   (void)w;
   (void)h;
   gp2xwiz_flip_display_ogl(d);
}

static bool gp2xwiz_acknowledge_resize_ogl(A5O_DISPLAY *d)
{
   (void)d;
   return false;
}


static bool gp2xwiz_resize_display_ogl(A5O_DISPLAY *d, int w, int h)
{
   (void)d;
   (void)w;
   (void)h;
   return false;
}


static bool gp2xwiz_is_compatible_bitmap_ogl(A5O_DISPLAY *display,
   A5O_BITMAP *bitmap)
{
   (void)display;
   (void)bitmap;
   return true;
}


static void gp2xwiz_get_window_position_ogl(A5O_DISPLAY *display, int *x, int *y)
{
   (void)display;
   *x = 0;
   *y = 0;
}


static bool gp2xwiz_wait_for_vsync_ogl(A5O_DISPLAY *display)
{
   (void)display;
   return false;
}

/* Obtain a reference to this driver. */
A5O_DISPLAY_INTERFACE *_al_display_gp2xwiz_opengl_driver(void)
{
   if (gp2xwiz_vt)
      return gp2xwiz_vt;

   gp2xwiz_vt = al_calloc(1, sizeof *gp2xwiz_vt);

   gp2xwiz_vt->create_display = gp2xwiz_create_display_ogl;
   gp2xwiz_vt->destroy_display = gp2xwiz_destroy_display_ogl;
   gp2xwiz_vt->set_current_display = gp2xwiz_set_current_display_ogl;
   gp2xwiz_vt->flip_display = gp2xwiz_flip_display_ogl;
   gp2xwiz_vt->update_display_region = gp2xwiz_update_display_region_ogl;
   gp2xwiz_vt->acknowledge_resize = gp2xwiz_acknowledge_resize_ogl;
   gp2xwiz_vt->create_bitmap = _al_ogl_create_bitmap;
   gp2xwiz_vt->get_backbuffer = _al_ogl_get_backbuffer;
   gp2xwiz_vt->set_target_bitmap = _al_ogl_set_target_bitmap;
   gp2xwiz_vt->is_compatible_bitmap = gp2xwiz_is_compatible_bitmap_ogl;
   gp2xwiz_vt->resize_display = gp2xwiz_resize_display_ogl;
   gp2xwiz_vt->set_icons = NULL;
   gp2xwiz_vt->set_window_title = NULL;
   gp2xwiz_vt->set_window_position = NULL;
   gp2xwiz_vt->get_window_position = gp2xwiz_get_window_position_ogl;
   gp2xwiz_vt->set_window_constraints = NULL;
   gp2xwiz_vt->get_window_constraints = NULL;
   gp2xwiz_vt->set_display_flag = NULL;
   gp2xwiz_vt->wait_for_vsync = gp2xwiz_wait_for_vsync_ogl;

   _al_ogl_add_drawing_functions(gp2xwiz_vt);

   return gp2xwiz_vt;
}

/* vi: set sts=3 sw=3 et: */
