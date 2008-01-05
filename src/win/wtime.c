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
 *      Windows time module.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <mmsystem.h>
#endif



DWORD _al_win_initial_time;



void _al_win_init_time(void)
{
   _al_win_initial_time = timeGetTime();
}



unsigned long al_current_time(void)
{
   /* XXX: Maybe use QueryPerformanceCounter? */
   return timeGetTime() - _al_win_initial_time;
}



/* al_rest:
 *  Rests the specified amount of milliseconds.
 */
void al_rest(long msecs)
{
   ASSERT(msecs >= 0);

   Sleep(msecs);
}
