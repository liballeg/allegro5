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
 *      Asynchronous event processing with pthreads.
 *
 *      By George Foot and Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/platform/aintunix.h"

#ifdef HAVE_LIBPTHREAD

#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <limits.h>

static void bg_man_pthreads_enable_interrupts (void);
static void bg_man_pthreads_disable_interrupts (void);

#define MAX_FUNCS 16

static bg_func funcs[MAX_FUNCS];
static int max_func; /* highest+1 used entry */

static pthread_t thread;
static pthread_mutex_t cli_mutex;
static pthread_cond_t cli_cond;
static int cli_count;

static void block_all_signals (void)
{
	sigset_t mask;
	sigfillset (&mask);
	pthread_sigmask (SIG_BLOCK, &mask, NULL);
}

static void *bg_man_pthreads_threadfunc (void *arg)
{
	struct timeval old_time, new_time;
	struct timeval delay;
	unsigned long interval, i;
	int n;

	block_all_signals();

	gettimeofday (&old_time, 0);

	while (1) {
		gettimeofday (&new_time, 0);
		interval = ((new_time.tv_sec - old_time.tv_sec) * 1000000L +
			    (new_time.tv_usec - old_time.tv_usec));
		old_time = new_time;

		while (interval) {
			i = MIN (interval, INT_MAX/(TIMERS_PER_SECOND/100));
			interval -= i;
			i = i * (TIMERS_PER_SECOND/100) / 10000L;

			pthread_mutex_lock (&cli_mutex);
			
			/* wait until interrupts are enabled */
			while (cli_count > 0)
				pthread_cond_wait (&cli_cond, &cli_mutex);

			/* call all the callbacks */
			for (n = 0; n < max_func; n++)
				if (funcs[n]) funcs[n](1);

			pthread_mutex_unlock (&cli_mutex);
		}

		delay.tv_sec = 0;
		delay.tv_usec = 10000;
		select (0, NULL, NULL, NULL, &delay);
		pthread_testcancel();
	}

	return NULL;
}

static int bg_man_pthreads_init (void)
{
	int i;
	for (i = 0; i < MAX_FUNCS; i++)
		funcs[i] = NULL;
	max_func = 0;

	cli_count = 0;
	pthread_mutex_init (&cli_mutex, NULL);
	pthread_cond_init (&cli_cond, NULL);

	if (pthread_create (&thread, NULL, bg_man_pthreads_threadfunc, NULL)) {
		pthread_mutex_destroy (&cli_mutex);
		pthread_cond_destroy (&cli_cond);
		return -1;
	}

	return 0;
}

static void bg_man_pthreads_exit (void)
{
	pthread_cancel (thread);
	pthread_join (thread, NULL);
	pthread_mutex_destroy (&cli_mutex);
	pthread_cond_destroy (&cli_cond);
}

static int bg_man_pthreads_register_func (bg_func f)
{
	int i, ret = 0;
	bg_man_pthreads_disable_interrupts ();

	for (i = 0; funcs[i] && i < MAX_FUNCS; i++);
	if (i == MAX_FUNCS)
		ret = -1;
    	else {
	    	funcs[i] = f;
		if (i == max_func) max_func++;
	}

	bg_man_pthreads_enable_interrupts ();
	return ret;
}

static int really_unregister_func (bg_func f)
{
	int i;

	for (i = 0; i < max_func && funcs[i] != f; i++);
	if (i == max_func)
		return -1;
	else {
		funcs[i] = NULL;			
		if (i+1 == max_func)
			do {
				max_func--;
			} while ((max_func > 0) && !funcs[max_func-1]);
		return 0;
	}
}

static int bg_man_pthreads_unregister_func (bg_func f)
{
	/* Normally we just bg_man_pthread_disable_interrupts(), remove `f',
	 * then bg_man_pthread_enable_interrupts().
	 *
	 * However, the X system driver's input handler is a bg_man callback.
	 * When it receives a close event, it calls exit(), which calls
	 * allegro_exit().  Eventually various subsystems will try to
	 * unregister their callbacks, i.e. call this function.  But we're in
	 * the middle of input handler call, and `cli_mutex' is still locked.
	 *
	 * So we... special case!  If the calling thread is the bg_man thread
	 * we simply bypass the locking, as there's no synchronisation
	 * problem to avoid anyway.
	 */

	int ret;

	if (pthread_equal (pthread_self (), thread))
		/* Being called from a bg_man registered callback.  */
		ret = really_unregister_func (f);
	else {
		/* Normal case.  */
		bg_man_pthreads_disable_interrupts ();
		ret = really_unregister_func (f);
		bg_man_pthreads_enable_interrupts ();
	}

	return ret;
}

static void bg_man_pthreads_enable_interrupts (void)
{
	pthread_mutex_lock (&cli_mutex);
	if (--cli_count == 0)
		pthread_cond_signal (&cli_cond);
	pthread_mutex_unlock (&cli_mutex);
}

static void bg_man_pthreads_disable_interrupts (void)
{
	pthread_mutex_lock (&cli_mutex);
	cli_count++;
	pthread_mutex_unlock (&cli_mutex);
}

static int bg_man_pthreads_interrupts_disabled (void)
{
	return cli_count;
}

struct bg_manager _bg_man_pthreads = {
	1,
	bg_man_pthreads_init,
	bg_man_pthreads_exit,
	bg_man_pthreads_register_func,
	bg_man_pthreads_unregister_func,
	bg_man_pthreads_enable_interrupts,
	bg_man_pthreads_disable_interrupts,
	bg_man_pthreads_interrupts_disabled
};



/* _unix_create_mutex:
 *  Creates a mutex and returns a pointer to it.
 */
void *_unix_create_mutex (void)
{
   pthread_mutex_t *mx;

   mx = malloc (sizeof (pthread_mutex_t));
   if (!mx) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   pthread_mutex_init (mx, NULL);

   return (void *) mx;
}



/* _unix_destroy_mutex:
 *  Destroys a mutex.
 */
void _unix_destroy_mutex (void *handle)
{
   pthread_mutex_t *mx = (pthread_mutex_t *) handle;

   pthread_mutex_destroy (mx);

   free (mx);
}



/* _unix_lock_mutex:
 *  Locks a mutex.
 */
void _unix_lock_mutex (void *handle)
{
   pthread_mutex_t *mx = (pthread_mutex_t *) handle;

   pthread_mutex_lock (mx);
}



/* _unix_unlock_mutex:
 *  Unlocks a mutex.
 */
void _unix_unlock_mutex (void *handle)
{
   pthread_mutex_t *mx = (pthread_mutex_t *) handle;

   pthread_mutex_unlock (mx);
}

#endif

