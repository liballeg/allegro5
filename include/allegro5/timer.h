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

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Macros: conversion macros
 *  ALLEGRO_USECS_TO_SECS - microseconds to seconds
 *  ALLEGRO_MSECS_TO_SECS - milliseconds to seconds
 *  ALLEGRO_BPS_TO_SECS - beats per second to seconds
 *  ALLEGRO_BPM_TO_SECS - beats per minute to seconds
 *  ALLEGRO_SECS_TO_MSECS - seconds to milliseconds
 *  ALLEGRO_SECS_TO_USECS - seconds to microseconds
 *  These macros convert from various time units into milliseconds.
 */
#define ALLEGRO_USECS_TO_SECS(x)      (x / 1000000.0)
#define ALLEGRO_MSECS_TO_SECS(x)      (x / 1000.0)
#define ALLEGRO_BPS_TO_SECS(x)        (1.0 / x)
#define ALLEGRO_BPM_TO_SECS(x)        (60.0 / x)
#define ALLEGRO_SECS_TO_MSECS(x)      (x * 1000.0)
#define ALLEGRO_SECS_TO_USECS(x)      (x * 1000000.0)


/* Type: ALLEGRO_TIMER
 */
typedef struct ALLEGRO_TIMER ALLEGRO_TIMER;


/* Function: al_install_timer
 */
AL_FUNC(ALLEGRO_TIMER*, al_install_timer, (double speed_secs));


/* Function: al_uninstall_timer
 */
AL_FUNC(void, al_uninstall_timer, (ALLEGRO_TIMER *timer));


/* Function: al_start_timer
 */
AL_FUNC(void, al_start_timer, (ALLEGRO_TIMER *timer));


/* Function: al_stop_timer
 */
AL_FUNC(void, al_stop_timer, (ALLEGRO_TIMER *timer));


/* Function: al_timer_is_started
 */
AL_FUNC(bool, al_timer_is_started, (ALLEGRO_TIMER *timer));


/* Function: al_get_timer_speed
 */
AL_FUNC(double, al_get_timer_speed, (ALLEGRO_TIMER *timer));


/* Function: al_set_timer_speed
 */
AL_FUNC(void, al_set_timer_speed, (ALLEGRO_TIMER *timer, double speed_secs));


/* Function: al_get_timer_count
 */
AL_FUNC(long, al_get_timer_count, (ALLEGRO_TIMER *timer));


/* Function: al_set_timer_count
 */
AL_FUNC(void, al_set_timer_count, (ALLEGRO_TIMER *timer, long count));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_TIMER_H */
