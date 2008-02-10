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
 *      Old timer interrupt routines emulation.
 *
 *      By Shawn Hargreaves.
 *
 *      Synchronization added by Eric Botcazou.
 *
 *      Converted into an emulation layer by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"



#define TIMER_TO_MSEC(x)  ((long)((x) / 1000))

/* list of active timer handlers */
typedef struct TIMER_QUEUE
{
   ALLEGRO_TIMER *timer;
   AL_METHOD(void, proc, (void));      /* timer handler functions */
   AL_METHOD(void, param_proc, (void *param));
   void *param;                        /* param for param_proc if used */
} TIMER_QUEUE;


TIMER_DRIVER *timer_driver = NULL;        /* the active driver */
                                          /* (a dummy in this emulation) */

int _timer_installed = FALSE;

static TIMER_QUEUE _timer_queue[MAX_TIMERS]; /* list of active callbacks */

volatile int retrace_count = 0;           /* used for retrace syncing */
void (*retrace_proc)(void) = NULL;

long _vsync_speed = BPS_TO_TIMER(70);     /* retrace speed */

int _timer_use_retrace = FALSE;           /* are we synced to the retrace? */

volatile int _retrace_hpp_value = -1;     /* to set during next retrace */

static _AL_THREAD timer_thread;           /* the timer thread */
static _AL_MUTEX timer_mutex = _AL_MUTEX_UNINITED; /* global timer mutex */

static ALLEGRO_EVENT_QUEUE *event_queue;       /* event queue to collect timer events */
static ALLEGRO_TIMER *retrace_timer;           /* timer to simulate retrace counting */



/* a dummy driver */
static TIMER_DRIVER timerdrv_emu =
{
   AL_ID('T','E','M','U'),
   empty_string,
   empty_string,
   "Emulated timers"
};



/* rest_callback:
 *  Waits for time milliseconds.
 *  Note: this can wrap if the process runs for > ~49 days.
 */
void rest_callback(unsigned int time, void (*callback)(void))
{
   double dt = ALLEGRO_MSECS_TO_SECS((double) time);
   if (callback) {
      double end = al_current_time() + dt;

      while (al_current_time() < end)
         callback();
   }
   else {
      al_rest(dt);
   }
}



/* rest:
 *  Waits for time milliseconds.
 */
void rest(unsigned int time)
{
   al_rest(ALLEGRO_MSECS_TO_SECS((double) time));
}



/* timer_can_simulate_retrace: [deprecated but used internally]
 *  Checks whether the current driver is capable of a video retrace
 *  syncing mode.
 */
int timer_can_simulate_retrace()
{
   return FALSE;
}



/* timer_simulate_retrace: [deprecated but used internally]
 *  Turns retrace simulation on or off, and if turning it on, calibrates
 *  the retrace timer.
 */
void timer_simulate_retrace(int enable)
{
}



/* timer_is_using_retrace: [deprecated]
 *  Tells the user whether the current driver is providing a retrace
 *  sync.
 */
int timer_is_using_retrace()
{
   return FALSE;
}



/* timer_thread_func: [timer thread]
 *  Each "interrupt" callback registered by the user has an associated
 *  ALLEGRO_TIMER object. All these objects are registered to a global event
 *  queue. This function runs in a background thread and reads timer events
 *  from the event queue, calling the appropriate callbacks.
 */
static void timer_thread_func(_AL_THREAD *self, void *unused)
{
   while (!_al_thread_should_stop(self)) {
      ALLEGRO_EVENT event;

      if (!al_wait_for_event(event_queue, &event, 50))
         continue;

      if ((ALLEGRO_TIMER *)event.any.source == retrace_timer) {
         retrace_count++;

         /* retrace_proc is just a bad idea -- don't use it! */
         if (retrace_proc)
            retrace_proc();
      }
      else {
         bool found = false;
         TIMER_QUEUE copy;
         int x;

         memset(&copy, 0, sizeof(copy));

         /* We delay the call until the timer_mutex is unlocked, to
          * avoid deadlocks. The callback itself can add or remove
          * timers. Using a recursive mutex isn't enough either.
          *
          * FIXME: There is a problem with this approach. If
          * remove_int() is called from another thread while we are in
          * the middle of the lock...unlock, the proc being removed
          * may run one more time after it was supposed to be removed!
          * This can be troublesome if shortly after remove_int() some
          * resource needed by the proc is freed up (hopefully rare!).
          */

         _al_mutex_lock(&timer_mutex);
         {
            for (x=0; x<MAX_TIMERS; x++) {
               if (_timer_queue[x].timer == (ALLEGRO_TIMER *)event.any.source) {
                  copy = _timer_queue[x];
                  found = true;
                  break;
               }
            }
         }
         _al_mutex_unlock(&timer_mutex);

         if (found) {
            if (copy.param_proc)
               copy.param_proc(copy.param);
            else
               copy.proc();
         }
      }
   }
}



