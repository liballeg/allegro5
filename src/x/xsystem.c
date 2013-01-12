#ifdef DEBUG_X11
extern int _Xdebug; /* part of Xlib */
#endif

#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xcursor.h"
#include "allegro5/internal/aintern_xevents.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xkeyboard.h"
#include "allegro5/internal/aintern_xmouse.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintxglx.h"

ALLEGRO_DEBUG_CHANNEL("system")

static ALLEGRO_SYSTEM_INTERFACE *xglx_vt;

static ALLEGRO_SYSTEM *xglx_initialize(int flags)
{
   Display *x11display;
   Display *gfxdisplay;
   ALLEGRO_SYSTEM_XGLX *s;

   (void)flags;

#ifdef DEBUG_X11
   _Xdebug = 1;
#endif

   XInitThreads();

   /* Get an X11 display handle. */
   x11display = XOpenDisplay(0);
   if (x11display) {
      /* Never ask. */
      gfxdisplay = XOpenDisplay(0);
      if (!gfxdisplay) {
         ALLEGRO_ERROR("XOpenDisplay failed second time.\n");
         XCloseDisplay(x11display);
         return NULL;
      }
   }
   else {
      ALLEGRO_INFO("XOpenDisplay failed; assuming headless mode.\n");
      gfxdisplay = NULL;
   }

   _al_unix_init_time();

   s = al_calloc(1, sizeof *s);

   _al_mutex_init_recursive(&s->lock);
   _al_cond_init(&s->resized);
   s->inhibit_screensaver = false;

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_DISPLAY_XGLX *));

   s->gfxdisplay = gfxdisplay;
   s->x11display = x11display;

   s->system.vt = xglx_vt;

   if (s->x11display) {
      ALLEGRO_INFO("XGLX driver connected to X11 (%s %d).\n",
         ServerVendor(s->x11display), VendorRelease(s->x11display));
      ALLEGRO_INFO("X11 protocol version %d.%d.\n",
         ProtocolVersion(s->x11display), ProtocolRevision(s->x11display));

      /* We need to put *some* atom into the ClientMessage we send for
       * faking mouse movements with al_set_mouse_xy - so let's ask X11
       * for one here.
       */
      s->AllegroAtom = XInternAtom(x11display, "AllegroAtom", False);

      _al_thread_create(&s->thread, _al_xwin_background_thread, s);
      ALLEGRO_INFO("events thread spawned.\n");
   }

   return &s->system;
}

static void xglx_shutdown_system(void)
{
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   ALLEGRO_SYSTEM_XGLX *sx = (void *)s;

   ALLEGRO_INFO("shutting down.\n");

   if (sx->x11display) {
      /* Events thread only runs if we are connected to an X server. */
      _al_thread_join(&sx->thread);
   }

   /* Close all open displays. */
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);

   // Makes sure we wait for any commands sent to the X server when destroying the displays.
   // Should make sure we don't shutdown before modes are restored.
   if (sx->x11display) {
      XSync(sx->x11display, False);
   }
   
   _al_xsys_mmon_exit(sx);

   if (sx->x11display) {
      XCloseDisplay(sx->x11display);
      sx->x11display = None;
      ALLEGRO_DEBUG("xsys: close x11display.\n");
   }

   if (sx->gfxdisplay) {
      XCloseDisplay(sx->gfxdisplay);
      sx->gfxdisplay = None;
   }

   al_free(sx);
}

static ALLEGRO_DISPLAY_INTERFACE *xglx_get_display_driver(void)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   /* Look up the toggle_mouse_grab_key binding.  This isn't such a great place
    * to do it, but the config file is not available until after the system driver
    * is initialised.
    */
   if (system->system.config && !system->toggle_mouse_grab_keycode) {
      const char *binding = al_get_config_value(system->system.config,
         "keyboard", "toggle_mouse_grab_key");
      if (binding) {
         system->toggle_mouse_grab_keycode = _al_parse_key_binding(binding,
            &system->toggle_mouse_grab_modifiers);
         if (system->toggle_mouse_grab_keycode) {
            ALLEGRO_DEBUG("Toggle mouse grab key: '%s'\n", binding);
         }
         else {
            ALLEGRO_WARN("Cannot parse key binding '%s'\n", binding);
         }
      }
   }

   return _al_display_xglx_driver();
}

static ALLEGRO_KEYBOARD_DRIVER *xglx_get_keyboard_driver(void)
{
   return _al_xwin_keyboard_driver();
}

static ALLEGRO_MOUSE_DRIVER *xglx_get_mouse_driver(void)
{
   return _al_xwin_mouse_driver();
}

static ALLEGRO_JOYSTICK_DRIVER *xglx_get_joystick_driver(void)
{
   return _al_joystick_driver_list[0].driver;
}

static int xglx_get_num_video_adapters(void)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   return _al_xglx_get_num_video_adapters(system);
}

static bool xglx_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   return _al_xglx_get_monitor_info(system, adapter, info);
}

static bool xglx_get_cursor_position(int *ret_x, int *ret_y)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Window root = RootWindow(system->x11display, 0);
   Window child;
   int wx, wy;
   unsigned int mask;

   _al_mutex_lock(&system->lock);
   XQueryPointer(system->x11display, root, &root, &child, ret_x, ret_y,
      &wx, &wy, &mask);
   _al_mutex_unlock(&system->lock);
   return true;
}

static bool xglx_inhibit_screensaver(bool inhibit)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   system->inhibit_screensaver = inhibit;
   return true;
}

static int xglx_get_num_display_modes(void)
{
   int adapter = al_get_new_display_adapter();
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   return _al_xglx_get_num_display_modes(s, adapter);
}

static ALLEGRO_DISPLAY_MODE *xglx_get_display_mode(int mode, ALLEGRO_DISPLAY_MODE *dm)
{
   int adapter = al_get_new_display_adapter();
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   return _al_xglx_get_display_mode(s, adapter, mode, dm);
}

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_xglx_driver(void)
{
   if (xglx_vt)
      return xglx_vt;

   xglx_vt = al_calloc(1, sizeof *xglx_vt);

   xglx_vt->initialize = xglx_initialize;
   xglx_vt->get_display_driver = xglx_get_display_driver;
   xglx_vt->get_keyboard_driver = xglx_get_keyboard_driver;
   xglx_vt->get_mouse_driver = xglx_get_mouse_driver;
   xglx_vt->get_joystick_driver = xglx_get_joystick_driver;
   xglx_vt->get_num_display_modes = xglx_get_num_display_modes;
   xglx_vt->get_display_mode = xglx_get_display_mode;
   xglx_vt->shutdown_system = xglx_shutdown_system;
   xglx_vt->get_num_video_adapters = xglx_get_num_video_adapters;
   xglx_vt->get_monitor_info = xglx_get_monitor_info;
   xglx_vt->create_mouse_cursor = _al_xwin_create_mouse_cursor;
   xglx_vt->destroy_mouse_cursor = _al_xwin_destroy_mouse_cursor;
   xglx_vt->get_cursor_position = xglx_get_cursor_position;
   xglx_vt->grab_mouse = _al_xwin_grab_mouse;
   xglx_vt->ungrab_mouse = _al_xwin_ungrab_mouse;
   xglx_vt->get_path = _al_unix_get_path;
   xglx_vt->inhibit_screensaver = xglx_inhibit_screensaver;

   return xglx_vt;
}


/* vim: set sts=3 sw=3 et: */
