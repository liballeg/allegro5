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
   _unix_get_executable_name,       /* AL_METHOD(void, get_executable_name, (char *output, int size)); */
   _unix_find_resource,             /* AL_METHOD(int, find_resource, (char *dest, AL_CONST char *resource, int size)); */
   qnx_sys_set_window_title,        /* AL_METHOD(void, set_window_title, (AL_CONST char *name)); */
   qnx_sys_set_window_close_button, /* AL_METHOD(int, set_window_close_button, (int enable)); */
   qnx_sys_set_window_close_hook,   /* AL_METHOD(void, set_window_close_hook, (AL_METHOD(void, proc, (void)))); */
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
   qnx_sys_set_display_switch_mode, /* AL_METHOD(int, set_display_switch_mode, (int mode)); */
   qnx_sys_set_display_switch_cb,   /* AL_METHOD(int, set_display_switch_callback, (int dir, AL_METHOD(void, cb, (void)))); */
   qnx_sys_remove_display_switch_cb,/* AL_METHOD(void, remove_display_switch_callback, (AL_METHOD(void, cb, (void)))); */
   NULL,                            /* AL_METHOD(void, display_switch_lock, (int lock, int foreground)); */
   qnx_sys_desktop_color_depth,     /* AL_METHOD(int, desktop_color_depth, (void)); */
   qnx_sys_get_desktop_resolution,  /* AL_METHOD(int, get_desktop_resolution, (int *width, int *height)); */
   _unix_yield_timeslice,           /* AL_METHOD(void, yield_timeslice, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, digi_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, midi_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL,                            /* AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void)); */
   qnx_sys_timer_drivers            /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};



/* Keyboard driver */

KEYBOARD_DRIVER keyboard_qnx =
{
   KEYBOARD_QNX,
   empty_string,
   empty_string,
   "QNX keyboard",
   TRUE,
   qnx_keyboard_init,
   qnx_keyboard_exit,
   NULL,
   NULL,
   NULL,
   NULL, NULL,
   _pckey_scancode_to_ascii
};



_DRIVER_INFO _keyboard_driver_list[] =
{
   { KEYBOARD_QNX,      &keyboard_qnx,      TRUE  },
   { 0,                 NULL,               0     }
};



/* Mouse driver */

MOUSE_DRIVER mouse_qnx =
{
   MOUSE_QNX,
   empty_string,
   empty_string,
   "Photon mouse",
   qnx_mouse_init,
   qnx_mouse_exit,
   NULL, //qnx_mouse_poll,
   NULL, //qnx_mouse_timer_poll,
   qnx_mouse_position,
   qnx_mouse_set_range,
   NULL, //qnx_mouse_set_speed,
   qnx_mouse_get_mickeys,
   NULL
};

_DRIVER_INFO _mouse_driver_list[] =
{
   { MOUSE_QNX,         &mouse_qnx,         TRUE  },
   { 0,                 NULL,               0     }
};



/* Graphics drivers */

GFX_DRIVER gfx_photon = {
   GFX_PHOTON,
   empty_string, 
   empty_string,
   "Photon", 
   qnx_ph_init,                  /* AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth)); */
   qnx_ph_exit,                  /* AL_METHOD(void, exit, (struct BITMAP *b)); */
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync,                 /* AL_METHOD(void, vsync, (void)); */
   qnx_ph_set_palette,           /* AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   TRUE
};



GFX_DRIVER gfx_photon_direct = {
   GFX_PHOTON_DIRECT,
   empty_string, 
   empty_string,
   "Photon direct",
   qnx_phd_init,                 /* AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth)); */
   qnx_phd_exit,                 /* AL_METHOD(void, exit, (struct BITMAP *b)); */
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync,                 /* AL_METHOD(void, vsync, (void)); */
   qnx_ph_set_palette,           /* AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE
};



BEGIN_GFX_DRIVER_LIST
   { GFX_PHOTON_DIRECT, &gfx_photon_direct, TRUE  },
   { GFX_PHOTON,        &gfx_photon,        TRUE  },
END_GFX_DRIVER_LIST



/* Joystick driver */

BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST



/* MIDI drivers */

BEGIN_MIDI_DRIVER_LIST
   { MIDI_ALSA,         &midi_alsa,         TRUE  },
END_MIDI_DRIVER_LIST



/* Sound drivers */

BEGIN_DIGI_DRIVER_LIST
   { DIGI_ALSA,         &digi_alsa,         TRUE  },
END_DIGI_DRIVER_LIST

