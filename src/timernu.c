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
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_events.h"

#ifndef ALLEGRO_MSVC
   #include <sys/time.h>
#endif


/* forward declarations */
static double timer_thread_handle_tick(double interval);
static void timer_handle_tick(ALLEGRO_TIMER *this);


struct ALLEGRO_TIMER
{
   ALLEGRO_EVENT_SOURCE es;
   bool started;
   double speed_secs;
   long count;
   double counter;		/* counts down to zero=blastoff */
};



/*
 * The timer thread that runs in the background to drive the timers.
 */

static _AL_THREAD timer_thread;
static _AL_MUTEX timer_thread_mutex = _AL_MUTEX_UNINITED;
static _AL_VECTOR active_timers = _AL_VECTOR_INITIALIZER(ALLEGRO_TIMER *);


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

   double old_time = al_current_time();
   double new_time;
   double interval = 0.032768;

   while (!_al_thread_should_stop(self)) {
      al_rest(interval);

      _al_mutex_lock(&timer_thread_mutex);
      {
         /* Calculate actual time elapsed.  */
         new_time = al_current_time();
         interval = new_time - old_time;
         old_time = new_time;

         /* Handle a tick.  */
         interval = timer_thread_handle_tick(interval);
      }
      _al_mutex_unlock(&timer_thread_mutex);
   }
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
         timer->counter += timer->speed_secs;
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
ALLEGRO_TIMER* al_install_timer(double speed_secs)
{
   ASSERT(speed_secs > 0);
   {
      ALLEGRO_TIMER *timer = _AL_MALLOC(sizeof *timer);

      ASSERT(timer);

      if (timer) {
         _al_event_source_init(&timer->es);
         timer->started = false;
         timer->count = 0;
         timer->speed_secs = speed_secs;
         timer->counter = 0;

         _al_register_destructor(timer, (void (*)(void *)) al_uninstall_timer);
      }

      return timer;
   }
}



/* al_uninstall_timer: [primary thread]
 *  Destroy this timer object.
 */
void al_uninstall_timer(ALLEGRO_TIMER *this)
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
void al_start_timer(ALLEGRO_TIMER *this)
{
   ASSERT(this);
   {
      size_t new_size;

      if (this->started)
         return;

      _al_mutex_lock(&timer_thread_mutex);
      {
         ALLEGRO_TIMER **slot;

         this->started = true;
         this->counter = this->speed_secs;

         slot = _al_vector_alloc_back(&active_timers);
         *slot = this;

         new_size = _al_vector_size(&active_timers);
      }
      _al_mutex_unlock(&timer_thread_mutex);

      if (new_size == 1) {
         _al_mutex_init(&timer_thread_mutex);
         _al_thread_create(&timer_thread, timer_thread_proc, NULL);
      }
   }
}



/* al_stop_timer: [primary thread]
 *  Stop this timer.  If it is the last started timer, the background
 *  timer thread is subsequently stopped.
 */
void al_stop_timer(ALLEGRO_TIMER *this)
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
         _al_thread_join(&timer_thread);
         _al_mutex_destroy(&timer_thread_mutex);
         _al_vector_free(&active_timers);
      }
   }
}



/* al_timer_is_started: [primary thread]
 *  Return if this timer is started.
 */
bool al_timer_is_started(ALLEGRO_TIMER *this)
{
   ASSERT(this);

   return this->started;
}



/* al_get_timer_speed: [primary thread]
 *  Return this timer's speed.
 */
double al_get_timer_speed(ALLEGRO_TIMER *this)
{
   ASSERT(this);

   return this->speed_secs;
}



/* al_set_timer_speed: [primary thread]
 *  Change this timer's speed.
 */
void al_set_timer_speed(ALLEGRO_TIMER *this, double new_speed_secs)
{
   ASSERT(this);
   ASSERT(new_speed_secs > 0);

   _al_mutex_lock(&timer_thread_mutex);
   {
      if (this->started) {
         this->counter -= this->speed_secs;
         this->counter += new_speed_secs;
      }

      this->speed_secs = new_speed_secs;
   }
   _al_mutex_unlock(&timer_thread_mutex);
}



/* al_get_timer_count: [primary thread]
 *  Return this timer's count.
 */
long al_get_timer_count(ALLEGRO_TIMER *this)
{
   ASSERT(this);

   return this->count;
}



/* al_set_timer_count: [primary thread]
 *  Change this timer's count.
 */
void al_set_timer_count(ALLEGRO_TIMER *this, long new_count)
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
static void timer_handle_tick(ALLEGRO_TIMER *this)
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
         ALLEGRO_EVENT *event = _al_event_source_get_unused_event(&this->es);
         if (event) {
            event->timer.type = ALLEGRO_EVENT_TIMER;
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
