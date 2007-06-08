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
 *      New timer API for Windows. 
 *
 *      By Stefan Schimanski.
 *
 *      Enhanced low performance driver by Eric Botcazou.
 *
 *      Hacked into new timer API by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_dtor.h"
#include "allegro/internal/aintern_events.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <mmsystem.h>
   #include <process.h>
#endif

#define PREFIX_I                "al-wtimer INFO: "
#define PREFIX_W                "al-wtimer WARNING: "
#define PREFIX_E                "al-wtimer ERROR: "


/* readability typedefs */
typedef long msecs_t;
typedef long usecs_t;


/* forward declarations */
static usecs_t timer_thread_handle_tick(usecs_t interval);
static void timer_handle_tick(AL_TIMER *this);


struct AL_TIMER
{
   AL_EVENT_SOURCE es;
   bool started;
   usecs_t speed_usecs;
   long count;
   long counter;		/* counts down to zero=blastoff */
};



/*
 * The timer thread that runs in the background to drive the timers.
 */

static _AL_THREAD timer_thread; /* dedicated thread */
static HANDLE timer_stop_event; /* dedicated thread termination event */
static _AL_MUTEX timer_thread_mutex = _AL_MUTEX_UNINITED;
static _AL_VECTOR active_timers = _AL_VECTOR_INITIALIZER(AL_TIMER *);


/* high performance driver */
static LARGE_INTEGER counter_freq;

/* unit conversion */
#define COUNTER_TO_USEC(x) ((unsigned long)(x * 1000000 / counter_freq.QuadPart))
#define USEC_TO_MSEC(x) ((unsigned long)(x) / 1000)
#define MSEC_TO_USEC(x) (x * 1000)



/* tim_win32_high_perf_thread: [timer thread]
 *  Thread loop function for the high performance driver.
 */
static void tim_win32_high_perf_thread(_AL_THREAD *self, void *unused)
{
   DWORD result;
   usecs_t delay = 0x8000;
   LARGE_INTEGER curr_counter;
   LARGE_INTEGER prev_tick;
   LARGE_INTEGER diff_counter;

   /* init thread */
   _win_thread_init();

   /* get initial counter */
   QueryPerformanceCounter(&prev_tick);

   while (true) {

      _al_mutex_lock(&timer_thread_mutex);
      {
#if 0
         if (!app_foreground) {
            /* restart counter if the thread was blocked */
            if (thread_switch_out())
               QueryPerformanceCounter(&prev_tick);
         }
#endif

         /* get current counter */
         QueryPerformanceCounter(&curr_counter);

         /* get counter units till next tick */
         diff_counter.QuadPart = curr_counter.QuadPart - prev_tick.QuadPart;

         /* save current counter for next tick */
         prev_tick.QuadPart = curr_counter.QuadPart;

         /* call timer procs */
         delay = timer_thread_handle_tick(COUNTER_TO_USEC(diff_counter.QuadPart));
      }
      _al_mutex_unlock(&timer_thread_mutex);

      /* wait calculated time */
      result = MsgWaitForMultipleObjects(1, &timer_stop_event, FALSE, USEC_TO_MSEC(delay), QS_ALLINPUT);
      if (result != WAIT_TIMEOUT) {
         _win_thread_exit();
         return;
      }
   }

   (void)unused;
}



/* tim_win32_low_perf_thread: [timer thread]
 *  Thread loop function for the low performance driver.
 */
