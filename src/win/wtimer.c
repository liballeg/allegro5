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
 *      Windows timer driver. 
 *
 *      By Stefan Schimanski.
 *
 *      Enhanced low performance driver by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
   #include <process.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


_DRIVER_INFO _timer_driver_list[] =
{
   {TIMER_WIN32_HIGH_PERF, &timer_win32_high_perf, TRUE},
   {TIMER_WIN32_LOW_PERF,  &timer_win32_low_perf,  TRUE},
   {0, NULL, 0}
};


static int tim_win32_high_perf_init(void);
static int tim_win32_low_perf_init(void);
static void tim_win32_exit(void);
static int can_simulate_retrace(void);
static void win32_rest(long time, AL_METHOD(void, callback, (void)));


TIMER_DRIVER timer_win32_high_perf =
{
   TIMER_WIN32_HIGH_PERF,
   empty_string,
   empty_string,
   "Win32 high performance timer",
   tim_win32_high_perf_init,
   tim_win32_exit,
   NULL, NULL, NULL, NULL,
   can_simulate_retrace,
   NULL,
   win32_rest
};


TIMER_DRIVER timer_win32_low_perf =
{
   TIMER_WIN32_LOW_PERF,
   empty_string,
   empty_string,
   "Win32 low performance timer",
   tim_win32_low_perf_init,
   tim_win32_exit,
   NULL, NULL, NULL, NULL,
   can_simulate_retrace,
   NULL,
   win32_rest
};


static HANDLE timer_thread = NULL;       /* dedicated timer thread      */
static HANDLE timer_stop_event = NULL;   /* thread termination event    */
static long timer_delay = 0;             /* how long between interrupts */
static int timer_semaphore = FALSE;      /* reentrant interrupt ?       */
static long vsync_counter;               /* retrace position counter    */
static long vsync_speed;                 /* retrace speed               */

/* high performance driver */
static LARGE_INTEGER counter_freq;
static LARGE_INTEGER counter_per_msec;

/* unit conversion */
#define COUNTER_TO_MSEC(x) ((unsigned long)(x / counter_per_msec.QuadPart))
#define COUNTER_TO_TIMER(x) ((unsigned long)(x * TIMERS_PER_SECOND / counter_freq.QuadPart))
#define MSEC_TO_COUNTER(x) (x * counter_per_msec.QuadPart)
#define TIMER_TO_COUNTER(x) (x * counter_freq.QuadPart / TIMERS_PER_SECOND)
#define TIMER_TO_MSEC(x) ((unsigned long)(x) / (TIMERS_PER_SECOND / 1000))



/* set_sync_timer_freq:
 *  sets the speed of the retrace counter.
 */
void set_sync_timer_freq(int freq)
{
   vsync_speed = BPS_TO_TIMER(freq);
}



/* tim_win32_handle_timer_tick:
 *  called by the driver to handle a tick.
 */
static int tim_win32_handle_timer_tick(int interval)
{
   long d;
   int i;
   long new_delay = 0x8000;

   timer_delay += interval;

   /* reentrant interrupt ? */
   if (timer_semaphore)
      return 0x2000;

   timer_semaphore = TRUE;
   d = timer_delay;

   /* deal with retrace synchronisation */
   vsync_counter -= d;

   if (vsync_counter <= 0) {
      vsync_counter += vsync_speed;
      retrace_count++;
      if (retrace_proc)
	 retrace_proc();
   }

   if (vsync_counter < new_delay)
      new_delay = vsync_counter;

   /* process the user callbacks */
   for (i = 0; i < MAX_TIMERS; i++) {
      if (((_timer_queue[i].proc) || (_timer_queue[i].param_proc)) && (_timer_queue[i].speed > 0)) {
	 _timer_queue[i].counter -= d;

	 while ((_timer_queue[i].counter <= 0) && ((_timer_queue[i].proc) || (_timer_queue[i].param_proc)) && (_timer_queue[i].speed > 0)) {
	    _timer_queue[i].counter += _timer_queue[i].speed;
	    if (_timer_queue[i].param_proc)
	       _timer_queue[i].param_proc(_timer_queue[i].param);
	    else
	       _timer_queue[i].proc();
	 }

	 if ((_timer_queue[i].counter > 0) && (_timer_queue[i].counter < new_delay))
	    new_delay = _timer_queue[i].counter;
      }
   }

   timer_delay -= d;
   timer_semaphore = FALSE;

   /* fudge factor to prevent interrupts from coming too close to each other */
   if (new_delay < MSEC_TO_TIMER(1))
      new_delay = MSEC_TO_TIMER(1);

   return new_delay;
}



