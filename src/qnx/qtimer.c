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
 *      QNX timer driver.
 *
 *      By Eric Botcazou.
 *
 *      Based on the pthreads timer driver for Unix by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
#error Something is wrong with the makefile
#endif

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>


#define TIMER_TO_USEC(x)  ((long)((x) / 1.193181))
#define USEC_TO_TIMER(x)  ((long)((x) * (TIMERS_PER_SECOND / 1000000.)))


static int ptimer_init(void);
static void ptimer_exit(void);


TIMER_DRIVER timer_qnx =
{
   TIMER_QNX,
   empty_string,
   empty_string,
   "Pthreads timer",
   ptimer_init,
   ptimer_exit,
   NULL, NULL,		/* install_int, remove_int */
   NULL, NULL,		/* install_param_int, remove_param_int */
   NULL, NULL,		/* can_simulate_retrace, simulate_retrace */
   NULL			/* rest */
};


static pthread_t thread;
static int thread_alive;


static void block_all_signals(void)
{
   sigset_t mask;
   sigfillset(&mask);
   pthread_sigmask(SIG_BLOCK, &mask, NULL);
}



/* ptimer_thread_func:
 *  The timer thread.
 */
static void *ptimer_thread_func(void *unused)
{
   struct timeval old_time;
   struct timeval new_time;
   struct timeval delay;
   struct sched_param sparam;
   int spolicy;
   long interval = 0x8000;

   /* see the comment in qsystem.c about the thread scheduling policy */
   if (pthread_getschedparam(pthread_self(), &spolicy, &sparam) == EOK) {
      sparam.sched_priority += 2;
      pthread_setschedparam(pthread_self(), spolicy, &sparam);
   }

   block_all_signals();

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