static void tim_win32_low_perf_thread(_AL_THREAD *self, void *unused)
{
   DWORD result;
   usecs_t delay = 0x8000;
   DWORD prev_time;  
   DWORD curr_time;  /* in milliseconds */
   DWORD diff_time;

   /* init thread */
   _win_thread_init();

   /* get initial time */
   prev_time = timeGetTime();

   while (true) {
      _al_mutex_lock(&timer_thread_mutex);
      {
#if 0
         if (!app_foreground) {
            /* restart time if the thread was blocked */
            if (thread_switch_out())
               prev_time = timeGetTime();
         }
#endif

         /* get current time */
         curr_time = timeGetTime();

         /* get time till next tick */
         diff_time = curr_time - prev_time;

         /* save current time for next tick */
         prev_time = curr_time;

         /* call timer procs */
         delay = timer_thread_handle_tick(MSEC_TO_USEC(diff_time));
      }
      _al_mutex_unlock(&timer_thread_mutex);

      /* wait calculated time */
      result = WaitForSingleObject(timer_stop_event, USEC_TO_MSEC(delay));
      if (result != WAIT_TIMEOUT) {
         _win_thread_exit();
	 return;
      }
   }

   (void)unused;
}



/* tim_win32_high_perf_init: [primary thread]
 *  Initializes the high performance driver.
 */
static bool tim_win32_high_perf_init(void)
{
   if (!QueryPerformanceFrequency(&counter_freq)) {
      _TRACE(PREFIX_W "High performance timer not supported\n");
      return false;
   }

   /* create thread termination event */
   timer_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   _al_mutex_init(&timer_thread_mutex);

   /* start timer thread */
   _al_thread_create(&timer_thread, tim_win32_high_perf_thread, NULL);

   /* increase priority of timer thread */
   SetThreadPriority(timer_thread.thread, THREAD_PRIORITY_TIME_CRITICAL);

   return true;
}



/* tim_win32_low_perf_init: [primary thread]
 *  Initializes the low performance driver.
 */
static bool tim_win32_low_perf_init(void)
{
   /* create thread termination event */
   timer_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   _al_mutex_init(&timer_thread_mutex);

   /* start timer thread */
   _al_thread_create(&timer_thread, tim_win32_low_perf_thread, NULL);

   /* increase priority of timer thread */
   SetThreadPriority(timer_thread.thread, THREAD_PRIORITY_TIME_CRITICAL);

   return true;
}



/* tim_win32_exit: [primary thread]
 *  Shuts down either timer driver.
 */
static void tim_win32_exit(void)
{
   /* acknowledge that the thread should stop */
   SetEvent(timer_stop_event);

   /* wait until thread has ended */
   _al_thread_join(&timer_thread);

   /* thread has ended, now we can release all resources */
   _al_mutex_destroy(&timer_thread_mutex);
   CloseHandle(timer_stop_event);
}



/* timer_thread_handle_tick: [timer thread]
 *  Call handle_tick() method of every timer in active_timers, and
 *  returns the duration that the timer thread should try to sleep
 *  next time.
 */
static usecs_t timer_thread_handle_tick(usecs_t interval)
{
   usecs_t new_delay = 0x8000;
   unsigned int i;

   for (i = 0; i < _al_vector_size(&active_timers); i++) {
      AL_TIMER **slot = _al_vector_ref(&active_timers, i);
      AL_TIMER *timer = *slot;

      timer->counter -= interval;

      while (timer->counter <= 0) {
         timer->counter += timer->speed_usecs;
	 timer_handle_tick(timer);
      }

      if ((timer->counter > 0) && (timer->counter < new_delay))
         new_delay = timer->counter;
   }

   return new_delay;
}



/*
 * Timer objects
 */


/* al_install_timer: [primary thread]
 *  Create a new timer object.
 */
AL_TIMER* al_install_timer(msecs_t speed_msecs)
{
   ASSERT(speed_msecs > 0);
   {
      AL_TIMER *timer = _AL_MALLOC(sizeof *timer);

      ASSERT(timer);

      if (timer) {
         _al_event_source_init(&timer->es);
         timer->started = false;
         timer->count = 0;
         timer->speed_usecs = speed_msecs * 1000;
         timer->counter = 0;

         _al_register_destructor(timer, (void (*)(void *)) al_uninstall_timer);
      }

      return timer;
   }
}



/* al_uninstall_timer: [primary thread]
 *  Destroy this timer object.
 */
