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
 *      Stuff for BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif 



SYSTEM_DRIVER system_beos = {
   SYSTEM_BEOS,                 // int id;
   empty_string,                // char *name;
   empty_string,                // char *desc;
   "System",                    // char *ascii_name;
   be_sys_init,                 // AL_METHOD(int, init, (void));
   be_sys_exit,                 // AL_METHOD(void, exit, (void));
   be_sys_get_executable_name,  // AL_METHOD(void, get_executable_name, (char *output, int size));
   be_sys_find_resource,        // AL_METHOD(int, find_resource, (char *dest, char *resource, int size));
   be_sys_set_window_title,     // AL_METHOD(void, set_window_title, (char *name));
   be_sys_set_window_close_button, // AL_METHOD(int, set_window_close_button, (int enable));
   be_sys_set_window_close_hook,   // AL_METHOD(void, set_window_close_hook, (void (*proc)()));
   be_sys_message,              // AL_METHOD(void, message, (char *msg));
   NULL,                        // AL_METHOD(void, assert, (char *msg));
   NULL,                        // AL_METHOD(void, save_console_state, (void));
   NULL,                        // AL_METHOD(void, restore_console_state, (void));
   NULL,                        // AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height));
   NULL,                        // AL_METHOD(void, created_bitmap, (struct BITMAP *bmp));
   NULL,                        // AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));
   NULL,                        // AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));
   NULL,                        // AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap));
   NULL,                        // AL_METHOD(void, read_hardware_palette, (void));
   NULL,                        // AL_METHOD(void, set_palette_range, (struct RGB *p, int from, int to, int vsync));
   NULL,                        // AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth));
   be_sys_set_display_switch_mode, // AL_METHOD(int, set_display_switch_mode, (int mode));
   be_sys_set_display_switch_cb,// AL_METHOD(int, set_display_switch_callback, (int dir, AL_METHOD(void, cb, (void))));
   be_sys_remove_display_switch_cb, // AL_METHOD(int, remove_display_switch_callback, (AL_METHOD(void, cb, (void))));
   NULL,                        // AL_METHOD(void, display_switch_lock, (int lock));
   be_sys_desktop_color_depth,  // AL_METHOD(int, desktop_color_depth, (void));
   be_sys_get_desktop_resolution, // AL_METHOD(int, get_desktop_resolution, (int *width, int *height));
   be_sys_yield_timeslice,      // AL_METHOD(void, yield_timeslice, (void));
   NULL,                        // AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void));
   NULL,                        // AL_METHOD(_DRIVER_INFO *, digi_drivers, (void));
   NULL,                        // AL_METHOD(_DRIVER_INFO *, midi_drivers, (void));
   NULL,                        // AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void));
   NULL,                        // AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void));
   NULL,                        // AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void));
   NULL                         // AL_METHOD(_DRIVER_INFO *, timer_drivers, (void));
};
