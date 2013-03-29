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
static double timer_thread_handle_tick(double interval);
static void timer_handle_tick(ALLEGRO_TIMER *timer);


struct ALLEGRO_TIMER
{
   ALLEGRO_EVENT_SOURCE es;
   bool started;
   double speed_secs;
   int64_t count;
   double counter;		/* counts down to zero=blastoff */
};



/*
 * The timer thread that runs in the background to drive the timers.
 */

static _AL_MUTEX timers_mutex = _AL_MUTEX_UNINITED;
static _AL_VECTOR active_timers = _AL_VECTOR_INITIALIZER(ALLEGRO_TIMER *);
static _AL_THREAD * volatile timer_thread = NULL;



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

#ifdef ALLEGRO_QNX
   /* thread priority adjustment for QNX:
    *  The timer thread is set to the highest relative priority.
    *  (see the comment in src/qnx/qsystem.c about the scheduling policy)
    */
   {
      struct sched_param sparam;
      int spolicy;

      if (pthread_getschedparam(pthread_self(), &spolicy, &sparam) == EOK) {
         sparam.sched_priority += 4;
         pthread_setschedparam(pthread_self(), spolicy, &sparam);
      }
   }
#endif

   double old_time = al_get_time();
   double new_time;
   double interval = 0.032768;

   while (!_al_get_thread_should_stop(self)) {
      al_rest(interval);

      _al_mutex_lock(&timers_mutex);
      {
         /* Calculate actual time elapsed.  */
         new_time = al_get_time();
         interval = new_time - old_time;
         old_time = new_time;

         /* Handle a tick.  */
         interval = timer_thread_handle_tick(interval);
      }
      _al_mutex_unlock(&timers_mutex);
   }

   (void)unused;
}



/* timer_thread_handle_tick: [timer thread]
 *  Call handle_tick() method of every timer in active_timers, and
 *  returns the duration that the timer thread should try to sleep
 *  next time.
 */
static double timer_thread_handle_tick(double interval)
{
   double new_delay = 0.032768;
   unsigned int i;

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
   ASSERT(timer_thread == NULL);

   _al_mutex_destroy(&timers_mutex);
}



void _al_init_timers(void)
{
   _al_mutex_init(&timers_mutex);
   _al_add_exit_func(shutdown_timers, "shutdown_timers");
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

         _al_register_destructor(_al_dtor_list, timer,
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

      _al_unregister_destructor(_al_dtor_list, timer);

      _al_event_source_free(&timer->es);
      al_free(timer);
   }
}



/* Function: al_start_timer
 */
void al_start_timer(ALLEGRO_TIMER *timer)
{
   ASSERT(timer);
   {
      size_t new_size;

      if (timer->started)
         return;

      _al_mutex_lock(&timers_mutex);
      {
         ALLEGRO_TIMER **slot;

         timer->started = true;
         timer->counter = timer->speed_secs;

         slot = _al_vector_alloc_back(&active_timers);
         *slot = timer;

         new_size = _al_vector_size(&active_timers);
      }
      _al_mutex_unlock(&timers_mutex);

      if (new_size == 1) {
         timer_thread = al_malloc(sizeof(_AL_THREAD));
         _al_thread_create(timer_thread, timer_thread_proc, NULL);
      }
   }
}



/* Function: al_stop_timer
 */
void al_stop_timer(ALLEGRO_TIMER *timer)
{
   ASSERT(timer);
   {
      _AL_THREAD *thread_to_join = NULL;

      if (!timer->started)
         return;

      _al_mutex_lock(&timers_mutex);
      {
         _al_vector_find_and_delete(&active_timers, &timer);
         timer->started = false;

         if (_al_vector_size(&active_timers) == 0) {
            _al_vector_free(&active_timers);
            thread_to_join = timer_thread;
            timer_thread = NULL;
         }
      }
      _al_mutex_unlock(&timers_mutex);

      if (thread_to_join) {
         _al_thread_join(thread_to_join);
         al_free(thread_to_join);
      }
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

   _al_mutex_lock(&timers_mutex);
   {
      if (timer->started) {
         timer->counter -= timer->speed_secs;
         timer->counter += new_speed_secs;
      }

      timer->speed_secs = new_speed_secs;
   }
   _al_mutex_unlock(&timers_mutex);
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

   _al_mutex_lock(&timers_mutex);
   {
      timer->count = new_count;
   }
   _al_mutex_unlock(&timers_mutex);
}



/* Function: al_add_timer_count
 */
void al_add_timer_count(ALLEGRO_TIMER *timer, int64_t diff)
{
   ASSERT(timer);

   _al_mutex_lock(&timers_mutex);
   {
      timer->count += diff;
   }
   _al_mutex_unlock(&timers_mutex);
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
