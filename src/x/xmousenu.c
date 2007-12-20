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
 *      By Peter Wang.
 *
 *      Original by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_NO_COMPATIBILITY

#include "allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "xwin.h"



typedef struct ALLEGRO_MOUSE_XWIN
{
   ALLEGRO_MOUSE parent;
   ALLEGRO_MSESTATE state;
   int min_x, min_y;
   int max_x, max_y;
} ALLEGRO_MOUSE_XWIN;



static bool xmouse_installed = false;

/* the one and only mouse object */
static ALLEGRO_MOUSE_XWIN the_mouse;



/* forward declarations */
static bool xmouse_init(void);
static void xmouse_exit(void);
static ALLEGRO_MOUSE *xmouse_get_mouse(void);
static unsigned int xmouse_get_mouse_num_buttons(void);
static unsigned int xmouse_get_mouse_num_axes(void);
static bool xmouse_set_mouse_xy(int x, int y);
static bool xmouse_set_mouse_axis(int which, int z);
static bool xmouse_set_mouse_range(int x1, int y1, int x2, int y2);
static void xmouse_get_state(ALLEGRO_MSESTATE *ret_state);

static void wheel_motion_handler(int dz);
static int x_button_to_wheel(unsigned int x_button);
static unsigned int x_button_to_al_button(unsigned int x_button);
static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button);



/* the driver vtable */
#define MOUSEDRV_XWIN  AL_ID('X','W','I','N')

static ALLEGRO_MOUSE_DRIVER mousedrv_xwin =
{
   MOUSEDRV_XWIN,
   empty_string,
   empty_string,
   "X-Windows mouse",
   xmouse_init,
   xmouse_exit,
   xmouse_get_mouse,
   xmouse_get_mouse_num_buttons,
   xmouse_get_mouse_num_axes,
   xmouse_set_mouse_xy,
   xmouse_set_mouse_axis,
   xmouse_set_mouse_range,
   xmouse_get_state
};



/* list the available drivers */
_DRIVER_INFO _al_xwin_mouse_driver_list[] =
{
   {  MOUSEDRV_XWIN, &mousedrv_xwin, TRUE  },
   {  0,             NULL,           FALSE }
};



/* xmouse_init:
 *  Initialise the driver.
 */
static bool xmouse_init(void)
{
   _al_xwin_enable_hardware_cursor(true);

   memset(&the_mouse, 0, sizeof the_mouse);

   _al_event_source_init(&the_mouse.parent.es);

   xmouse_installed = true;

   return true;
}



/* xmouse_exit:
 *  Shut down the mouse driver.
 */
static void xmouse_exit(void)
{
   if (!xmouse_installed)
      return;
   xmouse_installed = false;

   /* ... */

   _al_event_source_free(&the_mouse.parent.es);
}



/* xmouse_get_mouse:
 *  Returns the address of a ALLEGRO_MOUSE structure representing the mouse.
 */
static ALLEGRO_MOUSE *xmouse_get_mouse(void)
{
   ASSERT(xmouse_installed);

   return (ALLEGRO_MOUSE *)&the_mouse;
}



/* xmouse_get_mouse_num_buttons:
 *  Return the number of buttons on the mouse.
 */
static unsigned int xmouse_get_mouse_num_buttons(void)
{
   int num_buttons;
   unsigned char map[8];

   ASSERT(xmouse_installed);

   XLOCK();
   num_buttons = XGetPointerMapping(_xwin.display, map, sizeof(map));
   XUNLOCK();

   num_buttons = CLAMP(2, num_buttons, 3);
   return num_buttons;
}



/* xmouse_get_mouse_num_axes:
 *  Return the number of axes on the mouse.
 */
static unsigned int xmouse_get_mouse_num_axes(void)
{
   ASSERT(xmouse_installed);

   /* XXX get the right number later */
   return 3;
}



/* xmouse_set_mouse_xy:
 *  Set the mouse position.  Return true if successful.
 */
