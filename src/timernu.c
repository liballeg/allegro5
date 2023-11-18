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
 *      New timer API.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_timer.h"

#ifndef ALLEGRO_MSVC
#ifndef ALLEGRO_BCC32
   #include <sys/time.h>
#endif
#endif


/* forward declarations */
static void timer_handle_tick(ALLEGRO_TIMER *timer);


struct ALLEGRO_TIMER
{
   ALLEGRO_EVENT_SOURCE es;
   bool started;
   double speed_secs;
   int64_t count;
   double counter;		/* counts down to zero=blastoff */
   _AL_LIST_ITEM *dtor_item;
};



/*
 * The timer thread that runs in the background to drive the timers.
 */

static ALLEGRO_MUTEX *timers_mutex;
static _AL_VECTOR active_timers = _AL_VECTOR_INITIALIZER(ALLEGRO_TIMER *);
static _AL_THREAD * volatile timer_thread = NULL;
static ALLEGRO_COND *timer_cond = NULL;
static bool destroy_thread = false;

// Allegro's al_get_time measures "the time since Allegro started", and so
// does not ignore time spent in a suspended state. Further, some implementations
// currently use a calendar clock, which changes based on the system clock.
// However, the timer control thread needs to ignore suspended time, so that it always
// performs the correct number of timer ticks.
// This has only been verified for Allegro, so for now fallback to old
// behavior on other platforms. `_al_timer_thread_handle_tick` will bound
// the interval to a reasonable value to prevent other platforms from behaving
// poorly when the clock changes.
// See https://github.com/liballeg/allegro5/issues/1510
// Note: perhaps this could move into a new public API: al_get_uptime

#if defined(ALLEGRO_MACOSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
#define USE_UPTIME
struct timespec _al_initial_uptime;
#endif

static void _init_get_uptime()
{
#ifdef USE_UPTIME
   clock_gettime(CLOCK_UPTIME_RAW, &_al_initial_uptime);
#endif
}

static double _al_get_uptime()
{
#ifdef USE_UPTIME
   struct timespec now;
   double time;

   clock_gettime(CLOCK_UPTIME_RAW, &now);
   time = (double) (now.tv_sec - _al_initial_uptime.tv_sec)
      + (double) (now.tv_nsec - _al_initial_uptime.tv_nsec) * 1.0e-9;
   return time;
#else
   return al_get_time();
#endif
}


/* timer_thread_proc: [timer thread]
 *  The timer thread procedure itself.
 */
static void timer_thread_proc(_AL_THREAD *self, void *unused)
{
#if 0
   /* Block all signals.  */
   /* This was needed for some reason in v4, but I can't remember why,
    * and it might not be relevant now.  It has a tendency to create
    * zombie processes if the program is aborted abnormally, so I'm
    * taking it out for now.
    */
   {
      sigset_t mask;
      sigfillset(&mask);
      pthread_sigmask(SIG_BLOCK, &mask, NULL);
   }
#endif

   _init_get_uptime();
   double old_time = _al_get_uptime();
   double new_time;
   double interval = 0.032768;

   while (!_al_get_thread_should_stop(self)) {
      al_lock_mutex(timers_mutex);
      while (_al_vector_size(&active_timers) == 0 && !destroy_thread) {
         al_wait_cond(timer_cond, timers_mutex);
         old_time = _al_get_uptime() - interval;
      }
      al_unlock_mutex(timers_mutex);

      al_rest(interval);

      al_lock_mutex(timers_mutex);
      {
         /* Calculate actual time elapsed.  */
         new_time = _al_get_uptime();
         interval = new_time - old_time;
         old_time = new_time;

         /* Handle a tick.  */
         interval = _al_timer_thread_handle_tick(interval);
      }
      al_unlock_mutex(timers_mutex);
   }

   (void)unused;
}



/* timer_thread_handle_tick: [timer thread]
 *  Call handle_tick() method of every timer in active_timers, and
 *  returns the duration that the timer thread should try to sleep
 *  next time.
 */
double _al_timer_thread_handle_tick(double interval)
{
   double new_delay = 0.032768;
   unsigned int i;

   /* Never allow negative time, or greater than 10 seconds delta.
    * This is to handle clock changes on platforms not using a monotonic,
    * suspense-free clock.
    */
   interval = _ALLEGRO_CLAMP(0, interval, 10.0);

   for (i = 0; i < _al_vector_size(&active_timers); i++) {
      ALLEGRO_TIMER **slot = _al_vector_ref(&active_timers, i);
      ALLEGRO_TIMER *timer = *slot;

      timer->counter -= interval;

      while (timer->counter <= 0) {
         timer_handle_tick(timer);
         timer->counter += timer->speed_secs;
      }

      if ((timer->counter > 0) && (timer->counter < new_delay))
         new_delay = timer->counter;
   }

   return new_delay;
}



static void shutdown_timers(void)
{
   ASSERT(_al_vector_size(&active_timers) == 0);

   if (timer_thread != NULL) {
      al_lock_mutex(timers_mutex);
      _al_vector_free(&active_timers);
      destroy_thread = true;
      al_signal_cond(timer_cond);
      al_unlock_mutex(timers_mutex);
      _al_thread_join(timer_thread);
   }
   else {
      _al_vector_free(&active_timers);
   }

   al_free(timer_thread);

   timer_thread = NULL;

   al_destroy_mutex(timers_mutex);

   al_destroy_cond(timer_cond);
}



