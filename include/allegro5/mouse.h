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
 *      Mouse routines.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __al_included_allegro5_mouse_h
#define __al_included_allegro5_mouse_h

#include "allegro5/base.h"
#include "allegro5/events.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Allow up to four extra axes for future expansion. */
#define A5O_MOUSE_MAX_EXTRA_AXES	 4


typedef struct A5O_MOUSE A5O_MOUSE;


/* Type: A5O_MOUSE_STATE
 */
typedef struct A5O_MOUSE_STATE A5O_MOUSE_STATE;

struct A5O_MOUSE_STATE
{
   /* (x, y) Primary mouse position
    * (z) Mouse wheel position (1D 'wheel'), or,
    * (w, z) Mouse wheel position (2D 'ball')
    * display - the display the mouse is on (coordinates are relative to this)
    * pressure - the pressure applied to the mouse (for stylus/tablet)
    */
   int x;
   int y;
   int z;
   int w;
   int more_axes[A5O_MOUSE_MAX_EXTRA_AXES];
   int buttons;
   float pressure;
   struct A5O_DISPLAY *display;
};


AL_FUNC(bool,           al_is_mouse_installed,  (void));
AL_FUNC(bool,           al_install_mouse,       (void));
AL_FUNC(void,           al_uninstall_mouse,     (void));
AL_FUNC(unsigned int,   al_get_mouse_num_buttons, (void));
AL_FUNC(unsigned int,   al_get_mouse_num_axes,  (void));
AL_FUNC(bool,           al_set_mouse_xy,        (struct A5O_DISPLAY *display, int x, int y));
AL_FUNC(bool,           al_set_mouse_z,         (int z));
AL_FUNC(bool,           al_set_mouse_w,         (int w));
AL_FUNC(bool,           al_set_mouse_axis,      (int axis, int value));
AL_FUNC(void,           al_get_mouse_state,     (A5O_MOUSE_STATE *ret_state));
AL_FUNC(bool,           al_mouse_button_down,   (const A5O_MOUSE_STATE *state, int button));
AL_FUNC(int,            al_get_mouse_state_axis, (const A5O_MOUSE_STATE *state, int axis));
AL_FUNC(bool, al_can_get_mouse_cursor_position, (void));
AL_FUNC(bool, al_get_mouse_cursor_position, (int *ret_x, int *ret_y));
AL_FUNC(bool, al_grab_mouse, (struct A5O_DISPLAY *display));
AL_FUNC(bool, al_ungrab_mouse, (void));
AL_FUNC(void, al_set_mouse_wheel_precision, (int precision));
AL_FUNC(int, al_get_mouse_wheel_precision, (void));

AL_FUNC(A5O_EVENT_SOURCE *, al_get_mouse_event_source, (void));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
