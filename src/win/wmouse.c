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
 *      Windows mouse driver.
 *
 *      By Milan Mimica.
 *
 *      See readme.txt for copyright information.
 */

#if 0
/* Raw input */
#define _WIN32_WINNT 0x0501
#ifndef WINVER
#define WINVER 0x0600
#endif
#endif
#include <windows.h>

/*
 * Even the most recent MinGW at the moment of writing this is missing
 * this symbol.
 */
#ifndef SM_MOUSEHORIZONTALWHEELPRESENT
#define SM_MOUSEHORIZONTALWHEELPRESENT    91
#endif

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_display.h"

static ALLEGRO_MOUSE_STATE mouse_state;
static ALLEGRO_MOUSE the_mouse;
static bool installed = false;

// The raw versions of z/w in the mouse_state. They are related to them by a scaling constant.
static int raw_mouse_z = 0;
static int raw_mouse_w = 0;


static bool init_mouse(void)
{
   ALLEGRO_DISPLAY *display;

   if (installed)
      return false;

   /* If the display was created before the mouse is installed and the mouse
    * cursor is initially within the window, then the display field has correct
    * and useful info so don't clobber it.
    */
   display = mouse_state.display;
   memset(&mouse_state, 0, sizeof(mouse_state));
   mouse_state.display = display;

   _al_event_source_init(&the_mouse.es);

#if 0
   if (al_get_new_display_flags() & ALLEGRO_FULLSCREEN) {
      RAWINPUTDEVICE rid[1];
      rid[0].usUsagePage = 0x01;
      rid[0].usUsage = 0x02;
      rid[0].dwFlags = RIDEV_NOLEGACY;
      rid[0].hwndTarget = 0;
      if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == FALSE) {
         return false;
      }
   }
#endif

   installed = true;

   return true;
}


static void exit_mouse(void)
{
   if (!installed)
      return;

   memset(&mouse_state, 0, sizeof(mouse_state));
   _al_event_source_free(&the_mouse.es);
   installed = false;
}


static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z, int w, float pressure,
                                 int dx, int dy, int dz, int dw,
                                 unsigned int button,
                                 ALLEGRO_DISPLAY *source)
{
   ALLEGRO_EVENT event;

   if (!_al_event_source_needs_to_generate_event(&the_mouse.es))
      return;

   _al_event_source_lock(&the_mouse.es);
   event.mouse.type = type;
   event.mouse.timestamp = al_get_time();
   event.mouse.display = source;
   event.mouse.x = x;
   event.mouse.y = y;
   event.mouse.z = z;
   event.mouse.w = w;
   event.mouse.dx = dx;
   event.mouse.dy = dy;
   event.mouse.dz = dz;
   event.mouse.dw = dw;
   event.mouse.button = button;
   event.mouse.pressure = pressure;
   _al_event_source_emit_event(&the_mouse.es, &event);
   _al_event_source_unlock(&the_mouse.es);
}


static ALLEGRO_MOUSE* get_mouse(void)
{
   return &the_mouse;
}


static unsigned int get_num_buttons(void)
{
   return GetSystemMetrics(SM_CMOUSEBUTTONS);
}


static unsigned int get_num_axes(void)
{
   bool x = GetSystemMetrics(SM_MOUSEHORIZONTALWHEELPRESENT);
   bool z = GetSystemMetrics(SM_MOUSEWHEELPRESENT);
   if (x && z)
      return 4;
   if (x || z)
      return 3;
   return 2;
}


static bool set_mouse_xy(ALLEGRO_DISPLAY *disp, int x, int y)
{
   int dx, dy;
   POINT pt;
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)disp;

   if (!installed)
      return false;

   dx = x - mouse_state.x;
   dy = y - mouse_state.y;

   if (dx || dy) {
      mouse_state.x = x;
      mouse_state.y = y;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_WARPED,
         mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
         dx, dy, 0, 0,
         0, (void*)win_disp);
   }

   pt.x = x;
   pt.y = y;

   ClientToScreen(win_disp->window, &pt);

   SetCursorPos(pt.x, pt.y);

   return true;
}


static bool set_mouse_axis(int which, int val)
{
   /* Vertical mouse wheel. */
   if (which == 2) {
      int dz = (val - mouse_state.z);

      raw_mouse_z = WHEEL_DELTA * val / al_get_mouse_wheel_precision();

      if (dz != 0) {
         mouse_state.z = val;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
            0, 0, dz, 0,
            0, mouse_state.display);
      }

      return true;
   }

   /* Horizontal mouse wheel. */
   if (which == 3) {
      int dw = (val - mouse_state.w);

      raw_mouse_w = WHEEL_DELTA * val / al_get_mouse_wheel_precision();

      if (dw != 0) {
         mouse_state.w = val;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
            0, 0, 0, dw,
            0, mouse_state.display);
      }

      return true;
   }

   return false;
}


