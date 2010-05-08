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

ALLEGRO_STATIC_ASSERT(sizeof(ALLEGRO_TIMEOUT_UNIX) <= sizeof(ALLEGRO_TIMEOUT));


/* Marks the time Allegro was initialised, for al_current_time(). */
static struct timeval initial_time;
#ifdef ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
static bool clock_monotonic;
static struct timespec initial_time_ns;
#endif



/* _al_unix_init_time:
 *  Called by the system driver to mark the beginning of time.
 */
void _al_unix_init_time(void)
{
#ifdef ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
   clock_monotonic = (clock_gettime(CLOCK_MONOTONIC, &initial_time_ns) == 0);
   if (!clock_monotonic)
#endif
   {
      gettimeofday(&initial_time, NULL);
   }
}



/* Function: al_current_time
 */
double al_current_time(void)
{
#ifdef ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
   if (clock_monotonic) {
      struct timespec now_ns;
      clock_gettime(CLOCK_MONOTONIC, &now_ns);
      return (double) (now_ns.tv_sec - initial_time_ns.tv_sec)
         + (double) (now_ns.tv_nsec - initial_time_ns.tv_nsec) * 1.0e-9;
   }
   else
#endif
   {
      struct timeval now;
      gettimeofday(&now, NULL);
      return (double) (now.tv_sec - initial_time.tv_sec)
         + (double) (now.tv_usec - initial_time.tv_usec) * 1.0e-6;
   }
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
   struct timespec now_ns;
   double integral;
   double frac;

   ASSERT(ut);

#ifdef ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
   if (clock_monotonic) {
      clock_gettime(CLOCK_MONOTONIC, &now_ns);
   }
   else
#endif
   {
      gettimeofday(&now, NULL);
      now_ns.tv_sec = now.tv_sec;
      now_ns.tv_nsec = now.tv_usec * 1000;
   }

   if (seconds <= 0.0) {
      ut->abstime = now_ns;
   }
   else {
      frac = modf(seconds, &integral);

      ut->abstime.tv_sec = now_ns.tv_sec + integral;
      ut->abstime.tv_nsec = now_ns.tv_nsec + (frac * 1000000000L);
      ut->abstime.tv_sec += ut->abstime.tv_nsec / 1000000000L;
      ut->abstime.tv_nsec = ut->abstime.tv_nsec % 1000000000L;
   }
}

/* vim: set sts=3 sw=3 et */
