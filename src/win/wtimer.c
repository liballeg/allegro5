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
 *      Windows timer driver. 
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"

#ifndef SCAN_DEPEND
   #include <process.h>
#endif


#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


_DRIVER_INFO _timer_driver_list[] =
{
   /* The multi-threaded driver causes the keyboard to freeze under Win2k
    * when multiple keys are pressed. The fix would be to modify the handling
    * of the keyboard autorepeat in src/keyboard.c, so that it uses a static
    * timer instead of a dynamic one which is re-installed each time a new
    * key is struck.
    * For the time being, we simply use the single-threaded driver.
    */
   {TIMER_WIN32_ST, &timer_win32_st, TRUE},
   {TIMER_WIN32_MT, &timer_win32_mt, TRUE},
   {0, NULL, 0}
};


static int tim_win32_mt_init(void);
static void tim_win32_mt_exit(void);
static int tim_win32_mt_install_int(AL_METHOD(void, proc, (void)), long speed);
static void tim_win32_mt_remove_int(AL_METHOD(void, proc, (void)));
static int tim_win32_mt_install_param_int(AL_METHOD(void, proc, (void *param)), void *param, long speed);
static void tim_win32_mt_remove_param_int(AL_METHOD(void, proc, (void *param)), void *param);

static int tim_win32_st_init(void);
static void tim_win32_st_exit(void);

static int can_simulate_retrace(void);
static void win32_rest(long time, AL_METHOD(void, callback, (void)));


TIMER_DRIVER timer_win32_mt =
{
   TIMER_WIN32_MT,
   empty_string,
   empty_string,
   "Win32 Multithreaded Timer",
   tim_win32_mt_init,
   tim_win32_mt_exit,
   tim_win32_mt_install_int,
   tim_win32_mt_remove_int,
   tim_win32_mt_install_param_int,
   tim_win32_mt_remove_param_int,
   can_simulate_retrace,
   NULL,
   win32_rest
};


TIMER_DRIVER timer_win32_st =
{
   TIMER_WIN32_ST,
   empty_string,
   empty_string,
   "Win32 Singlethreaded Timer",
   tim_win32_st_init,
   tim_win32_st_exit,
   NULL, NULL, NULL, NULL,
   can_simulate_retrace,
   NULL,
   win32_rest
};


typedef struct WIN32_TIMER_INT {
   AL_METHOD(void, proc, (void));
   AL_METHOD(void, param_proc, (void *param));
   void *param;
   int ms_delay;                /* msec */
   LARGE_INTEGER counter_delay; /* in counter units */
   HANDLE thread;
   DWORD threadid;
   int suicide;
   HANDLE restart_event, stop_event;
   unsigned long mynum;         /* bitfield, in powers of two */
} WIN32_TIMER_INT;


static WIN32_TIMER_INT win32_timer_ints[32];    /* no reentrant malloc() */
static unsigned long win32_timer_int_used = 0;  /* bitfield */

static CRITICAL_SECTION tim_crit_sect;

/* High performance counter */
static int use_counter = 0;
static LARGE_INTEGER counter_freq;
static LARGE_INTEGER counter_per_msec;

/* single threaded timer stuff */
static HANDLE st_thread = NULL;
static HANDLE st_stop_event = NULL;
static long timer_delay = 0;    /* how long between interrupts */
static int timer_semaphore = FALSE;     /* reentrant interrupt? */
static long vsync_counter;      /* retrace position counter */
static long vsync_speed;        /* retrace speed */

/* from wdispsw.c */
extern HANDLE _foreground_event;

/* unit conversion */
#define COUNTER_TO_MSEC(x) ((unsigned long)(x / counter_per_msec.QuadPart))
#define COUNTER_TO_TIMER(x) ((unsigned long)(x * TIMERS_PER_SECOND / counter_freq.QuadPart))
#define MSEC_TO_COUNTER(x) (x * counter_per_msec.QuadPart)
#define TIMER_TO_COUNTER(x) (x * counter_freq.QuadPart / TIMERS_PER_SECOND)
#define TIMER_TO_MSEC(x) ((unsigned long)(x) / (TIMERS_PER_SECOND / 1000))


/* find_timer_int:
 */
WIN32_TIMER_INT *find_timer_int(AL_METHOD(void, proc, (void)))
{
   int i;

   for (i=0; i<32; i++) {
      if ((win32_timer_int_used & (1<<i)) && 
	  (win32_timer_ints[i].proc == proc))
	 return &win32_timer_ints[i];
   }

   return NULL;
}



/* find_param_timer_int:
 */
WIN32_TIMER_INT *find_param_timer_int(AL_METHOD(void, proc, (void *param)), void *param)
{
   int i;

   for (i=0; i<32; i++) {
      if ((win32_timer_int_used & (1<<i)) &&
	  (win32_timer_ints[i].param_proc == proc) && 
	  (win32_timer_ints[i].param == param))
	 return &win32_timer_ints[i];
   }

   return NULL;
}



