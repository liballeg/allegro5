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
struct timespec _al_unix_initial_time;

/* clock_gettime() doesn't exist in MacOSX versions < 10.12. Use
 * gettimeofday() if building for MacOSX and targeting version < 10.12.
 */
#if defined(ALLEGRO_MACOSX) && MAC_OS_X_VERSION_MIN_REQUIRED < 101200
#include <mach/mach_time.h>
int _internal_clock_gettime(clockid_t clock_id, struct timespec* t)
{
   struct timeval now;

   (void)clock_id;
   gettimeofday(&now, NULL);
   t->tv_sec = now.tv_sec;
   t->tv_nsec = now.tv_usec * 1000;
   return 0;
}
#define _al_clock_gettime _internal_clock_gettime
#else
#define _al_clock_gettime clock_gettime
#endif

// Prefer clock that never decreases and includes time suspended.
// Can probably use CLOCK_MONOTONIC_RAW for non-mac too, but remains to be
// verified.
#if defined(ALLEGRO_MACOSX)
#define CLOCK_ID CLOCK_MONOTONIC_RAW
#else
#define CLOCK_ID CLOCK_REALTIME
#endif



/* _al_unix_init_time:
 *  Called by the system driver to mark the beginning of time.
 */
void _al_unix_init_time(void)
{
   _al_clock_gettime(CLOCK_ID, &_al_unix_initial_time);
}



double _al_unix_get_time(void)
{
   struct timespec now;
   double time;

   _al_clock_gettime(CLOCK_ID, &now);
   time = (double) (now.tv_sec - _al_unix_initial_time.tv_sec)
      + (double) (now.tv_nsec - _al_unix_initial_time.tv_nsec) * 1.0e-9;
   return time;
}



void _al_unix_rest(double seconds)
{
   struct timespec timeout;
   double integral;
   double frac;

   frac = modf(seconds, &integral);
   timeout.tv_sec = (time_t)integral;
   timeout.tv_nsec = (suseconds_t)(frac * 1e9);
   nanosleep(&timeout, 0);
}



void _al_unix_init_timeout(ALLEGRO_TIMEOUT *timeout, double seconds)
{
   ALLEGRO_TIMEOUT_UNIX *ut = (ALLEGRO_TIMEOUT_UNIX *) timeout;
   struct timespec now;
   double integral;
   double frac;

   ASSERT(ut);

   _al_clock_gettime(CLOCK_ID, &now);

   if (seconds <= 0.0) {
      ut->abstime.tv_sec = now.tv_sec;
      ut->abstime.tv_nsec = now.tv_nsec;
   }
   else {
      frac = modf(seconds, &integral);

      ut->abstime.tv_sec = now.tv_sec + integral;
      ut->abstime.tv_nsec = now.tv_nsec + (frac * 1000000000L);
      ut->abstime.tv_sec += ut->abstime.tv_nsec / 1000000000L;
      ut->abstime.tv_nsec = ut->abstime.tv_nsec % 1000000000L;
   }
}

/* vim: set sts=3 sw=3 et */