/* find_timer_slot:
 *  Searches the list of user timer callbacks for a specified function, 
 *  returning the position at which it was found, or -1 if it isn't there.
 */
static int find_timer_slot(void (*proc)(void))
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if (_timer_queue[x].proc == proc)
	 return x;

   return -1;
}



/* find_param_timer_slot:
 *  Searches the list of user timer callbacks for a specified paramater 
 *  function, returning the position at which it was found, or -1 if it 
 *  isn't there.
 */
static int find_param_timer_slot(void (*proc)(void *param), void *param)
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if ((_timer_queue[x].param_proc == proc) && (_timer_queue[x].param == param))
	 return x;

   return -1;
}



/* find_empty_timer_slot:
 *  Searches the list of user timer callbacks for an empty slot.
 */
static int find_empty_timer_slot(void)
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if ((!_timer_queue[x].proc) && (!_timer_queue[x].param_proc))
	 return x;

   return -1;
}



/* install_timer_int:
 *  Installs a function into the list of user timers, or if it is already 
 *  installed, adjusts its speed. This function will be called once every 
 *  speed msecs. Returns a negative number if there was no room to 
 *  add a new routine.
 */
static int install_timer_int(void *proc, void *param, long speed_msecs, int param_used)
{
   int x;

   if (!timer_driver) {                   /* make sure we are installed */
      if (install_timer() != 0)
	 return -1;
   }

   if (param_used)
      x = find_param_timer_slot((void (*)(void *))proc, param);
   else
      x = find_timer_slot((void (*)(void))proc); 

   if (x < 0)
      x = find_empty_timer_slot();

   if (x < 0)
      return -1;

   _al_mutex_lock(&timer_mutex);
   {
      if ((proc == _timer_queue[x].proc) || (proc == _timer_queue[x].param_proc)) {
         al_set_timer_speed(_timer_queue[x].timer, speed_msecs);
      }
      else {
         _timer_queue[x].timer = al_install_timer(speed_msecs);
         if (param_used) {
            _timer_queue[x].param = param;
            _timer_queue[x].param_proc = (void (*)(void *))proc;
         }
         else
            _timer_queue[x].proc = (void (*)(void))proc;

         al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)_timer_queue[x].timer);
         al_start_timer(_timer_queue[x].timer);
      }
   }
   _al_mutex_unlock(&timer_mutex);

   return 0;
}



/* install_int:
 *  Wrapper for install_timer_int, which takes the speed in milliseconds,
 *  no paramater.
 */
int install_int(void (*proc)(void), long speed)
{
   return install_timer_int((void *)proc, NULL, speed, FALSE);
}



/* install_int_ex:
 *  Wrapper for install_timer_int, which takes the speed in timer ticks,
 *  no paramater.
 */
int install_int_ex(void (*proc)(void), long speed)
{
   return install_timer_int((void *)proc, NULL, TIMER_TO_MSEC(speed), FALSE);
}



/* install_param_int:
 *  Wrapper for install_timer_int, which takes the speed in milliseconds,
 *  with paramater.
 */
int install_param_int(void (*proc)(void *param), void *param, long speed)
{
   return install_timer_int((void *)proc, param, speed, TRUE);
}



/* install_param_int_ex:
 *  Wrapper for install_timer_int, which takes the speed in timer ticks,
 *  no paramater.
 */
int install_param_int_ex(void (*proc)(void *param), void *param, long speed)
{
   return install_timer_int((void *)proc, param, TIMER_TO_MSEC(speed), TRUE);
}



/* remove_timer_int:
 *  Removes a function from the list of user timers.
 */
