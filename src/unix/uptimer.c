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
 *      Timer module for Unix, using pthreads.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"



#ifdef HAVE_LIBPTHREAD

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>


#define TIMER_TO_USEC(x)  ((long)((x) / 1.193181))
#define USEC_TO_TIMER(x)  ((long)((x) * (TIMERS_PER_SECOND / 1000000.)))


static int ptimer_init(void);
static void ptimer_exit(void);



TIMER_DRIVER timerdrv_unix_pthreads =
{
   TIMERDRV_UNIX_PTHREADS,
   empty_string,
   empty_string,
   "Unix pthreads timers",
   ptimer_init,
   ptimer_exit,
   NULL, NULL,		/* install_int, remove_int */
   NULL, NULL,		/* install_param_int, remove_param_int */
   NULL, NULL,		/* can_simulate_retrace, simulate_retrace */
   NULL			/* rest */
};



static pthread_t thread;
static int thread_alive;



/* ptimer_thread_func:
 *  The timer thread.
 */
static void *ptimer_thread_func(void *unused)
{
   struct timeval old_time;
   struct timeval new_time;
   struct timeval delay;
   long interval = 0x8000;

   gettimeofday(&old_time, 0);

   while (thread_alive) {
      /* `select' is more accurate than `usleep' on my system.  */
      delay.tv_sec = interval / TIMERS_PER_SECOND;
      delay.tv_usec = TIMER_TO_USEC(interval) % 1000000L;
      select(0, NULL, NULL, NULL, &delay);

      /* Calculate actual time elapsed.  */
      gettimeofday(&new_time, 0);
      interval = USEC_TO_TIMER((new_time.tv_sec - old_time.tv_sec) * 1000000L
			       + (new_time.tv_usec - old_time.tv_usec));
      old_time = new_time;

      /* Handle a tick.  */
      interval = _handle_timer_tick(interval);
   }

   return NULL;
}



/* ptimer_init:
 *  Installs the timer thread.
 */
static int ptimer_init(void)
{
   thread_alive = 1;

   if (pthread_create(&thread, NULL, ptimer_thread_func, NULL) != 0) {
      thread_alive = 0;
      return -1;
   }

   return 0;
}



/* ptimer_exit:
 *  Shuts down the timer thread.
 */
static void ptimer_exit(void)
{
   if (thread_alive) {
      thread_alive = 0;
      pthread_join(thread, NULL);
   }
}


#endif
