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
 *      Timer routines.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __al_included_allegro5_timer_h
#define __al_included_allegro5_timer_h

#include "allegro5/base.h"
#include "allegro5/events.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Function: A5O_USECS_TO_SECS
 */
#define A5O_USECS_TO_SECS(x)      ((x) / 1000000.0)

/* Function: A5O_MSECS_TO_SECS
 */
#define A5O_MSECS_TO_SECS(x)      ((x) / 1000.0)

/* Function: A5O_BPS_TO_SECS
 */
#define A5O_BPS_TO_SECS(x)        (1.0 / (x))

/* Function: A5O_BPM_TO_SECS
 */
#define A5O_BPM_TO_SECS(x)        (60.0 / (x))


/* Type: A5O_TIMER
 */
typedef struct A5O_TIMER A5O_TIMER;


AL_FUNC(A5O_TIMER*, al_create_timer, (double speed_secs));
AL_FUNC(void, al_destroy_timer, (A5O_TIMER *timer));
AL_FUNC(void, al_start_timer, (A5O_TIMER *timer));
AL_FUNC(void, al_stop_timer, (A5O_TIMER *timer));
AL_FUNC(void, al_resume_timer, (A5O_TIMER *timer));
AL_FUNC(bool, al_get_timer_started, (const A5O_TIMER *timer));
AL_FUNC(double, al_get_timer_speed, (const A5O_TIMER *timer));
AL_FUNC(void, al_set_timer_speed, (A5O_TIMER *timer, double speed_secs));
AL_FUNC(int64_t, al_get_timer_count, (const A5O_TIMER *timer));
AL_FUNC(void, al_set_timer_count, (A5O_TIMER *timer, int64_t count));
AL_FUNC(void, al_add_timer_count, (A5O_TIMER *timer, int64_t diff));
AL_FUNC(A5O_EVENT_SOURCE *, al_get_timer_event_source, (A5O_TIMER *timer));


#ifdef __cplusplus
   }
#endif

#endif
