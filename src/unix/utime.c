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
#include <sys/select.h>

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
 *  Return the current time, in microseconds, since some arbitrary
 *  point in time.
 */
unsigned long al_current_time(void)
{
   struct timeval now;

   gettimeofday(&now, NULL);

   return ((now.tv_sec  - initial_time.tv_sec)  * 1000 +
           (now.tv_usec - initial_time.tv_usec) / 1000);
}



/* al_rest:
 *  Make the caller rest for some time.
 */
void al_rest(long msecs)
{
#ifdef ALLEGRO_MACOSX
   usleep(msecs * 1000);
#else
   struct timeval timeout;
   timeout.tv_sec = 0;
   timeout.tv_usec = msecs * 1000;
   select(0, NULL, NULL, NULL, &timeout);
#endif
}