static bool xmouse_set_mouse_xy(int x, int y)
{
   ASSERT(xmouse_installed);

   if ((x < 0) || (y < 0) || (x >= _xwin.window_width) || (y >= _xwin.window_height))
      return false;

   _al_event_source_lock(&the_mouse.parent.es);
   {
      XWarpPointer(_xwin.display, _xwin.window, _xwin.window, 0, 0,
                   _xwin.window_width, _xwin.window_height, x, y);
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* xmouse_set_mouse_axis:
 *  Set the mouse wheel position.  Return true if successful.
 */
static bool xmouse_set_mouse_axis(int which, int z)
{
   ASSERT(xmouse_installed);

   if (which != 2) {
      return false;
   }

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dz = (z - the_mouse.state.z);

      if (dz != 0) {
         the_mouse.state.z = z;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            0, 0, dz,
            0);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* xmouse_set_mouse_range:
 *  TODO
 */
static bool xmouse_set_mouse_range(int x1, int y1, int x2, int y2)
{
   return false;
}



/* xmouse_get_state:
 *  Copy the current mouse state into RET_STATE, with any necessary locking.
 */
static void xmouse_get_state(ALLEGRO_MSESTATE *ret_state)
{
   ASSERT(xmouse_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      *ret_state = the_mouse.state;
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* _al_xwin_mouse_button_press_handler: [bgman thread]
 *  Called by _xwin_process_event() for ButtonPress events received from the X
 *  server.
 */
void _al_xwin_mouse_button_press_handler(unsigned int x_button)
{
   int dz;
   unsigned int al_button;

   if (!xmouse_installed)
      return;

   /* Is this event actually generated for a mouse wheel movement? */
   /* TODO secondary wheel support */
   dz = x_button_to_wheel(x_button);
   if (dz != 0) {
      wheel_motion_handler(dz);
      return;
   }

   /* Is this button supported by the Allegro API? */
   al_button = x_button_to_al_button(x_button);
   if (al_button == 0)
      return;

   _al_event_source_lock(&the_mouse.parent.es);
   {
      the_mouse.state.buttons |= (1 << (al_button - 1));

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_BUTTON_DOWN,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         0, 0, 0,
         al_button);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* wheel_motion_handler: [bgman thread]
 *  Called by _al_xwin_mouse_button_press_handler() if the ButtonPress event
 *  received from the X server is actually for a mouse wheel movement.
 */
static void wheel_motion_handler(int dz)
{
   _al_event_source_lock(&the_mouse.parent.es);
   {
      the_mouse.state.z += dz;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         0, 0, dz,
         0);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* _al_xwin_mouse_button_release_handler: [bgman thread]
 *  Called by _xwin_process_event() for ButtonRelease events received from the
 *  X server.
 */
void _al_xwin_mouse_button_release_handler(unsigned int x_button)
{
   int al_button;

   if (!xmouse_installed)
      return;

   al_button = x_button_to_al_button(x_button);
   if (al_button == 0)
      return;

   _al_event_source_lock(&the_mouse.parent.es);
   {
      the_mouse.state.buttons &=~ (1 << (al_button - 1));

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_BUTTON_UP,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         0, 0, 0,
         al_button);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* _al_xwin_mouse_motion_notify_handler: [bgman thread]
 *  Called by _xwin_process_event() for MotionNotify events received from the X
 *  server.
 */
void _al_xwin_mouse_motion_notify_handler(int x, int y)
{
   if (!xmouse_installed)
      return;

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dx = x - the_mouse.state.x;
      int dy = y - the_mouse.state.y;
      the_mouse.state.x = x;
      the_mouse.state.y = y;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         dx, dy, 0,
         0);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* _al_xwin_mouse_motion_notify_handler_dga2: [bgman thread]
 *  Called by _xdga_process_event() for MotionNotify events received from the X
 *  server.  These are different from the MotionNotify events received when not
 *  in DGA2 mode.
 */
void _al_xwin_mouse_motion_notify_handler_dga2(int dx, int dy,
                                               int min_x, int min_y,
                                               int max_x, int max_y)
{
   if (!xmouse_installed)
      return;

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int x = the_mouse.state.x + dx;
      int y = the_mouse.state.y + dy;

      the_mouse.state.x = CLAMP(min_x, x, max_x);
      the_mouse.state.y = CLAMP(min_y, y, max_y);

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         dx, dy, 0,
         0);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* x_button_to_wheel: [bgman thread]
 *  Return a non-zero number if an X mouse button number corresponds to the
 *  mouse wheel scrolling, otherwise return 0.
 */
static int x_button_to_wheel(unsigned int x_button)
{
   switch (x_button) {
      case Button4:
         return +1;
      case Button5:
         return -1;
      default:
         /* not a wheel */
         return 0;
   }
}



/* x_button_to_al_button: [bgman thread]
 *  Map a X button number to an Allegro button number.
 */
static unsigned int x_button_to_al_button(unsigned int x_button)
{
   switch (x_button) {
      case Button1:
         return 1;
      case Button2:
         return 3;
      case Button3:
         return 2;
      default:
         /* unknown or wheel */
         return 0;
   }
}



/* generate_mouse_event: [bgman thread]
 *  Helper to generate a mouse event.
 */
static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button)
{
   ALLEGRO_EVENT *event;

   if (!_al_event_source_needs_to_generate_event(&the_mouse.parent.es))
      return;

   event = _al_event_source_get_unused_event(&the_mouse.parent.es);
   if (!event)
      return;

   event->mouse.type = type;
   event->mouse.timestamp = al_current_time();
   event->mouse.__display__dont_use_yet__ = NULL; /* TODO */
   event->mouse.x = x;
   event->mouse.y = y;
   event->mouse.z = z;
   event->mouse.dx = dx;
   event->mouse.dy = dy;
   event->mouse.dz = dz;
   event->mouse.button = button;
   _al_event_source_emit_event(&the_mouse.parent.es, event);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