// logic common to al_start_timer and al_resume_timer
// al_start_timer : passes reset_counter = true to start from the beginning
// al_resume_timer: passes reset_counter = false to preserve the previous time
static void enable_timer(ALLEGRO_TIMER *timer, bool reset_counter)
{
   ASSERT(timer);
   {
      if (timer->started)
         return;

      al_lock_mutex(timers_mutex);
      {
         ALLEGRO_TIMER **slot;

         timer->started = true;

         if (reset_counter)
            timer->counter = timer->speed_secs;

         slot = _al_vector_alloc_back(&active_timers);
         *slot = timer;

         al_signal_cond(timer_cond);
      }
      al_unlock_mutex(timers_mutex);

      if (timer_thread == NULL) {
         destroy_thread = false;
         timer_thread = al_malloc(sizeof(_AL_THREAD));
         _al_thread_create(timer_thread, timer_thread_proc, NULL);
      }
   }
}



void _al_init_timers(void)
{
   timers_mutex = al_create_mutex();
   timer_cond = al_create_cond();
   _al_add_exit_func(shutdown_timers, "shutdown_timers");
}



int _al_get_active_timers_count(void)
{
   return _al_vector_size(&active_timers);
}


/*
 * Timer objects
 */


/* Function: al_create_timer
 */
ALLEGRO_TIMER *al_create_timer(double speed_secs)
{
   ASSERT(speed_secs > 0);
   {
      ALLEGRO_TIMER *timer = al_malloc(sizeof *timer);

      ASSERT(timer);

      if (timer) {
         _al_event_source_init(&timer->es);
         timer->started = false;
         timer->count = 0;
         timer->speed_secs = speed_secs;
         timer->counter = 0;

         timer->dtor_item = _al_register_destructor(_al_dtor_list, "timer", timer,
            (void (*)(void *)) al_destroy_timer);
      }

      return timer;
   }
}



/* Function: al_destroy_timer
 */
void al_destroy_timer(ALLEGRO_TIMER *timer)
{
   if (timer) {
      al_stop_timer(timer);

      _al_unregister_destructor(_al_dtor_list, timer->dtor_item);

      _al_event_source_free(&timer->es);
      al_free(timer);
   }
}



/* Function: al_start_timer
 */
void al_start_timer(ALLEGRO_TIMER *timer)
{
   enable_timer(timer, true); // true to reset the counter
}



/* Function: al_resume_timer
 */
void al_resume_timer(ALLEGRO_TIMER *timer)
{
   enable_timer(timer, false); // false to preserve the counter
}



/* Function: al_stop_timer
 */
void al_stop_timer(ALLEGRO_TIMER *timer)
{
   ASSERT(timer);
   {
      if (!timer->started)
         return;

      al_lock_mutex(timers_mutex);
      {
         _al_vector_find_and_delete(&active_timers, &timer);
         timer->started = false;
      }
      al_unlock_mutex(timers_mutex);
   }
}



/* Function: al_get_timer_started
 */
bool al_get_timer_started(const ALLEGRO_TIMER *timer)
{
   ASSERT(timer);

   return timer->started;
}



/* Function: al_get_timer_speed
 */
double al_get_timer_speed(const ALLEGRO_TIMER *timer)
{
   ASSERT(timer);

   return timer->speed_secs;
}



/* Function: al_set_timer_speed
 */
void al_set_timer_speed(ALLEGRO_TIMER *timer, double new_speed_secs)
{
   ASSERT(timer);
   ASSERT(new_speed_secs > 0);

   al_lock_mutex(timers_mutex);
   {
      if (timer->started) {
         timer->counter -= timer->speed_secs;
         timer->counter += new_speed_secs;
      }

      timer->speed_secs = new_speed_secs;
   }
   al_unlock_mutex(timers_mutex);
}



/* Function: al_get_timer_count
 */
int64_t al_get_timer_count(const ALLEGRO_TIMER *timer)
{
   ASSERT(timer);

   return timer->count;
}



/* Function: al_set_timer_count
 */
void al_set_timer_count(ALLEGRO_TIMER *timer, int64_t new_count)
{
   ASSERT(timer);

   al_lock_mutex(timers_mutex);
   {
      timer->count = new_count;
   }
   al_unlock_mutex(timers_mutex);
}



/* Function: al_add_timer_count
 */
void al_add_timer_count(ALLEGRO_TIMER *timer, int64_t diff)
{
   ASSERT(timer);

   al_lock_mutex(timers_mutex);
   {
      timer->count += diff;
   }
   al_unlock_mutex(timers_mutex);
}


/* timer_handle_tick: [timer thread]
 *  Handle a single tick.
 */
static void timer_handle_tick(ALLEGRO_TIMER *timer)
{
   /* Lock out event source helper functions (e.g. the release hook
    * could be invoked simultaneously with this function).
    */
   _al_event_source_lock(&timer->es);
   {
      /* Update the count.  */
      timer->count++;

      /* Generate an event, maybe.  */
      if (_al_event_source_needs_to_generate_event(&timer->es)) {
         ALLEGRO_EVENT event;
         event.timer.type = ALLEGRO_EVENT_TIMER;
         event.timer.timestamp = al_get_time();
         event.timer.count = timer->count;
         event.timer.error = -timer->counter;
         _al_event_source_emit_event(&timer->es, &event);
      }
   }
   _al_event_source_unlock(&timer->es);
}



/* Function: al_get_timer_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_timer_event_source(ALLEGRO_TIMER *timer)
{
   return &timer->es;
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
