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


/* Marks the time Allegro was initialised, for al_current_time(). */
static struct timeval initial_time;



/* _al_unix_init_time:
 *  Called by the system driver to mark the beginning of time.
 */
void _al_unix_init_time(void)
{
   gettimeofday(&initial_time, NULL);
}



/* al_current_time:
 *  Return the current time, with microsecond resolution, since some arbitrary
 *  point in time.
 */
double al_current_time(void)
{
   struct timeval now;
   gettimeofday(&now, 0); //null = timezone afaik
   double time = (double) (now.tv_sec - initial_time.tv_sec)
      + (double) (now.tv_usec - initial_time.tv_usec) * 1.0e-6;
   return time;
}



/* al_rest:
 *  Make the caller rest for some time.
 */
void al_rest(double seconds)
{
   struct timespec timeout;
   double fsecs = floor(seconds);
   timeout.tv_sec = (time_t) fsecs;
   timeout.tv_nsec = (suseconds_t) ((seconds - fsecs) * 1e9);
   nanosleep(&timeout, 0);
}
