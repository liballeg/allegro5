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
 *      List of QNX driver tables.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "qnxalleg.h"
#include "allegro/aintern.h"
#include "allegro/aintqnx.h"

#ifndef ALLEGRO_QNX
#error Something is wrong with the makefile
#endif



/* System driver */

SYSTEM_DRIVER system_qnx =
{
   SYSTEM_QNX,
   empty_string,
   empty_string,
   "QNX Realtime Platform",
   qnx_sys_init,                    /* AL_METHOD(int, init, (void)); */
   qnx_sys_exit,                    /* AL_METHOD(void, exit, (void)); */
   NULL,                            /* AL_METHOD(void, get_executable_name, (char *output, int size)); */
   NULL,                            /* AL_METHOD(int, find_resource, (char *dest, AL_CONST char *resource, int size)); */
   NULL,                            /* AL_METHOD(void, set_window_title, (AL_CONST char *name)); */
   NULL,                            /* AL_METHOD(int, set_window_close_button, (int enable)); */
   NULL,                            /* AL_METHOD(void, set_window_close_hook, (AL_METHOD(void, proc, (void)))); */
   qnx_sys_message,                 /* AL_METHOD(void, message, (AL_CONST char *msg)); */
   NULL,                            /* AL_METHOD(void, assert, (AL_CONST char *msg)); */
   NULL,                            /* AL_METHOD(void, save_console_state, (void)); */
   NULL,                            /* AL_METHOD(void, restore_console_state, (void)); */
   NULL,                            /* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
   NULL,                            /* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
   NULL,                            /* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
   NULL,                            /* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent)); */
   NULL,                            /* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
   NULL,                            /* AL_METHOD(void, read_hardware_palette, (void)); */
   NULL,                            /* AL_METHOD(void, set_palette_range, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,                            /* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
   NULL,                            /* AL_METHOD(int, set_display_switch_mode, (int mode)); */
   NULL,                            /* AL_METHOD(int, set_display_switch_callback, (int dir, AL_METHOD(void, cb, (void)))); */
   NULL,                            /* AL_METHOD(void, remove_display_switch_callback, (AL_METHOD(void, cb, (void)))); */
   NULL,                            /* AL_METHOD(void, display_switch_lock, (int lock, int foreground)); */
   NULL,                            /* AL_METHOD(int, desktop_color_depth, (void)); */
   NULL,                            /* AL_METHOD(void, yield_timeslice, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, digi_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, midi_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void)); */
   qnx_timer_drivers                /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};



_DRIVER_INFO _system_driver_list[] =
{
   {SYSTEM_QNX,       &system_qnx,     TRUE  },
   {SYSTEM_NONE,      &system_none,    FALSE },
   {0,                NULL,            0     }
};



/* Keyboard driver */

_DRIVER_INFO _keyboard_driver_list[] =
{
   { 0,               NULL,            0     }
};



/* Mouse driver */

_DRIVER_INFO _mouse_driver_list[] =
{
   {0,                NULL,            0     }	 
};



/* Graphics drivers */

BEGIN_GFX_DRIVER_LIST
END_GFX_DRIVER_LIST



/* Joystick driver */

BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST



/* MIDI drivers */

BEGIN_MIDI_DRIVER_LIST
{ MIDI_ALSA,          &midi_alsa,      TRUE  },
END_MIDI_DRIVER_LIST



/* Sound drivers */

BEGIN_DIGI_DRIVER_LIST
{ DIGI_ALSA,          &digi_alsa,      TRUE  },
END_DIGI_DRIVER_LIST

