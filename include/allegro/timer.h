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

#ifndef ALLEGRO_TIMER_H
#define ALLEGRO_TIMER_H

#include "allegro/base.h"

AL_BEGIN_EXTERN_C


/* Convenient macros */
#define AL_SECS_TO_MSECS(x)      ((long)(x) * 1000)
#define AL_BPS_TO_MSECS(x)       (1000 / (long)(x))
#define AL_BPM_TO_MSECS(x)       ((60 * 1000) / (long)(x))


/* Abstract data type */
typedef struct AL_TIMER AL_TIMER;


AL_TIMER *al_install_timer(long speed_msecs);
void al_uninstall_timer(AL_TIMER*);
void al_start_timer(AL_TIMER*);
void al_stop_timer(AL_TIMER*);
bool al_timer_is_started(AL_TIMER*);
long al_timer_get_speed(AL_TIMER*);
void al_timer_set_speed(AL_TIMER*, long speed_msecs);
long al_timer_get_count(AL_TIMER*);
void al_timer_set_count(AL_TIMER*, long count);


AL_END_EXTERN_C

#endif          /* ifndef ALLEGRO_TIMER_H */
