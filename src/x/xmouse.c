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
 *      X-Windows mouse module.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"


static int mouse_minx = 0;
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;
static int mymickey_y = 0;



static int _xwin_mousedrv_init(void);
static void _xwin_mousedrv_exit(void);
static void _xwin_mousedrv_position(int x, int y);
static void _xwin_mousedrv_set_range(int x1, int y1, int x2, int y2);
static void _xwin_mousedrv_set_speed(int xspeed, int yspeed);
static void _xwin_mousedrv_get_mickeys(int *mickeyx, int *mickeyy);


static MOUSE_DRIVER mouse_xwin =
{
   MOUSE_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows mouse",
   _xwin_mousedrv_init,
   _xwin_mousedrv_exit,
   NULL,
   NULL,
   _xwin_mousedrv_position,
   _xwin_mousedrv_set_range,
   _xwin_mousedrv_set_speed,
   _xwin_mousedrv_get_mickeys,
   NULL
};



/* list the available drivers */
_DRIVER_INFO _xwin_mouse_driver_list[] =
{
   {  MOUSE_XWINDOWS, &mouse_xwin, TRUE  },
   {  0,              NULL,        0     }
};



/* _xwin_mousedrv_handler:
 *  Mouse "interrupt" handler for mickey-mode driver.
 */
static void _xwin_mousedrv_handler(int x, int y, int z, int buttons)
{
   _mouse_b = buttons;

   mymickey_x += x;
   mymickey_y += y;

   _mouse_x += x;
   _mouse_y += y;
   _mouse_z += z;

   if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx)
       || (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {
      _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
      _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);
   }

   _handle_mouse_input();
}



/* _xwin_mousedrv_init:
 *  Initializes the mickey-mode driver.
 */
static int _xwin_mousedrv_init(void)
{
   int num_buttons;
   unsigned char map[8];

   num_buttons = _xwin_get_pointer_mapping(map, sizeof(map));
   num_buttons = MID(2, num_buttons, 3);

   XLOCK();

   _xwin_mouse_interrupt = _xwin_mousedrv_handler;

   XUNLOCK();

   return num_buttons;
}



/* _xwin_mousedrv_exit:
 *  Shuts down the mickey-mode driver.
 */
static void _xwin_mousedrv_exit(void)
{
   XLOCK();

   _xwin_mouse_interrupt = 0;

   XUNLOCK();
}



/* _xwin_mousedrv_position:
 *  Sets the position of the mickey-mode mouse.
 */
static void _xwin_mousedrv_position(int x, int y)
{
   XLOCK();

   _mouse_x = x;
   _mouse_y = y;

   mymickey_x = mymickey_y = 0;

   XUNLOCK();

   _xwin_set_warped_mouse_mode(FALSE);
}



/* _xwin_mousedrv_set_range:
 *  Sets the range of the mickey-mode mouse.
 */
static void _xwin_mousedrv_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   XLOCK();

   _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

   XUNLOCK();
}



/* _xwin_mousedrv_set_speed:
 *  Sets the speed of the mickey-mode mouse.
 */
static void _xwin_mousedrv_set_speed(int xspeed, int yspeed)
{
   /* Use xset utility with "m" option.  */
}



/* _xwin_mousedrv_get_mickeys:
 *  Reads the mickey-mode count.
 */
static void _xwin_mousedrv_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;

   _xwin_set_warped_mouse_mode(TRUE);
}