void al_uninstall_timer(AL_TIMER *this)
{
   ASSERT(this);

   al_stop_timer(this);

   _al_unregister_destructor(this);

   _al_event_source_free(&this->es);
   _AL_FREE(this);
}



/* al_start_timer: [primary thread]
 *  Start this timer.  If it is the first started timer, the
 *  background timer thread is subsequently started.
 */
void al_start_timer(AL_TIMER *this)
{
   ASSERT(this);
   {
      size_t new_size;

      if (this->started)
         return;

      _al_mutex_lock(&timer_thread_mutex);
      {
         AL_TIMER **slot;

         this->started = true;
         this->counter = this->speed_usecs;

         slot = _al_vector_alloc_back(&active_timers);
         *slot = this;

         new_size = _al_vector_size(&active_timers);
      }
      _al_mutex_unlock(&timer_thread_mutex);

      if (new_size == 1) {
         if (!tim_win32_high_perf_init())
            tim_win32_low_perf_init();
      }
   }
}



/* al_stop_timer: [primary thread]
 *  Stop this timer.  If it is the last started timer, the background
 *  timer thread is subsequently stopped.
 */
void al_stop_timer(AL_TIMER *this)
{
   ASSERT(this);
   {
      size_t new_size;

      if (!this->started)
         return;

      _al_mutex_lock(&timer_thread_mutex);
      {
         _al_vector_find_and_delete(&active_timers, &this);
         this->started = false;

         new_size = _al_vector_size(&active_timers);
      }
      _al_mutex_unlock(&timer_thread_mutex);

      if (new_size == 0) {
         tim_win32_exit();
         _al_vector_free(&active_timers);
      }
   }
}



/* al_timer_is_started: [primary thread]
 *  Return if this timer is started.
 */
bool al_timer_is_started(AL_TIMER *this)
{
   ASSERT(this);

   return this->started;
}



/* al_timer_get_speed: [primary thread]
 *  Return this timer's speed.
 */
msecs_t al_timer_get_speed(AL_TIMER *this)
{
   ASSERT(this);

   return this->speed_usecs / 1000;
}



/* al_timer_set_speed: [primary thread]
 *  Change this timer's speed.
 */
void al_timer_set_speed(AL_TIMER *this, msecs_t new_speed_msecs)
{
   ASSERT(this);
   ASSERT(new_speed_msecs > 0);

   _al_mutex_lock(&timer_thread_mutex);
   {
      if (this->started) {
         this->counter -= this->speed_usecs;
         this->counter += new_speed_msecs * 1000;
      }

      this->speed_usecs = new_speed_msecs * 1000;
   }
   _al_mutex_unlock(&timer_thread_mutex);
}



/* al_timer_get_count: [primary thread]
 *  Return this timer's count.
 */
long al_timer_get_count(AL_TIMER *this)
{
   ASSERT(this);

   return this->count;
}



/* al_timer_set_count: [primary thread]
 *  Change this timer's count.
 */
void al_timer_set_count(AL_TIMER *this, long new_count)
{
   ASSERT(this);

   _al_mutex_lock(&timer_thread_mutex);
   {
      this->count = new_count;
   }
   _al_mutex_unlock(&timer_thread_mutex);
}



/* timer_handle_tick: [timer thread]
 *  Handle a single tick.
 */
static void timer_handle_tick(AL_TIMER *this)
{
   /* Lock out event source helper functions (e.g. the release hook
    * could be invoked simultaneously with this function).
    */
   _al_event_source_lock(&this->es);
   {
      /* Update the count.  */
      this->count++;

      /* Generate an event, maybe.  */
      if (_al_event_source_needs_to_generate_event(&this->es)) {
         AL_EVENT *event = _al_event_source_get_unused_event(&this->es);
         if (event) {
            event->timer.type = AL_EVENT_TIMER;
            event->timer.timestamp = al_current_time();
            event->timer.count = this->count;
            _al_event_source_emit_event(&this->es, event);
         }
      }
   }
   _al_event_source_unlock(&this->es);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