static void get_mouse_state(ALLEGRO_MOUSE_STATE *ret_state)
{
   _al_event_source_lock(&the_mouse.es);
   *ret_state = mouse_state;
   _al_event_source_unlock(&the_mouse.es);
}


/* the driver vtable */
#define MOUSE_WINAPI AL_ID('W','A','P','I')

static ALLEGRO_MOUSE_DRIVER mousedrv_winapi =
{
   MOUSE_WINAPI,
   "",
   "",
   "WinAPI mouse",
   init_mouse,
   exit_mouse,
   get_mouse,
   get_num_buttons,
   get_num_axes,
   set_mouse_xy,
   set_mouse_axis,
   get_mouse_state
};


_AL_DRIVER_INFO _al_mouse_driver_list[] =
{
   {MOUSE_WINAPI, &mousedrv_winapi, true},
   {0, NULL, 0}
};


void _al_win_mouse_handle_leave(ALLEGRO_DISPLAY_WIN *win_disp)
{
   /* The state should be updated even if the mouse is not installed so that
    * it will be correct if the mouse is installed later.
    */
   if (mouse_state.display == (void*)win_disp)
      mouse_state.display = NULL;

   if (!installed)
      return;

   generate_mouse_event(ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY,
      mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
      0, 0, 0, 0,
      0, (void*)win_disp);
}


void _al_win_mouse_handle_enter(ALLEGRO_DISPLAY_WIN *win_disp)
{
   /* The state should be updated even if the mouse is not installed so that
    * it will be correct if the mouse is installed later.
    */
   mouse_state.display = (void*)win_disp;

   if (!installed)
      return;

   generate_mouse_event(ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY,
      mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
      0, 0, 0, 0,
      0, (void*)win_disp);
}


void _al_win_mouse_handle_move(int x, int y, bool abs, ALLEGRO_DISPLAY_WIN *win_disp)
{
   int dx, dy;
   int oldx, oldy;

   oldx = mouse_state.x;
   oldy = mouse_state.y;

   if (!installed)
      return;

   if (!abs) {
      mouse_state.x += x;
      mouse_state.y += y;
      dx = x;
      dy = y;
   }
   else {
      dx = x - mouse_state.x;
      dy = y - mouse_state.y;
      mouse_state.x = x;
      mouse_state.y = y;
   }

   if (oldx != mouse_state.x || oldy != mouse_state.y) {
      generate_mouse_event(ALLEGRO_EVENT_MOUSE_AXES,
         mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
         dx, dy, 0, 0,
         0, (void*)win_disp);
   }
}


void _al_win_mouse_handle_wheel(int raw_dz, bool abs, ALLEGRO_DISPLAY_WIN *win_disp)
{
   int d;
   int new_z;

   if (!installed)
      return;

   if (!abs) {
      raw_mouse_z += raw_dz;
   }
   else {
      raw_mouse_z = raw_dz;
   }

   new_z = al_get_mouse_wheel_precision() * raw_mouse_z / WHEEL_DELTA;
   d = new_z - mouse_state.z;
   mouse_state.z = new_z;

   generate_mouse_event(ALLEGRO_EVENT_MOUSE_AXES,
      mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
      0, 0, d, 0,
      0, (void*)win_disp);
}


void _al_win_mouse_handle_hwheel(int raw_dw, bool abs, ALLEGRO_DISPLAY_WIN *win_disp)
{
   int d;
   int new_w;

   if (!installed)
      return;

   if (!abs) {
      raw_mouse_w += raw_dw;
   }
   else {
      raw_mouse_w = raw_dw;
   }

   new_w = al_get_mouse_wheel_precision() * raw_mouse_w / WHEEL_DELTA;
   d = new_w - mouse_state.w;
   mouse_state.w = new_w;

   generate_mouse_event(ALLEGRO_EVENT_MOUSE_AXES,
      mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
      0, 0, 0, d,
      0, (void*)win_disp);
}



void _al_win_mouse_handle_button(int button, bool down, int x, int y, bool abs,
                                 ALLEGRO_DISPLAY_WIN *win_disp)
{
   int type = down ? ALLEGRO_EVENT_MOUSE_BUTTON_DOWN
                   : ALLEGRO_EVENT_MOUSE_BUTTON_UP;

   if (!installed)
      return;

   if (!abs) {
      mouse_state.x += x;
      mouse_state.y += y;
   }
   else {
      mouse_state.x = x;
      mouse_state.y = y;
   }

   if (down)
      mouse_state.buttons |= (1 << (button-1));
   else
      mouse_state.buttons &= ~(1 << (button-1));

   mouse_state.pressure = mouse_state.buttons ? 1.0 : 0.0; /* TODO */

   generate_mouse_event(type,
      mouse_state.x, mouse_state.y, mouse_state.z, mouse_state.w, mouse_state.pressure,
      0, 0, 0, 0,
      button, (void*)win_disp);
}

/* vim: set sts=3 sw=3 et: */
