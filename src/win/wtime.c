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



static DWORD _al_win_initial_time_low_res;

#define LARGE_INTEGER_TO_INT64(li) (((int64_t)li.HighPart << 32) | \
	(int64_t)li.LowPart)
static LARGE_INTEGER _al_win_initial_time_high_res;
static int64_t high_res_timer_freq;

static double (*real_current_time_func)(void);


static double low_res_current_time(void)
{
   return (double) (timeGetTime() - _al_win_initial_time_low_res) / 1000;
}


static double high_res_current_time(void)
{
	LARGE_INTEGER count;

	QueryPerformanceCounter(&count);

	int64_t c = LARGE_INTEGER_TO_INT64(count);

	return (double)c / high_res_timer_freq;
}


double al_current_time(void)
{
	return (*real_current_time_func)();
}


void _al_win_init_time(void)
{
   LARGE_INTEGER tmp_freq;

   QueryPerformanceFrequency(&tmp_freq);

   high_res_timer_freq = LARGE_INTEGER_TO_INT64(tmp_freq);

   if (high_res_timer_freq == 0) {
      real_current_time_func = low_res_current_time;
      _al_win_initial_time_low_res = timeGetTime();
   }
   else {
      real_current_time_func = high_res_current_time;
      QueryPerformanceCounter(&_al_win_initial_time_high_res);
   }
}





/* al_rest:
 *  Rests the specified amount of milliseconds.
 */
void al_rest(double seconds)
{
   ASSERT(seconds >= 0);

   Sleep((DWORD)(seconds * 1000.0));
}
