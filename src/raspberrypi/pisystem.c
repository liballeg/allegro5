#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/platform/aintraspberrypi.h"
#include "allegro5/internal/aintern_raspberrypi.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintlnx.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xevents.h"
#include "allegro5/internal/aintern_xmouse.h"

#include <bcm_host.h>

#include <signal.h>

ALLEGRO_DEBUG_CHANNEL("system")

static ALLEGRO_SYSTEM_INTERFACE *pi_vt;

static ALLEGRO_SYSTEM *pi_initialize(int flags)
{
   (void)flags;

   ALLEGRO_SYSTEM_RASPBERRYPI *s;

   bcm_host_init();

   s = al_calloc(1, sizeof *s);

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_DISPLAY_RASPBERRYPI *));

   _al_unix_init_time();

   if (getenv("DISPLAY")) {
      _al_mutex_init_recursive(&s->lock);
      s->x11display = XOpenDisplay(0);
      _al_thread_create(&s->thread, _al_xwin_background_thread, s);
      ALLEGRO_INFO("events thread spawned.\n");
      /* We need to put *some* atom into the ClientMessage we send for
       * faking mouse movements with al_set_mouse_xy - so let's ask X11
       * for one here.
       */
      s->AllegroAtom = XInternAtom(s->x11display, "AllegroAtom", False);
   }

   s->inhibit_screensaver = false;

   s->system.vt = pi_vt;

   return &s->system;
}

static void pi_shutdown_system(void)
{
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   ALLEGRO_SYSTEM_RASPBERRYPI *spi = (void *)s;

   ALLEGRO_INFO("shutting down.\n");

   /* Close all open displays. */
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);

   if (getenv("DISPLAY")) {
      _al_thread_join(&spi->thread);
      XCloseDisplay(spi->x11display);
   }

   bcm_host_deinit();

   raise(SIGINT);

   al_free(spi);
}

static ALLEGRO_KEYBOARD_DRIVER *pi_get_keyboard_driver(void)
{
   if (getenv("DISPLAY")) {
      return _al_xwin_keyboard_driver();
   }
   return _al_linux_keyboard_driver_list[0].driver;
}

static ALLEGRO_MOUSE_DRIVER *pi_get_mouse_driver(void)
{
   if (getenv("DISPLAY")) {
      return _al_xwin_mouse_driver();
   }
   return _al_linux_mouse_driver_list[0].driver;
}

static ALLEGRO_JOYSTICK_DRIVER *pi_get_joystick_driver(void)
{
   return _al_joystick_driver_list[0].driver;
}

static int pi_get_num_video_adapters(void)
{
   return 1;
}

static bool pi_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   (void)adapter;
   int dx, dy, w, h;
   _al_raspberrypi_get_screen_info(&dx, &dy, &w, &h);
   info->x1 = 0;
   info->y1 = 0;
   info->x2 = w;
   info->y2 = h;
   return true;
}

static bool pi_get_cursor_position(int *ret_x, int *ret_y)
{
   // FIXME:
   (void)ret_x;
   (void)ret_y;
   return false;
}

static bool pi_inhibit_screensaver(bool inhibit)
{
   ALLEGRO_SYSTEM_RASPBERRYPI *system = (void *)al_get_system_driver();

   system->inhibit_screensaver = inhibit;
   return true;
}

static int pi_get_num_display_modes(void)
{
   return 1;
}

static ALLEGRO_DISPLAY_MODE *pi_get_display_mode(int mode, ALLEGRO_DISPLAY_MODE *dm)
{
   (void)mode;
   int dx, dy, w, h;
   _al_raspberrypi_get_screen_info(&dx, &dy, &w, &h);
   dm->width = w;
   dm->height = h;
   dm->format = 0; // FIXME
   dm->refresh_rate = 60;
   return dm;
}

static ALLEGRO_MOUSE_CURSOR *pi_create_mouse_cursor(ALLEGRO_BITMAP *bmp, int focus_x_ignored, int focus_y_ignored)
{
   (void)focus_x_ignored;
   (void)focus_y_ignored;

   ALLEGRO_STATE state;
   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS | ALLEGRO_STATE_TARGET_BITMAP);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   ALLEGRO_BITMAP *cursor_bmp = al_clone_bitmap(bmp);
   ALLEGRO_MOUSE_CURSOR_RASPBERRYPI *cursor = al_malloc(sizeof(ALLEGRO_MOUSE_CURSOR_RASPBERRYPI));
   cursor->bitmap = cursor_bmp;
   al_restore_state(&state);
   return (ALLEGRO_MOUSE_CURSOR *)cursor;
}

static void pi_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_MOUSE_CURSOR_RASPBERRYPI *pi_cursor = (void *)cursor;
   al_destroy_bitmap(pi_cursor->bitmap);
   al_free(pi_cursor);
}

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_raspberrypi_driver(void)
{
   if (pi_vt)
      return pi_vt;

   pi_vt = al_calloc(1, sizeof *pi_vt);

   pi_vt->initialize = pi_initialize;
   pi_vt->get_display_driver = _al_get_raspberrypi_display_interface;
   pi_vt->get_keyboard_driver = pi_get_keyboard_driver;
   pi_vt->get_mouse_driver = pi_get_mouse_driver;
   pi_vt->get_joystick_driver = pi_get_joystick_driver;
   pi_vt->get_num_display_modes = pi_get_num_display_modes;
   pi_vt->get_display_mode = pi_get_display_mode;
   pi_vt->shutdown_system = pi_shutdown_system;
   pi_vt->get_num_video_adapters = pi_get_num_video_adapters;
   pi_vt->get_monitor_info = pi_get_monitor_info;
   pi_vt->create_mouse_cursor = pi_create_mouse_cursor;
   pi_vt->destroy_mouse_cursor = pi_destroy_mouse_cursor;
   pi_vt->get_cursor_position = pi_get_cursor_position;
   pi_vt->get_path = _al_unix_get_path;
   pi_vt->inhibit_screensaver = pi_inhibit_screensaver;

   return pi_vt;
}


/* vim: set sts=3 sw=3 et: */
