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
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintunix.h"

#ifdef HAVE_LIBPTHREAD

#include <pthread.h>


#define MAX_FUNCS 16
static bg_func funcs[MAX_FUNCS];
static int max_func; /* highest+1 used entry */

static pthread_t thread;

static volatile int request_exit, exitted;

static void *bg_man_pthreads_threadfunc (void *arg)
{
	int i = 0;
	while (!request_exit) {
		if (funcs[i]) funcs[i](1);
		i++;
		if (i >= max_func) i = 0;
		sched_yield();
	}
	exitted = 1;
	return NULL;
}


static int bg_man_pthreads_init (void)
{
	int i;
	for (i = 0; i < MAX_FUNCS; i++)
		funcs[i] = NULL;

	max_func = 0;

	request_exit = 0;
	exitted = 0;

	if (pthread_create (&thread, NULL, bg_man_pthreads_threadfunc, NULL))
		return -1;

	return 0;
}

static void bg_man_pthreads_exit (void)
{
	request_exit = 1;
	pthread_join (thread, NULL);
}

static int bg_man_pthreads_register_func (bg_func f)
{
	int i;
	for (i = 0; funcs[i] && i < MAX_FUNCS; i++);
	if (i == MAX_FUNCS) return -1;

	funcs[i] = f;

	if (i == max_func) max_func++;

	return 0;
}

static int bg_man_pthreads_unregister_func (bg_func f)
{
	int i;
	for (i = 0; funcs[i] != f && i < max_func; i++);
	if (i == max_func) return -1;

	funcs[i] = NULL;

	if (i+1 == max_func)
		do {
			max_func--;
		} while (!funcs[max_func] && max_func > 0);

	return 0;
}

struct bg_manager _bg_man_pthreads = {
	1,
	bg_man_pthreads_init,
	bg_man_pthreads_exit,
	bg_man_pthreads_register_func,
	bg_man_pthreads_unregister_func
};

#endif

