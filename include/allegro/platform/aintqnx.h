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
 *      Internal header for the QNX Allegro library.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTQNX_H
#define AINTQNX_H

#include "qnxalleg.h"
#include "allegro/platform/aintunix.h"

#ifdef __cplusplus
extern "C" {
#endif


AL_FUNC(void, qnx_keyboard_handler, (int, int));
AL_FUNC(void, qnx_keyboard_focused, (int, int));

AL_FUNC(void, qnx_mouse_handler, (int, int, int, int));


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