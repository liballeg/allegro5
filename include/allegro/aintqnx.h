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
 *      Internal header for the QNX Allegro library; this is mainly used for
 *      driver functions prototypes.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTQNX_H
#define AINTQNX_H

#include "qnxalleg.h"
#include "allegro/aintunix.h"

#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(int, qnx_sys_init, (void));
AL_FUNC(void, qnx_sys_exit, (void));
AL_FUNC(void, qnx_sys_message, (AL_CONST char *));
AL_FUNC(void, qnx_sys_set_window_title, (AL_CONST char *));
AL_FUNC(int, qnx_sys_set_window_close_button, (int));
AL_FUNC(void, qnx_sys_set_window_close_hook, (AL_METHOD(void, proc, (void))));
AL_FUNC(int, qnx_sys_set_display_switch_mode, (int mode));
AL_FUNC(int, qnx_sys_set_display_switch_cb, (int dir, AL_METHOD(void, cb, (void))));
AL_FUNC(void, qnx_sys_remove_display_switch_cb, (AL_METHOD(void, cb, (void))));
AL_FUNC(void, qnx_sys_yield_timeslice, (void));

AL_FUNC(int, qnx_sys_desktop_color_depth, (void));
AL_FUNC(int, qnx_sys_get_desktop_resolution, (int *width, int *height));
AL_FUNC(_DRIVER_INFO *, qnx_sys_timer_drivers, (void));

AL_FUNC(int, qnx_keyboard_init, (void));
AL_FUNC(void, qnx_keyboard_exit, (void));
AL_FUNC(void, qnx_keyboard_handler, (int, int));
AL_FUNC(void, qnx_keyboard_focused, (int, int));

AL_FUNC(int, qnx_mouse_init, (void));
AL_FUNC(void, qnx_mouse_exit, (void));
AL_FUNC(void, qnx_mouse_position, (int, int));
AL_FUNC(void, qnx_mouse_set_range, (int, int, int, int));
AL_FUNC(void, qnx_mouse_set_speed, (int, int));
AL_FUNC(void, qnx_mouse_get_mickeys, (int *, int *));
AL_FUNC(void, qnx_mouse_handler, (int, int, int, int));

AL_FUNC(struct BITMAP *, qnx_phd_init, (int, int, int, int, int));
AL_FUNC(void, qnx_phd_exit, (struct BITMAP *));
AL_FUNC(struct BITMAP *, qnx_ph_init, (int, int, int, int, int));
AL_FUNC(void, qnx_ph_exit, (struct BITMAP *));
AL_FUNC(void, qnx_ph_vsync, (void));
AL_FUNC(void, qnx_ph_set_palette, (AL_CONST struct RGB *, int, int, int));


/* A very strange thing: PgWaitHWIdle() cannot be found in any system
 * header file, but it is explained in the QNX docs, and it actually
 * exists in the Photon library... So until QNX fixes the missing declaration,
 * we will declare it here.
 */
int PgWaitHWIdle(void);


#ifdef __cplusplus
}
#endif

#endif 