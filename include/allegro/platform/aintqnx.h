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

#include "allegro/platform/aintunix.h"

#ifndef SCAN_DEPEND
   #include <pthread.h>
   #include <Ph.h>
   #include <Pt.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


#define PH_GFX_NONE       0
#define PH_GFX_WINDOW     1
#define PH_GFX_DIRECT     2
#define PH_GFX_OVERLAY    3

/* from qphoton.c */
AL_VAR(int, ph_gfx_mode);
AL_FUNCPTR(void, ph_update_window, (PhRect_t* rect));
AL_VAR(PdOffscreenContext_t, *ph_window_context);
AL_ARRAY(PgColor_t, ph_palette);

/* from qsystem.c */
AL_VAR(PtWidget_t, *ph_window);
AL_VAR(pthread_mutex_t, qnx_events_mutex);
AL_VAR(pthread_mutex_t, *qnx_gfx_mutex);

/* from qkeydrv.c */
AL_FUNC(void, qnx_keyboard_handler, (int, int));
AL_FUNC(void, qnx_keyboard_focused, (int, int));

/* from qmouse.c */
AL_VAR(int, qnx_mouse_warped);
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
