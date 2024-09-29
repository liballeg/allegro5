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


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/platform/aintwin.h"

#include <mmsystem.h>

A5O_STATIC_ASSERT(wtime,
   sizeof(A5O_TIMEOUT_WIN) <= sizeof(A5O_TIMEOUT));


#define LARGE_INTEGER_TO_INT64(li) (((int64_t)li.HighPart << 32) | \
	(int64_t)li.LowPart)

static int64_t high_res_timer_freq;
static int64_t _al_win_prev_time;
static double _al_win_total_time;

static double (*real_get_time_func)(void);

static _AL_MUTEX time_mutex = _AL_MUTEX_UNINITED;


static double low_res_current_time(void)
{
    int64_t cur_time;
    double ellapsed_time;

    _al_mutex_lock(&time_mutex);
   
   cur_time = (int64_t) timeGetTime();
   ellapsed_time = (double) (cur_time - _al_win_prev_time) / 1000;

   if (cur_time < _al_win_prev_time) {
       ellapsed_time += 4294967.295;
   }

   _al_win_total_time += ellapsed_time;
   _al_win_prev_time = cur_time;

   _al_mutex_unlock(&time_mutex);

   return _al_win_total_time;
}


static double high_res_current_time(void)
{
   LARGE_INTEGER count;
   int64_t cur_time;
   double ellapsed_time;

   _al_mutex_lock(&time_mutex);

   QueryPerformanceCounter(&count);

   cur_time = LARGE_INTEGER_TO_INT64(count);
   ellapsed_time = (double)(cur_time - _al_win_prev_time) / (double)high_res_timer_freq;

   _al_win_total_time += ellapsed_time;
   _al_win_prev_time = cur_time;

   _al_mutex_unlock(&time_mutex);

   return _al_win_total_time;
}


double _al_win_get_time(void)
{
   return (*real_get_time_func)();
}


void _al_win_init_time(void)
{
   LARGE_INTEGER tmp_freq;
   _al_win_total_time = 0;

   _al_mutex_init(&time_mutex);
   
   if (QueryPerformanceFrequency(&tmp_freq) == 0) {
      real_get_time_func = low_res_current_time;
      _al_win_prev_time = (int64_t) timeGetTime();
   }
   else {
      LARGE_INTEGER count;
      high_res_timer_freq = LARGE_INTEGER_TO_INT64(tmp_freq);
      real_get_time_func = high_res_current_time;
      QueryPerformanceCounter(&count);
      _al_win_prev_time = LARGE_INTEGER_TO_INT64(count);
   }
}



void _al_win_shutdown_time(void)
{
   _al_mutex_destroy(&time_mutex);
}



void _al_win_rest(double seconds)
{
   if (seconds <= 0)
      return;

   Sleep((DWORD)(seconds * 1000.0));
}



void _al_win_init_timeout(A5O_TIMEOUT *timeout, double seconds)
{
   A5O_TIMEOUT_WIN *wt = (A5O_TIMEOUT_WIN *) timeout;

   ASSERT(wt);
   ASSERT(seconds <= INT_MAX/1000.0);

   if (seconds <= 0.0) {
      wt->abstime = timeGetTime();
   }
   else {
      wt->abstime = timeGetTime() + (DWORD)(seconds * 1000.0);
   }
}

/* vim: set sts=3 sw=3 et */