/* mt_timer_thread:
 */
void mt_timer_thread(WIN32_TIMER_INT * This)
{
   HANDLE events[2] =
   {This->stop_event, This->restart_event};
   DWORD result;
   int suicide = FALSE;

   This->threadid = GetCurrentThreadId();

   while (TRUE) {
      if (!app_foreground)
	 thread_switch_out();

      result = WaitForMultipleObjects(2, events, FALSE, MAX(This->ms_delay, 1));

      switch (result) {

	 case WAIT_OBJECT_0 + 0:
	    if (This->suicide) {
	       EnterCriticalSection(&tim_crit_sect);
	       suicide = TRUE;
	    }

	    CloseHandle(This->restart_event);
	    CloseHandle(This->stop_event);

	    win32_timer_int_used &= ~This->mynum;

	    if (suicide)
	       LeaveCriticalSection(&tim_crit_sect);
	    return;             /* end thread */

	 case WAIT_OBJECT_0 + 1:
	    break;              /* start new wait */

	 case WAIT_TIMEOUT:
	    if (This->param_proc)
	       This->param_proc(This->param);
	    else if (This->proc)
	       This->proc();
	    break;              /* call timer proc */
      }
   }
}



/* mt_timer_thread_counter:
 *  version with high performance counter support
 */
void mt_timer_thread_counter(WIN32_TIMER_INT * This)
{
   HANDLE events[2] =
   {This->stop_event, This->restart_event};
   DWORD result;
   unsigned int delay;
   LARGE_INTEGER next_tick;
   LARGE_INTEGER cur_counter;
   LARGE_INTEGER diff_counter;
   int suicide = FALSE;

   /* get initial counter */
   QueryPerformanceCounter(&next_tick);
   next_tick.QuadPart += This->counter_delay.QuadPart;

   This->threadid = GetCurrentThreadId();

   while (TRUE) {
      /* wait until foreground */
      if (WaitForSingleObject(_foreground_event, 0) == WAIT_TIMEOUT) {
	 thread_switch_out();
	 QueryPerformanceCounter(&next_tick);
      }

      /* get current counter */
      QueryPerformanceCounter(&cur_counter);

      while (next_tick.QuadPart < cur_counter.QuadPart) {
	 /* call timer proc */
	 if (This->param_proc)
	    This->param_proc(This->param);
	 else if (This->proc)
	    This->proc();
	 else
	    break;

	 /* calculate next tick */
	 next_tick.QuadPart += This->counter_delay.QuadPart;
      }

      /* get counter units till next tick */
      diff_counter.QuadPart = next_tick.QuadPart - cur_counter.QuadPart;

      /* calculate delay in msec */
      delay = COUNTER_TO_MSEC(diff_counter.QuadPart);

      /* Wait calculated time */
      result = WaitForMultipleObjects(2, events, FALSE, MAX(delay, 1));

      switch (result) {

	 case WAIT_OBJECT_0 + 0:        /* end thread */
	    if (This->suicide) {
	       EnterCriticalSection(&tim_crit_sect);
	       suicide = TRUE;
	    }

	    CloseHandle(This->restart_event);
	    CloseHandle(This->stop_event);

	    win32_timer_int_used &= ~This->mynum;

	    if (suicide)
	       LeaveCriticalSection(&tim_crit_sect);
	    return;

	 case WAIT_OBJECT_0 + 1:;
	    /* start new wait */
	    break;
      }
   }
}



/* stop_timer_thread:
 */
void stop_timer_thread(WIN32_TIMER_INT * timer_int)
{
   /* acknowledge that the thread should stop */
   SetEvent(timer_int->stop_event);

   /* wait until thread has ended */
   if (timer_int->threadid == GetCurrentThreadId())
      timer_int->suicide = TRUE;
   else
      WaitForSingleObject(timer_int->thread, INFINITE);
}



/* retrace_timer:
 */
static void retrace_timer(void)
{
   retrace_count++;
   if (retrace_proc)
      retrace_proc();
}



/* set_sync_timer_freq:
 *  sets the speed of the retrace counter.
 */
void set_sync_timer_freq(int freq)
{
   if (timer_driver == &timer_win32_mt)
      tim_win32_mt_install_int(retrace_timer, BPS_TO_TIMER(freq));
   else
      vsync_speed = BPS_TO_TIMER(freq);
}



/* tim_win32_mt_init:
 */
static int tim_win32_mt_init(void)
{
   win32_timer_int_used = 0;

   InitializeCriticalSection(&tim_crit_sect);

   /* setup high performance counter */
   if (QueryPerformanceFrequency(&counter_freq) != 0) {
      counter_per_msec.QuadPart = counter_freq.QuadPart / 1000;
      use_counter = 1;
   }
   else
      use_counter = 0;

   /* installs retrace simulator timer */
   tim_win32_mt_install_int(retrace_timer, BPS_TO_TIMER(70));

   return 0;
}



