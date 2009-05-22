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

#ifndef WINVER
#define WINVER 0x0600
#endif
#include <windows.h>

/*
 * Even the most recent MinGW at the moment of writing this is missing
 * this symbol.
 */
#ifndef SM_MOUSEHORIZONTALWHEELPRESENT
#define SM_MOUSEHORIZONTALWHEELPRESENT    91
#endif

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_display.h"

ALLEGRO_MOUSE the_mouse;
ALLEGRO_MOUSE_STATE mouse_state;

static bool installed = false;


bool init_mouse(void)
{
   if (installed)
      return false;

   memset(&mouse_state, 0, sizeof the_mouse);
   _al_event_source_init(&the_mouse.es);

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

   installed = true;

   return true;
}


static void exit_mouse(void)
{
   if (!installed)
      return;

   memset(&mouse_state, 0, sizeof the_mouse);
   _al_event_source_free(&the_mouse.es);
   installed = false;
}

static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button,
                                 ALLEGRO_DISPLAY *source)
{
   ALLEGRO_EVENT event;

   if (!_al_event_source_needs_to_generate_event(&the_mouse.es))
      return;

   _al_event_source_lock(&the_mouse.es);
   event.mouse.type = type;
   event.mouse.timestamp = al_current_time();
   event.mouse.display = source;
   event.mouse.x = x;
   event.mouse.y = y;
   event.mouse.z = z;
   event.mouse.dx = dx;
   event.mouse.dy = dy;
   event.mouse.dz = dz;
   event.mouse.button = button;
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


static bool set_mouse_xy(int x, int y)
{
   int new_x, new_y;
   int dx, dy;
   int wx, wy;
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)al_get_current_display();

   if (!installed)
      return false;

   new_x = _ALLEGRO_CLAMP(win_disp->mouse_range_x1, x, win_disp->mouse_range_x2);
   new_y = _ALLEGRO_CLAMP(win_disp->mouse_range_y1, y, win_disp->mouse_range_y2);

   dx = new_x - mouse_state.x;
   dy = new_y - mouse_state.y;

   if (dx || dy) {
      mouse_state.x = x;
      mouse_state.y = y;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_WARPED,
         mouse_state.x, mouse_state.y, mouse_state.z,
         dx, dy, 0,
         0, (void*)win_disp);
   }

   _al_win_get_window_position(win_disp->window, &wx, &wy);

   if (!(win_disp->display.flags & ALLEGRO_FULLSCREEN)) {
      SetCursorPos(new_x+wx, new_y+wy);
   }

   return true;
}


static bool set_mouse_axis(int which, int z)
{
   ALLEGRO_DISPLAY *disp = al_get_current_display();

   if (which != 2) {
      return false;
   }

   {
      int dz = (z - mouse_state.z);

      if (dz != 0) {
         mouse_state.z = z;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            mouse_state.x, mouse_state.y, mouse_state.z,
            0, 0, dz,
            0, disp);
      }
   }

   return true;
}

static bool set_mouse_range(int x1, int y1, int x2, int y2)
{
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)al_get_current_display();
   int new_x, new_y;
   int dx, dy;

   win_disp->mouse_range_x1 = x1;
   win_disp->mouse_range_y1 = y1;
   win_disp->mouse_range_x2 = x2;
   win_disp->mouse_range_y2 = y2;

   new_x = _ALLEGRO_CLAMP(win_disp->mouse_range_x1, mouse_state.x, win_disp->mouse_range_x2);
   new_y = _ALLEGRO_CLAMP(win_disp->mouse_range_y1, mouse_state.y, win_disp->mouse_range_y2);

   dx = new_x - mouse_state.x;
   dy = new_y - mouse_state.y;

   if (dx || dy) {
      mouse_state.x = new_x;
      mouse_state.y = new_y;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_AXES,
         mouse_state.x, mouse_state.y, mouse_state.z,
         dx, dy, 0,
         0, (void*)win_disp);
   }

   return true;
}

static void get_mouse_state(ALLEGRO_MOUSE_STATE *ret_state)
{
   unsigned int i;
   ALLEGRO_DISPLAY *disp = NULL;
   ALLEGRO_SYSTEM *sys;

   _al_event_source_lock(&the_mouse.es);
   {
      sys = al_system_driver();
      for (i = 0; i < sys->displays._size; i++) {
         ALLEGRO_DISPLAY_WIN **d = (void*)_al_vector_ref(&sys->displays, i);
         if ((*d)->window == GetForegroundWindow()) {
            disp = (void*)*d;
            break;
         }
      }
      mouse_state.display = disp;
      *ret_state = mouse_state;
   }
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
   set_mouse_range,
   get_mouse_state
};


_DRIVER_INFO _al_mouse_driver_list[] =
{
   {MOUSE_WINAPI, &mousedrv_winapi, true},
   {0, NULL, 0}
};


void _al_win_mouse_handle_leave(ALLEGRO_DISPLAY_WIN *win_disp)
{
   if (!installed)
      return;
   generate_mouse_event(ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY,
      mouse_state.x, mouse_state.y, mouse_state.z,
      0, 0, 0,
      mouse_state.buttons, (void*)win_disp);
}


void _al_win_mouse_handle_enter(ALLEGRO_DISPLAY_WIN *win_disp)
{
   if (!installed)
      return;
   generate_mouse_event(ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY,
      mouse_state.x, mouse_state.y, mouse_state.z,
      0, 0, 0,
      mouse_state.buttons, (void*)win_disp);
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

   mouse_state.x = _ALLEGRO_CLAMP(win_disp->mouse_range_x1, mouse_state.x, win_disp->mouse_range_x2);
   mouse_state.y = _ALLEGRO_CLAMP(win_disp->mouse_range_y1, mouse_state.y, win_disp->mouse_range_y2);

   if (oldx != mouse_state.x || oldy != mouse_state.y) {
      generate_mouse_event(ALLEGRO_EVENT_MOUSE_AXES,
         mouse_state.x, mouse_state.y, mouse_state.z,
         dx, dy, 0,
         mouse_state.buttons, (void*)win_disp);
   }
}


void _al_win_mouse_handle_wheel(int z, bool abs, ALLEGRO_DISPLAY_WIN *win_disp)
{
   int d;

   if (!installed)
      return;

   if (!abs) {
      mouse_state.z += z;
      d = z;
   }
   else {
      d = z - mouse_state.z;
      mouse_state.z = z;
   }

   generate_mouse_event(ALLEGRO_EVENT_MOUSE_AXES,
      mouse_state.x, mouse_state.y, mouse_state.z,
      0, 0, d,
      mouse_state.buttons, (void*)win_disp);
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

   mouse_state.buttons = button;

   generate_mouse_event(type,
   mouse_state.x, mouse_state.y, mouse_state.z,
   0, 0, 0,
   mouse_state.buttons, (void*)win_disp);
}