static void remove_timer_int(void *proc, void *param, int param_used)
{
   int x;

   if (param_used)
      x = find_param_timer_slot((void (*)(void *))proc, param);
   else
      x = find_timer_slot((void (*)(void))proc); 

   if (x < 0)
      return;

   _al_mutex_lock(&timer_mutex);
   {
      al_uninstall_timer(_timer_queue[x].timer);
      _timer_queue[x].timer = NULL;
      _timer_queue[x].proc = NULL;
      _timer_queue[x].param_proc = NULL;
      _timer_queue[x].param = NULL;
   }
   _al_mutex_unlock(&timer_mutex);
}



/* remove_int:
 *  Wrapper for remove_timer_int, without parameters.
 */
void remove_int(void (*proc)(void))
{
   remove_timer_int((void *)proc, NULL, FALSE);
}



/* remove_param_int:
 *  Wrapper for remove_timer_int, with parameters.
 */
void remove_param_int(void (*proc)(void *param), void *param)
{
   remove_timer_int((void *)proc, param, TRUE);
}



/* clear_timer_queue:
 *  Clears the timer queue.
 */
static void clear_timer_queue(void)
{
   int i;

   for (i=0; i<MAX_TIMERS; i++) {
      if (_timer_queue[i].timer) {
         al_uninstall_timer(_timer_queue[i].timer);
         _timer_queue[i].timer = NULL;
      }
      _timer_queue[i].proc = NULL;
      _timer_queue[i].param_proc = NULL;
      _timer_queue[i].param = NULL;
   }
}



/* install_timer:
 *  Installs the timer interrupt handler. You must do this before installing
 *  any user timer routines. You must set up the timer before trying to 
 *  display a mouse pointer or using any of the GUI routines.
 */
int install_timer()
{
   if (timer_driver)
      return 0;

   timer_driver = &timerdrv_emu;

   clear_timer_queue();

   _al_mutex_init(&timer_mutex);

   event_queue = al_create_event_queue();
   retrace_timer = al_install_timer(TIMER_TO_MSEC(_vsync_speed));
   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)retrace_timer);

   /* start timer thread */
   _al_thread_create(&timer_thread, timer_thread_func, NULL);

#ifdef ALLEGRO_WINDOWS
   /* increase priority of timer thread */
   SetThreadPriority(timer_thread.thread, THREAD_PRIORITY_TIME_CRITICAL);
#endif

   al_start_timer(retrace_timer);

   _add_exit_func(remove_timer, "remove_timer");
   _timer_installed = TRUE;

   return 0;
}



/* remove_timer:
 *  Removes our timer handler and resets the BIOS clock. You don't normally
 *  need to call this, because allegro_exit() will do it for you.
 */
void remove_timer(void)
{
   if (!timer_driver)
      return;

   _al_thread_join(&timer_thread);

   al_uninstall_timer(retrace_timer);
   retrace_timer = NULL;

   al_destroy_event_queue(event_queue);
   event_queue = NULL;

   _al_mutex_destroy(&timer_mutex);

   /* uninstall existing timers and */
   /* make sure subsequent remove_int() calls don't crash */
   clear_timer_queue();

   _remove_exit_func(remove_timer);
   _timer_installed = FALSE;

   timer_driver = NULL;
}



/* This section is to allow src/linux/vtswitch.c to suspend the timer
 * emulation subsystem when switching away in SWITCH_PAUSE or
 * SWITCH_AMNESIA modes.  It used to do it by calling TIMER_DRIVER
 * init() and exit() methods directly...
 */

#ifdef ALLEGRO_LINUX

void _al_suspend_old_timer_emulation(void)
{
   TRACE("_al_suspend_old_timer_emulation called\n");

   if (!timer_driver)
      return;

   _al_mutex_lock(&timer_mutex);
   {
      int x;

      for (x=0; x<MAX_TIMERS; x++)
         if (_timer_queue[x].timer)
            al_stop_timer(_timer_queue[x].timer);

      al_stop_timer(retrace_timer);

      al_flush_event_queue(event_queue);
   }
   _al_mutex_unlock(&timer_mutex);
}

void _al_resume_old_timer_emulation(void)
{
   TRACE("_al_resume_old_timer_emulation called\n");

   if (!timer_driver)
      return;

   _al_mutex_lock(&timer_mutex);
   {
      int x;

      al_start_timer(retrace_timer);

      for (x=0; x<MAX_TIMERS; x++)
         if (_timer_queue[x].timer)
            al_start_timer(_timer_queue[x].timer);
   }
   _al_mutex_unlock(&timer_mutex);
}

#endif /* ALLEGRO_LINUX */



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
