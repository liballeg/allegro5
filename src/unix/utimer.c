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
 *      Unix timer module.
 *
 *      By Peter Wang.
 *
 *      _unix_rest by Elias Pschernig.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include <sys/time.h>


#ifndef ALLEGRO_MACOSX

/* System drivers provide their own lists, so this is just to keep the 
 * Allegro framework happy.  */
_DRIVER_INFO _timer_driver_list[] = {
   { 0, 0, 0 }
};

#endif



void _unix_rest(unsigned int ms, void (*callback) (void))
{
   if (callback) {
      struct timeval tv, tv_end;
      gettimeofday (&tv_end, NULL);
      tv_end.tv_usec = (tv_end.tv_usec + ms * 1000) % 1000000;
      tv_end.tv_sec += (tv_end.tv_usec + ms * 1000) / 1000000;
      while (1)
      {
         (*callback)();
         gettimeofday (&tv, NULL);
         if (tv.tv_sec >= tv_end.tv_sec && tv.tv_usec >= tv_end.tv_usec)
            break;
      }
   }
   else {
#ifdef ALLEGRO_MACOSX
      usleep(ms * 1000);
#else
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = ms * 1000;
      select(0, NULL, NULL, NULL, &timeout);
#endif
   }
}