/* tim_win32_mt_exit:
 */
static void tim_win32_mt_exit(void)
{
   int i;

   /* stop all timer threads and delete timer int structures */
   EnterCriticalSection(&tim_crit_sect);

   for (i=0; i<32; i++)
      if (win32_timer_int_used & (1<<i))
	 stop_timer_thread(&win32_timer_ints[i]);

   LeaveCriticalSection(&tim_crit_sect);
   DeleteCriticalSection(&tim_crit_sect);
}



/* mt_install_int:
 *  worker function for tim_win32_mt_install_int and tim_win32_mt_install_param_int
 */
static int mt_install_int(void *proc, void *param, long speed, int param_used)
{
   WIN32_TIMER_INT *timer_int;
   int i;

   if (proc) {
      EnterCriticalSection(&tim_crit_sect);

      if (param_used)
	 timer_int = find_param_timer_int(proc, param);
      else
	 timer_int = find_timer_int(proc);

      if (timer_int == NULL) {
	 /* create new timer int structure */
	 for (i=0; i<32; i++) {
	    if (!(win32_timer_int_used & (1<<i))) 
	       break;
	 }

	 if (i >= 32) {
	    LeaveCriticalSection(&tim_crit_sect);
	    return -1;
	 }

	 timer_int = &win32_timer_ints[i];

	 if (param_used) {
	    timer_int->proc = NULL;
	    timer_int->param_proc = proc;
	    timer_int->param = param;
	 }
	 else {
	    timer_int->proc = proc;
	    timer_int->param_proc = NULL;
	    timer_int->param = NULL;
	 }

	 timer_int->ms_delay = TIMER_TO_MSEC(speed);
	 if (use_counter)
	    timer_int->counter_delay.QuadPart = TIMER_TO_COUNTER(speed);
	 timer_int->restart_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	 timer_int->stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	 timer_int->threadid = 0;
	 timer_int->suicide = 0;
	 timer_int->mynum = 1<<i;

	 win32_timer_int_used |= timer_int->mynum;

	 /* start timer thread */
	 if (use_counter) {
	    timer_int->thread = (void *)_beginthread((void (*)(void *))mt_timer_thread_counter, 0, (void *)timer_int);
	 }
	 else {
	    timer_int->thread = (void *)_beginthread((void (*)(void *))mt_timer_thread, 0, (void *)timer_int);
	 }

	 /* increase priority of timer thread */
	 if (timer_int->ms_delay > 0)
	    SetThreadPriority(timer_int->thread, THREAD_PRIORITY_TIME_CRITICAL);
      }
      else {
	 /* modify speed of existing timer int */
	 timer_int->ms_delay = TIMER_TO_MSEC(speed);
	 if (use_counter)
	    timer_int->counter_delay.QuadPart = TIMER_TO_COUNTER(speed);

	 /* restart thread loop */
	 SetEvent(timer_int->restart_event);
      }

      LeaveCriticalSection(&tim_crit_sect);
   }

   return 0;
}



/* tim_win32_mt_install_int:
 */
static int tim_win32_mt_install_int(AL_METHOD(void, proc, (void)), long speed)
{
   return mt_install_int(proc, NULL, speed, 0);
}



/* tim_win32_mt_install_param_int:
 */
static int tim_win32_mt_install_param_int(AL_METHOD(void, proc, (void *param)), void *param, long speed)
{
   return mt_install_int(proc, param, speed, 1);
}



/* mt_remove_int:
 */
static void mt_remove_int(void *proc, void *param, int param_used)
{
   WIN32_TIMER_INT *timer_int;

   /* find timer int structure */
   EnterCriticalSection(&tim_crit_sect);
   if (param_used)
      timer_int = find_param_timer_int(proc, param);
   else
      timer_int = find_timer_int(proc);
   if (timer_int) {
      /* timer int was found, let's stop it */
      stop_timer_thread(timer_int);
   }
   LeaveCriticalSection(&tim_crit_sect);
}



/* tim_win32_mt_remove_int:
 */
static void tim_win32_mt_remove_int(AL_METHOD(void, proc, (void)))
{
   mt_remove_int(proc, NULL, FALSE);
}



/* tim_win32_mt_remove_param_int:
 */
static void tim_win32_mt_remove_param_int(AL_METHOD(void, proc, (void *param)), void *param)
{
   mt_remove_int(proc, param, TRUE);
}



/* st_handle_timer_tick:
 *  Called by the driver to handle a timer tick.
 */
