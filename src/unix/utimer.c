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
 *      Timer module for Unix.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"
#ifdef ALLEGRO_QNX
#include "allegro/aintqnx.h"
#else
#include "allegro/aintunix.h"
#endif



static int unix_timer_init(void);
static void unix_timer_exit(void);

void (*_unix_timer_interrupt)(unsigned long interval) = 0;


TIMER_DRIVER timerdrv_unix =
{
   TIMERDRV_UNIX,
   empty_string,
   empty_string,
   "Unix SIGALRM timer",
   unix_timer_init,
   unix_timer_exit,
   NULL, NULL, NULL, NULL, NULL
};


/* Default list of drivers, for when there's no system driver -- but
 * we can't work without one, so the list is empty.
 */
_DRIVER_INFO _timer_driver_list[] = {
   { 0, 0, 0 }
};


/* unix_timerint:
 *  Timer "interrupt" handler.
 */
static void unix_timerint(unsigned long interval)
{
   _handle_timer_tick(interval);
}



/* unix_timer_init:
 *  Installs the timer driver.
 */
static int unix_timer_init(void)
{
   DISABLE();

   _unix_timer_interrupt = unix_timerint;

   ENABLE();

   return 0;
}



/* unix_timer_exit:
 *  Shuts down the timer driver.
 */
static void unix_timer_exit(void)
{
   DISABLE();

   _unix_timer_interrupt = 0;

   ENABLE();
}

