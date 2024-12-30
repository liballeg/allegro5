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
 *      GP2X Wiz system driver
 *
 *      By Trent Gamblin.
 *
 */

#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_gp2xwiz.h"
#include "allegro5/platform/aintwiz.h"

static ALLEGRO_SYSTEM_INTERFACE *gp2xwiz_vt;

static ALLEGRO_SYSTEM *gp2xwiz_initialize(int flags)
{
   ALLEGRO_SYSTEM_GP2XWIZ *s;

   (void)flags;

   _al_unix_init_time();

   s = al_calloc(1, sizeof *s);

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_DISPLAY *));

   s->system.vt = gp2xwiz_vt;

   return &s->system;
}

static void gp2xwiz_shutdown_system(void)
{
   /* Close all open displays. */
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   ALLEGRO_SYSTEM_GP2XWIZ *sx = (void *)s;

   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);

   al_free(sx);

   lc_exit();

}

static ALLEGRO_DISPLAY_INTERFACE *gp2xwiz_get_display_driver(void)
{
   if (al_get_new_display_flags() & ALLEGRO_OPENGL)
      return _al_display_gp2xwiz_opengl_driver();
   else
      return _al_display_gp2xwiz_framebuffer_driver();
}

static ALLEGRO_KEYBOARD_DRIVER *gp2xwiz_get_keyboard_driver(void)
{
   //return _al_gp2xwiz_keyboard_driver;
   return NULL;
}

static ALLEGRO_MOUSE_DRIVER *gp2xwiz_get_mouse_driver(void)
{
   //return _al_gp2xwiz_mouse_driver;
   return NULL;
}

static ALLEGRO_JOYSTICK_DRIVER *gp2xwiz_get_joystick_driver(void)
{
   return _al_joystick_driver_list[0].driver;
}

static int gp2xwiz_get_num_video_adapters(void)
{
   return 1;
}

static bool gp2xwiz_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   (void)adapter;
   info->x1 = 0;
   info->y1 = 0;
   info->x2 = 320;
   info->y2 = 240;
   return true;
}

// FIXME
static bool gp2xwiz_get_cursor_position(int *ret_x, int *ret_y)
{
   *ret_x = 0;
   *ret_y = 0;
   return true;
}

static bool gp2xwiz_inhibit_screensaver(bool inhibit)
{
   (void)inhibit;
   return false;
}


static int gp2xwiz_get_num_display_modes(void)
{
   return 1;
}


static ALLEGRO_DISPLAY_MODE *gp2xwiz_get_display_mode(int index,
   ALLEGRO_DISPLAY_MODE *mode)
{
   (void)index;
   ASSERT(index == 0);
   mode->width = 320;
   mode->height = 240;
   mode->format = ALLEGRO_PIXEL_FORMAT_RGB_565;
   mode->refresh_rate = 60;
   return mode;
}


/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_gp2xwiz_driver(void)
{
   if (gp2xwiz_vt)
      return gp2xwiz_vt;

   gp2xwiz_vt = al_calloc(1, sizeof *gp2xwiz_vt);

   gp2xwiz_vt->id = ALLEGRO_SYSTEM_ID_GP2XWIZ;
   gp2xwiz_vt->initialize = gp2xwiz_initialize;
   gp2xwiz_vt->get_display_driver = gp2xwiz_get_display_driver;
   gp2xwiz_vt->get_keyboard_driver = gp2xwiz_get_keyboard_driver;
   gp2xwiz_vt->get_mouse_driver = gp2xwiz_get_mouse_driver;
   gp2xwiz_vt->get_joystick_driver = gp2xwiz_get_joystick_driver;
   gp2xwiz_vt->get_num_display_modes = gp2xwiz_get_num_display_modes;
   gp2xwiz_vt->get_display_mode = gp2xwiz_get_display_mode;
   gp2xwiz_vt->shutdown_system = gp2xwiz_shutdown_system;
   gp2xwiz_vt->get_num_video_adapters = gp2xwiz_get_num_video_adapters;
   gp2xwiz_vt->get_monitor_info = gp2xwiz_get_monitor_info;
   gp2xwiz_vt->get_cursor_position = gp2xwiz_get_cursor_position;
   gp2xwiz_vt->get_path = _al_unix_get_path;
   gp2xwiz_vt->inhibit_screensaver = gp2xwiz_inhibit_screensaver;
   gp2xwiz_vt->get_num_display_formats = gp2xwiz_get_num_display_formats;
   gp2xwiz_vt->get_time = _al_unix_get_time;
   gp2xwiz_vt->rest = _al_unix_rest;
   gp2xwiz_vt->init_timeout = _al_unix_init_timeout;

   return gp2xwiz_vt;
}

/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces(void)
{
   ALLEGRO_SYSTEM_INTERFACE **add;

   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_gp2xwiz_driver();
}

/* vim: set sts=3 sw=3 et: */