int st_handle_timer_tick(int interval)
{
   long d;
   int i;
   long new_delay = 0x8000;

   timer_delay += interval;

   /* reentrant interrupt? */
   if (timer_semaphore)
      return 0;

   timer_semaphore = TRUE;
   d = timer_delay;

   /* deal with retrace synchronisation */
   vsync_counter -= d;

   if (vsync_counter <= 0) {
      vsync_counter += BPS_TO_TIMER(70);
      retrace_count++;
      if (retrace_proc)
	 retrace_proc();
   }

   if (vsync_counter < new_delay)
      new_delay = vsync_counter;

   /* process the user callbacks */
   for (i = 0; i < MAX_TIMERS; i++) {
      if ((_timer_queue[i].proc) && (_timer_queue[i].speed > 0)) {
	 _timer_queue[i].counter -= d;

	 while ((_timer_queue[i].counter <= 0) && (_timer_queue[i].proc) && (_timer_queue[i].speed > 0)) {
	    _timer_queue[i].counter += _timer_queue[i].speed;
	    _timer_queue[i].proc();
	 }

	 if ((_timer_queue[i].counter > 0) && (_timer_queue[i].counter < new_delay))
	    new_delay = _timer_queue[i].counter;
      }
   }

   timer_delay -= d;
   timer_semaphore = FALSE;

   /* fudge factor to prevent interrupts coming too close to each other */
   if (new_delay < MSEC_TO_TIMER(1))
      new_delay = MSEC_TO_TIMER(1);

   return new_delay;
}



/* st_timer_thread:
 */
void st_timer_thread(void *nil)
{
   DWORD result;
   unsigned long delay = 0x8000;

   while (TRUE) {
      if (!app_foreground)
	 thread_switch_out();

      result = WaitForSingleObject(st_stop_event, MAX(TIMER_TO_MSEC(delay), 1));

      if (result == WAIT_TIMEOUT)
	 delay = st_handle_timer_tick(delay);
      else
	 return;
   }
}



/* st_timer_thread_counter:
 *  version with high performance counter support
 */
void st_timer_thread_counter(WIN32_TIMER_INT * This)
{
   DWORD result;
   unsigned long delay = 0x8000;
   LARGE_INTEGER cur_counter;
   LARGE_INTEGER prev_tick;
   LARGE_INTEGER diff_counter;

   /* get initial counter */
   QueryPerformanceCounter(&prev_tick);

   while (TRUE) {
      /* wait for foreground */
      if (WaitForSingleObject(_foreground_event, 0) == WAIT_TIMEOUT) {
	 thread_switch_out();
	 QueryPerformanceCounter(&prev_tick);
      }

      /* get current counter */
      QueryPerformanceCounter(&cur_counter);

      /* get counter units till next tick */
      diff_counter.QuadPart = cur_counter.QuadPart - prev_tick.QuadPart;

      /* save current counter for next tick */
      prev_tick.QuadPart = cur_counter.QuadPart;

      /* call timer proc */
      delay = st_handle_timer_tick(COUNTER_TO_TIMER(diff_counter.QuadPart));

      /* Wait calculated time */
      result = WaitForSingleObject(st_stop_event, MAX(TIMER_TO_MSEC(delay), 1));
      if (result != WAIT_TIMEOUT) {
	 return;
      }
   }
}



/* tim_win32_st_init:
 */
static int tim_win32_st_init(void)
{
   InitializeCriticalSection(&tim_crit_sect);

   /* setup high performance counter */
   if (QueryPerformanceFrequency(&counter_freq) != 0) {
      counter_per_msec.QuadPart = counter_freq.QuadPart / 1000;
      use_counter = 1;
   }
   else
      use_counter = 0;

   /* start timer thread */
   st_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   if (use_counter)
      st_thread = (void *)_beginthread((void (*)(void *))st_timer_thread_counter, 0, 0);
   else
      st_thread = (void *)_beginthread((void (*)(void *))st_timer_thread, 0, 0);

   /* increase priority of timer thread */
   SetThreadPriority(st_thread, THREAD_PRIORITY_TIME_CRITICAL);

   return 0;
}



/* tim_win32_st_exit:
 */
static void tim_win32_st_exit(void)
{
   /* acknowledge that the thread should stop */
   SetEvent(st_stop_event);

   /* wait until thread has ended */
   WaitForSingleObject(st_thread, INFINITE);

   /* thread has ended, now we can release all resources */
   CloseHandle(st_stop_event);

   DeleteCriticalSection(&tim_crit_sect);
}



/* can_simulate_retrace:
 */
static int can_simulate_retrace(void)
{
   return 0;
}



/* win32_rest:
 */
static void win32_rest(long time, AL_METHOD(void, callback, (void)))
{
   unsigned long start;
   unsigned long ms = time;

   if (callback) {
      start = GetTickCount();
      while (GetTickCount() - start < ms) ;
   }
   else {
      Sleep(ms);
   }
}
