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
 *      Internal header for the QNX Allegro library; this is mainly used for driver
 *      functions prototypes.
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
AL_FUNC(void, qnx_sys_message, (AL_CONST char*));
AL_FUNC(_DRIVER_INFO *, qnx_timer_drivers, (void));

AL_FUNC(int, qnx_keyboard_init, (void));
AL_FUNC(void, qnx_keyboard_exit, (void));
AL_FUNC(void, qnx_keyboard_handler, (int, int));
AL_FUNC(void, qnx_keyboard_focused, (int, int));


#ifdef __cplusplus
}
#endif

#endif 