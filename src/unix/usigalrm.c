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
 *      Asynchronous event processing with SIGALRM.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintunix.h"

#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>


static int _sigalrm_already_installed = FALSE;
static volatile int _sigalrm_interrupt_pending = FALSE;
static volatile int _sigalrm_cli_count = 0;
static volatile int _sigalrm_abort_requested = FALSE;
static volatile int _sigalrm_aborting = FALSE;
static void (*_sigalrm_interrupt_handler)(unsigned long interval) = 0;
void (*_sigalrm_digi_interrupt_handler)(unsigned long interval) = 0;
void (*_sigalrm_midi_interrupt_handler)(unsigned long interval) = 0;
static RETSIGTYPE (*_sigalrm_old_signal_handler)(int num) = 0;

static RETSIGTYPE _sigalrm_signal_handler(int num);



/* _sigalrm_disable_interrupts:
 *  Disables "interrupts".
 */
void _sigalrm_disable_interrupts(void)
{
   _sigalrm_cli_count++;
}



/* _sigalrm_enable_interrupts:
 *  Enables "interrupts.".
 */
void _sigalrm_enable_interrupts(void)
{
   if (--_sigalrm_cli_count == 0) {
      /* Process pending interrupts.  */
      if (_sigalrm_interrupt_pending) {
	 _sigalrm_interrupt_pending = FALSE;
	 _sigalrm_signal_handler(SIGALRM);
      }
   }
   else if (_sigalrm_cli_count < 0) {
      abort();
   }
}



/* _sigalrm_interrupts_disabled:
 *  Tests if interrupts are disabled.
 */
int _sigalrm_interrupts_disabled(void)
{
   return _sigalrm_cli_count;
}



static int first_time = TRUE;
static struct timeval old_time;
static struct timeval new_time;
static int paused;



/* _sigalrm_time_passed:
 *  Calculates time passed since last call to this function.
 */
static unsigned long _sigalrm_time_passed(void)
{
   unsigned long interval;

   if (paused) return 0;

   gettimeofday(&new_time, 0);
   if (!first_time) {
      interval = ((new_time.tv_sec - old_time.tv_sec) * 1000000L +
		  (new_time.tv_usec - old_time.tv_usec));
   }
   else {
      first_time = FALSE;
      interval = 0;
   }

   old_time = new_time;

   return interval;
}



/* _sigalrm_do_abort:
 *  Aborts program (interrupts will be enabled on entry).
 */
static void _sigalrm_do_abort(void)
{
   /* Someday, it may choose to abort or exit, depending on user choice.  */
   if (!_sigalrm_aborting) {
      _sigalrm_aborting = TRUE;
      exit(EXIT_SUCCESS);
   }
}



/* _sigalrm_request_abort:
 *  Requests interrupt-safe abort (abort with enabled interrupts).
 */
void _sigalrm_request_abort(void)
{
   if (_sigalrm_interrupts_disabled())
      _sigalrm_abort_requested = TRUE;
   else
      _sigalrm_do_abort();
}



void _sigalrm_pause(void)
{
   if (paused) return;
   paused = TRUE;
}

void _sigalrm_unpause(void)
{
   if (!paused) return;
   gettimeofday(&old_time, 0);
   paused = FALSE;
}



/* _sigalrm_start_timer:
 *  Starts timer in one-shot mode.
 */
static void _sigalrm_start_timer(void)
{
   struct itimerval timer;

   _sigalrm_old_signal_handler = signal(SIGALRM, _sigalrm_signal_handler);

   timer.it_interval.tv_sec = 0;
   timer.it_interval.tv_usec = 0;
   timer.it_value.tv_sec = 0;
   timer.it_value.tv_usec = 10000; /* 10 ms */
   setitimer(ITIMER_REAL, &timer, 0);
}



/* _sigalrm_stop_timer:
 *  Stops timer.
 */
static void _sigalrm_stop_timer(void)
{
   struct itimerval timer;

   timer.it_interval.tv_sec = 0;
   timer.it_interval.tv_usec = 0;
   timer.it_value.tv_sec = 0;
   timer.it_value.tv_usec = 0;
   setitimer(ITIMER_REAL, &timer, 0);

   signal(SIGALRM, _sigalrm_old_signal_handler);
}



/* _sigalrm_signal_handler:
 *  Handler for SIGALRM signal.
 */
static RETSIGTYPE _sigalrm_signal_handler(int num)
{
   unsigned long interval, i;

   if (_sigalrm_interrupts_disabled()) {
      _sigalrm_interrupt_pending = TRUE;
      return;
   }

   interval = _sigalrm_time_passed();

   while (interval) {
      i = MIN(interval, INT_MAX/(TIMERS_PER_SECOND/100));
      interval -= i;

      i = i * (TIMERS_PER_SECOND/100) / 10000L;

      if (_sigalrm_interrupt_handler)
	 (*_sigalrm_interrupt_handler)(i);

      if (_sigalrm_digi_interrupt_handler)
	 (*_sigalrm_digi_interrupt_handler)(i);

      if (_sigalrm_midi_interrupt_handler)
	 (*_sigalrm_midi_interrupt_handler)(i);
   }

   /* Abort with enabled interrupts.  */
   if (_sigalrm_abort_requested)
      _sigalrm_do_abort();

   _sigalrm_start_timer();
}



/* _sigalrm_init:
 *  Starts interrupts processing.
 */
int _sigalrm_init(void (*handler)(unsigned long interval))
{
   if (_sigalrm_already_installed)
      return -1;

   _sigalrm_already_installed = TRUE;

   _sigalrm_interrupt_handler = handler;
   _sigalrm_interrupt_pending = FALSE;
   _sigalrm_cli_count = 0;

   first_time = TRUE;
   paused = FALSE;

   _sigalrm_start_timer();

   return 0;
}



/* _sigalrm_exit:
 *  Stops interrupts processing.
 */
void _sigalrm_exit(void)
{
   if (!_sigalrm_already_installed)
      return;

   _sigalrm_stop_timer();

   _sigalrm_interrupt_handler = 0;
   _sigalrm_interrupt_pending = FALSE;
   _sigalrm_cli_count = 0;

   _sigalrm_already_installed = FALSE;
}

