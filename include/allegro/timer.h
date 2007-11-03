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

/* Title: Timer routines
 */

#ifndef ALLEGRO_TIMER_H
#define ALLEGRO_TIMER_H

#include "allegro/base.h"

AL_BEGIN_EXTERN_C



/* Macros: conversion macros
 *  AL_SECS_TO_MSECS - seconds to milliseconds
 *  AL_BPS_TO_MSECS - beats per second to milliseconds
 *  AL_BPM_TO_MSECS - beats per minute to milliseconds
 *
 *  These macros convert from various time units into milliseconds.
 */
#define ALLEGRO_SECS_TO_MSECS(x)      ((long)(x) * 1000)
#define ALLEGRO_BPS_TO_MSECS(x)       (1000 / (long)(x))
#define ALLEGRO_BPM_TO_MSECS(x)       ((60 * 1000) / (long)(x))


/* Type: ALLEGRO_TIMER
 *  This is an abstract data type representing a timer object.
 *  A timer object can act as an event source so can be casted to
 *  ALLEGRO_EVENT_SOURCE*.
 */
typedef struct ALLEGRO_TIMER ALLEGRO_TIMER;


/* Function: al_install_timer
 *  Install a new timer.  If successful, a pointer to a new timer object is
 *  returned, otherwise NULL is returned.  SPEED_MSECS is in milliseconds per
 *  "tick", and must be positive.  The new timer is initially stopped.
 * 
 *  The system driver must be installed before this function can be called.
 * 
 *  Usage note: typical granularity is on the order of milliseconds.
 */
AL_FUNC(ALLEGRO_TIMER*, al_install_timer, (long speed_msecs));


/* Function: al_uninstall_timer
 *  Uninstall the timer specified.  If the timer is started, it will
 *  automatically be stopped before uninstallation.  It will also
 *  automatically unregister the timer with any event queues.
 *
 *  TIMER may not be NULL.
 */
AL_FUNC(void, al_uninstall_timer, (ALLEGRO_TIMER *timer));


/* Function: al_start_timer
 *  Start the timer specified.  From then, the timer's counter will increment
 *  at a constant rate, and it will begin generating events.  Starting a
 *  timer that is already started does nothing.
 */
AL_FUNC(void, al_start_timer, (ALLEGRO_TIMER *timer));


/* Function: al_stop_timer
 *  Stop the timer specified.  The timer's counter will stop incrementing and
 *  it will stop generating events.  Stopping a timer that is already stopped
 *  does nothing.
 */
AL_FUNC(void, al_stop_timer, (ALLEGRO_TIMER *timer));


/* Function: al_timer_is_started
 *  Return true if the timer specified is currently started.
 */
AL_FUNC(bool, al_timer_is_started, (ALLEGRO_TIMER *timer));


/* Function: al_timer_get_speed
 *  Return the timer's speed.
 */
AL_FUNC(long, al_timer_get_speed, (ALLEGRO_TIMER *timer));


/* Function: al_timer_set_speed
 *  Set the timer's speed, i.e. the rate at which its counter will be
 *  incremented when it is started.  This can be done when the timer is
 *  started or stopped.  If the timer is currently running, it is made to
 *  look as though the speed change occured precisely at the last tick.
 *  SPEED_MSECS is in milliseconds per "tick", and must be positive.
 * 
 *  Usage note: typical granularity is on the order of milliseconds.
 */
AL_FUNC(void, al_timer_set_speed, (ALLEGRO_TIMER *timer, long speed_msecs));


/* Function: al_timer_get_count
 *  Return the timer's counter value.  The timer can be started or stopped.
 */
AL_FUNC(long, al_timer_get_count, (ALLEGRO_TIMER *timer));


/* Function: al_timer_set_count
 *  Change a timer's counter value.  The timer can be started or stopped.
 *  COUNT value may be positive or negative, but will always be incremented
 *  by +1.
 */
AL_FUNC(void, al_timer_set_count, (ALLEGRO_TIMER *timer, long count));



AL_END_EXTERN_C

#endif          /* ifndef ALLEGRO_TIMER_H */