/* tim_win32_high_perf_thread:
 *  thread loop function for the high performance driver.
 */
static void tim_win32_high_perf_thread(void *unused)
{
   DWORD result;
   unsigned long delay = 0x8000;
   LARGE_INTEGER curr_counter;
   LARGE_INTEGER prev_tick;
   LARGE_INTEGER diff_counter;

   /* get initial counter */
   QueryPerformanceCounter(&prev_tick);

   while (TRUE) {
      /* wait for foreground */
      if (WaitForSingleObject(_foreground_event, 0) == WAIT_TIMEOUT) {
	 thread_switch_out();
	 QueryPerformanceCounter(&prev_tick);
      }

      /* get current counter */
      QueryPerformanceCounter(&curr_counter);

      /* get counter units till next tick */
      diff_counter.QuadPart = curr_counter.QuadPart - prev_tick.QuadPart;

      /* save current counter for next tick */
      prev_tick.QuadPart = curr_counter.QuadPart;

      /* call timer proc */
      delay = tim_win32_handle_timer_tick(COUNTER_TO_TIMER(diff_counter.QuadPart));

      /* wait calculated time */
      result = WaitForSingleObject(timer_stop_event, TIMER_TO_MSEC(delay));
      if (result != WAIT_TIMEOUT)
	 return;
   }
}



/* tim_win32_low_perf_thread:
 *  thread loop function for the low performance driver.
 */
static void tim_win32_low_perf_thread(void *unused)
{
   DWORD result;
   unsigned long delay = 0x8000;
   DWORD prev_time;  
   DWORD curr_time;  /* in milliseconds */
   DWORD diff_time;

   /* get initial time */
   prev_time = timeGetTime();

   while (TRUE) {
      /* wait for foreground */
      if (WaitForSingleObject(_foreground_event, 0) == WAIT_TIMEOUT) {
	 thread_switch_out();
	 prev_time = timeGetTime();
      }

      /* get current time */
      curr_time = timeGetTime();

      /* get time till next tick */
      diff_time = curr_time - prev_time;

      /* save current time for next tick */
      prev_time = curr_time;

      /* call timer proc */
      delay = tim_win32_handle_timer_tick(MSEC_TO_TIMER(diff_time));

      /* wait calculated time */
      result = WaitForSingleObject(timer_stop_event, TIMER_TO_MSEC(delay));
      if (result != WAIT_TIMEOUT)
	 return;
   }
}



/* tim_win32_high_perf_init:
 *  initializes the high performance driver.
 */
static int tim_win32_high_perf_init(void)
{
   if (!QueryPerformanceFrequency(&counter_freq)) {
      _TRACE("High performance timer not supported\n");
      return -1;
   }

   /* setup high performance counter */
   counter_per_msec.QuadPart = counter_freq.QuadPart / 1000;

   /* create thread termination event */
   timer_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   /* start timer thread */
   timer_thread = (void *)_beginthread(tim_win32_high_perf_thread, 0, 0);

   /* increase priority of timer thread */
   SetThreadPriority(timer_thread, THREAD_PRIORITY_TIME_CRITICAL);

   return 0;
}



/* tim_win32_low_perf_init:
 *  initializes the low performance driver.
 */
static int tim_win32_low_perf_init(void)
{
   /* create thread termination event */
   timer_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   /* start timer thread */
   timer_thread = (void *)_beginthread(tim_win32_low_perf_thread, 0, 0);

   /* increase priority of timer thread */
   SetThreadPriority(timer_thread, THREAD_PRIORITY_TIME_CRITICAL);

   return 0;
}



/* tim_win32_exit:
 */
static void tim_win32_exit(void)
{
   /* acknowledge that the thread should stop */
   SetEvent(timer_stop_event);

   /* wait until thread has ended */
   WaitForSingleObject(timer_thread, INFINITE);

   /* thread has ended, now we can release all resources */
   CloseHandle(timer_stop_event);
}



/* can_simulate_retrace:
 */
static int can_simulate_retrace(void)
{
   return 0;
}



/* win32_rest:
 */
static void win32_rest(long time, AL_METHOD(void, callback, (void)))
{
   unsigned long start;
   unsigned long ms = time;

   if (callback) {
      start = timeGetTime();
      while (timeGetTime() - start < ms)
         (*callback)();
   }
   else {
      Sleep(ms);
   }
}
