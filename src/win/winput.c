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
 *      Input management functions.
 *
 *      By Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <process.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif


#define MAX_EVENTS 4

int input_events;
HANDLE input_event_id[MAX_EVENTS];  /* event #0 is reserved */
void (*input_event_handler[MAX_EVENTS])(void);

static int input_need_thread = FALSE;
static HANDLE input_thread = NULL;
static int input_thread_suicide = FALSE;



/* input_thread_proc:
 *  Thread function that handles the input when there is
 *  no dedicated window thread because the library has
 *  been attached to an external window.
 */
static void input_thread_proc(LPVOID unused)
{
   int result;

   thread_init();
   _TRACE("input thread starts\n");

   /* event loop */
   while (TRUE) {
      result = WaitForMultipleObjects(input_events, input_event_id, FALSE, INFINITE);
      if (result == WAIT_OBJECT_0) {
         /* we were instructed to unblock, make sure we can block again */
         _enter_critical();
         _exit_critical();
         if (input_thread_suicide)
            return;
      }
      else if ((result > WAIT_OBJECT_0) && (result < WAIT_OBJECT_0 + input_events)) {
         /* one of the registered events is in signaled state */
         (*input_event_handler[result - WAIT_OBJECT_0])();
      }
   }

   _TRACE("input thread exits\n");
   thread_exit();
}



/* input_register_event:
 *  Adds an event to the input thread event queue.
 */
int input_register_event(HANDLE event_id, void (*event_handler)(void))
{
   if (input_events == MAX_EVENTS)
      return -1;

   _enter_critical();

   /* unblock the input thread if any */
   if (!input_need_thread || input_thread)
      SetEvent(input_event_id[0]);

   /* effectively add */
   input_event_id[input_events] = event_id;
   input_event_handler[input_events] = event_handler;

   /* adjust the size of the queue */
   input_events++;

   /* create input thread if none */
   if (input_need_thread && !input_thread) {
      input_thread = (HANDLE) _beginthread(input_thread_proc, 0, NULL);
      input_thread_suicide = FALSE;
   }

   _exit_critical();

   _TRACE("1 input event registered (total = %d)\n", input_events-1);
   return 0;
}



/* input_unregister_event:
 *  Removes an event from the input thread event queue.
 */
void input_unregister_event(HANDLE event_id)
{
   int i, base;

   /* look for the requested event */
   for (i=1; i<input_events; i++)  /* event #0 is reserved */
      if (input_event_id[i] == event_id) {
         base = i;
         goto Found;
      }

   return;

 Found:
   _enter_critical();

   /* unblock the input thread */
   SetEvent(input_event_id[0]);

   /* shift the queue to the left if needed */
   for (i=base; i<input_events-1; i++) {
      input_event_id[i] = input_event_id[i+1];
      input_event_handler[i] = input_event_handler[i+1];
   }

   /* adjust the size of the queue */
   input_events--;

   /* kill input thread if no more event */
   if ((input_events == 1) && input_thread) {
      input_thread_suicide = TRUE;
      input_thread = NULL;
   }

   _exit_critical();

   _TRACE("1 input event unregistered (total = %d)\n", input_events-1);
}



/* input_init:
 *  Initializes the module.
 */
void input_init(int need_thread)
{
   input_need_thread = need_thread;

   /* event #0 is reserved by the module */
   input_event_id[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
   input_event_handler[0] = NULL;
   input_events = 1;
}



/* input_exit:
 *  Shuts down the module.
 */
void input_exit(void)
{
   CloseHandle(input_event_id[0]);
}

