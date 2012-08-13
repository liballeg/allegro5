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
 *      Linux console mouse driver for tslib (touch screens).
 *
 *      By Tobi Vollebregt.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_LINUX_TSLIB

#include <stdio.h>
#include <tslib.h>

#define PREFIX_I                "al-tslib INFO: "
#define PREFIX_W                "al-tslib WARNING: "
#define PREFIX_E                "al-tslib ERROR: "

#define ARRAY_SIZE(a)           ((int)sizeof((a)) / (int)sizeof((a)[0]))

static struct tsdev *ts;

static int mouse_minx = 0;          /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;


static int al_tslib_error_callback(const char *fmt, va_list ap)
{
   char tmp[ALLEGRO_ERROR_SIZE];
   int ret;

   ret = vsnprintf(tmp, sizeof(tmp), fmt, ap);

   TRACE(PREFIX_E "%s\n", tmp);
   uconvert(tmp, U_ASCII, allegro_error, U_CURRENT, ALLEGRO_ERROR_SIZE);

   return ret;
}

static int mouse_init(void)
{
   char tmp1[128], tmp2[128];
   AL_CONST char *udevice;

   /* Set custom error handling function */
   ts_error_fn = al_tslib_error_callback;

   /* Find the device filename */
   udevice = get_config_string(uconvert_ascii("mouse", tmp1),
                               uconvert_ascii("mouse_device", tmp2),
                               NULL);

   /* Open mouse device.  Devices are cool. */
   if (udevice) {
      TRACE(PREFIX_I "Trying %s device\n", udevice);
      ts = ts_open(uconvert_toascii(udevice, tmp1), TRUE);
      if (ts == NULL) {
         uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to open %s: %s"),
                   udevice, ustrerror(errno));
         return -1;
      }
   }
   else {
      /* If not specified in the config file, try several /dev/input/event<n>
       * devices. */
      const char *device_name[] = { "/dev/input/event0",
                                    "/dev/input/event1",
                                    "/dev/input/event2",
                                    "/dev/input/event3",
                                    NULL };
      int i;

      TRACE(PREFIX_I "Trying /dev/input/event[0-3] devices\n");

      for (i=0; device_name[i]; i++) {
         ts = ts_open(device_name[i], TRUE);
         if (ts != NULL)
	    break;
      }

      if (!device_name[i]) {
	 uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to open a touch screen device: %s"),
		   ustrerror(errno));
	 return -1;
      }
   }

   if (ts_config(ts)) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to configure touch screen device: %s"),
                ustrerror(errno));
      ts_close(ts);
      return -1;
   }

   return 0;
}

static void mouse_exit(void)
{
   if (ts) {
      ts_close(ts);
      ts = NULL;
   }
}

static void mouse_position(int x, int y)
{
   _mouse_x = CLAMP(mouse_minx, x, mouse_maxx);
   _mouse_y = CLAMP(mouse_miny, y, mouse_maxy);
}

static void mouse_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   mouse_position(_mouse_x, _mouse_y);
}

static void mouse_timer_poll(void)
{
   struct ts_sample samp[16];
   int n;

   n = ts_read(ts, samp, ARRAY_SIZE(samp));
   if (n > 0) {
      --n;
      mouse_position(samp[n].x, samp[n].y);
      _mouse_b = samp[n].pressure > 0;
      /*TRACE(PREFIX_I "Read %d samples.  x:%3d y:%3d pressure:%3d\n",
	      n + 1, samp[n].x, samp[n].y, samp[n].pressure);*/
   }
}

MOUSE_DRIVER mousedrv_linux_tslib =
{
   MOUSEDRV_LINUX_TSLIB,
   empty_string,
   empty_string,
   "Linux tslib touch screen",
   mouse_init,
   mouse_exit,
   NULL, /* poll */
   mouse_timer_poll,
   mouse_position,
   mouse_set_range,
   NULL, /* set_speed */
   NULL, /* get_mickeys */
   NULL, /* analyse_data */
   NULL, /* enable_hardware_cursor */
   NULL, /* select_system_cursor */
};

#endif
