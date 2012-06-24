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
 *      Unix time module.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <sys/time.h>
#include <math.h>

#include "allegro5/altime.h"
#include "allegro5/debug.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintuthr.h"

ALLEGRO_STATIC_ASSERT(utime,
   sizeof(ALLEGRO_TIMEOUT_UNIX) <= sizeof(ALLEGRO_TIMEOUT));


/* Marks the time Allegro was initialised, for al_get_time(). */
struct timeval _al_unix_initial_time;



/* _al_unix_init_time:
 *  Called by the system driver to mark the beginning of time.
 */
void _al_unix_init_time(void)
{
   gettimeofday(&_al_unix_initial_time, NULL);
}



/* Function: al_get_time
 */
double al_get_time(void)
{
   struct timeval now;
   double time;

   gettimeofday(&now, NULL);
   time = (double) (now.tv_sec - _al_unix_initial_time.tv_sec)
      + (double) (now.tv_usec - _al_unix_initial_time.tv_usec) * 1.0e-6;
   return time;
}



/* Function: al_rest
 */
void al_rest(double seconds)
{
   struct timespec timeout;
   double fsecs = floor(seconds);
   timeout.tv_sec = (time_t) fsecs;
   timeout.tv_nsec = (suseconds_t) ((seconds - fsecs) * 1e9);
   nanosleep(&timeout, 0);
}



/* Function: al_init_timeout
 */
void al_init_timeout(ALLEGRO_TIMEOUT *timeout, double seconds)
{
    ALLEGRO_TIMEOUT_UNIX *ut = (ALLEGRO_TIMEOUT_UNIX *) timeout;
    struct timeval now;
    double integral;
    double frac;

    ASSERT(ut);

    gettimeofday(&now, NULL);

    if (seconds <= 0.0) {
	ut->abstime.tv_sec = now.tv_sec;
	ut->abstime.tv_nsec = now.tv_usec * 1000;
    }
    else {
	frac = modf(seconds, &integral);

	ut->abstime.tv_sec = now.tv_sec + integral;
	ut->abstime.tv_nsec = (now.tv_usec * 1000) + (frac * 1000000000L);
	ut->abstime.tv_sec += ut->abstime.tv_nsec / 1000000000L;
	ut->abstime.tv_nsec = ut->abstime.tv_nsec % 1000000000L;
    }
}

/* vim: set sts=3 sw=3 et */
