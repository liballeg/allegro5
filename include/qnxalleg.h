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
 *      Header file for the QNX Allegro library port.
 *
 *      You dont need this file; it is used internally to prototype some library
 *      functions, macros and similar stuff.
 */


#ifndef QNX_ALLEGRO_H
#define QNX_ALLEGRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/platform/aintqnx.h"

#include <pthread.h>
#include <Ph.h>
#include <Pt.h>

#define EVENT_SIZE        sizeof(PhEvent_t) + 1000

#define PH_GFX_NONE       0
#define PH_GFX_WINDOW     1
#define PH_GFX_DIRECT     2
#define PH_GFX_OVERLAY    3


/* Globals */

AL_VAR(PtWidget_t, *ph_window);
AL_VAR(int, ph_gfx_mode);
AL_VAR(PdDirectContext_t, *ph_direct_context);
AL_VAR(PdOffscreenContext_t, *ph_screen_context);
AL_VAR(PdOffscreenContext_t, *ph_window_context);
AL_VAR(pthread_mutex_t, qnx_events_mutex);
AL_VAR(pthread_mutex_t, *qnx_gfx_mutex);
AL_VAR(int, qnx_mouse_warped);
AL_VAR(char, *ph_dirty_lines);
AL_VAR(void, (*ph_update_window)(PhRect_t* rect));
AL_VAR(int, ph_window_w);
AL_VAR(int, ph_window_h);
AL_ARRAY(PgColor_t, ph_palette);


typedef void RETSIGTYPE;

#ifdef __cplusplus
}
#endif

#endif
