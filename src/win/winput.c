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


#define MAX_EVENTS 8

/* input thread event queue */
int input_events;
HANDLE input_event_id[MAX_EVENTS];
void (*input_event_handler[MAX_EVENTS])(void);

/* pending event waiting for being processed */
static HANDLE pending_event_id;
static void (*pending_event_handler)(void);

/* internal input thread management */
static HANDLE ack_event = NULL;
static int reserved_events = 0;
static int input_need_thread = FALSE;
static HANDLE input_thread = NULL;



/* input_thread_proc: [input thread]
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
      if (result == WAIT_OBJECT_0 + 2)
         break;  /* thread suicide */
      else if ((result >= WAIT_OBJECT_0) && (result < WAIT_OBJECT_0 + input_events))
         (*input_event_handler[result - WAIT_OBJECT_0])();
   }

   _TRACE("input thread exits\n");
   thread_exit();
}



/* register_pending_event: [input thread]
 *  Registers effectively the pending event.
 */
static void register_pending_event(void)
{
   /* add the pending event to the queue */
   input_event_id[input_events] = pending_event_id;
   input_event_handler[input_events] = pending_event_handler;

   /* adjust the size of the queue */
   input_events++;

   /* acknowledge the registration */
   SetEvent(ack_event);
}



/* unregister_pending_event: [input thread]
 *  Unregisters effectively the pending event.
 */
static void unregister_pending_event(void)
{
   int i, found = -1;

   /* look for the pending event in the event queue */
   for (i=reserved_events; i<input_events; i++) {
      if (input_event_id[i] == pending_event_id) {
         found = i;
         break;
      }
   }

   if (found >= 0) {
      /* shift the queue to the left */
      for (i=found; i<input_events-1; i++) {
         input_event_id[i] = input_event_id[i+1];
         input_event_handler[i] = input_event_handler[i+1];
      }

      /* adjust the size of the queue */
      input_events--;
   }

   /* acknowledge the unregistration */
   SetEvent(ack_event);
}



/* input_register_event: [primary thread]
 *  Adds an event to the input thread event queue.
 */
int input_register_event(HANDLE event_id, void (*event_handler)(void))
{
   if (input_events == MAX_EVENTS)
      return -1;

   /* record the event */
   pending_event_id = event_id;
   pending_event_handler = event_handler;

   /* create the input thread if none */
   if (input_need_thread && !input_thread)
      input_thread = (HANDLE) _beginthread(input_thread_proc, 0, NULL);

   /* ask the input thread to register the pending event */
   SetEvent(input_event_id[0]);

   /* wait for the input thread to acknowledge */
   WaitForSingleObject(ack_event, INFINITE);

   _TRACE("1 input event registered (total = %d)\n", input_events-reserved_events);
   return 0;
}



/* input_unregister_event: [primary thread]
 *  Removes an event from the input thread event queue.
 */
void input_unregister_event(HANDLE event_id)
{
   /* record the event */
   pending_event_id = event_id;

   /* ask the input thread to unregister the pending event */
   SetEvent(input_event_id[1]);

   /* wait for the input thread to acknowledge */
   WaitForSingleObject(ack_event, INFINITE);

   /* kill the input thread if no more event */
   if (input_need_thread && (input_events == reserved_events)) {
      SetEvent(input_event_id[2]);  /* thread suicide */
      input_thread = NULL;
   }

   _TRACE("1 input event unregistered (total = %d)\n", input_events-reserved_events);
}



/* input_init: [primary thread]
 *  Initializes the module.
 */
void input_init(int need_thread)
{
   input_event_id[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
   input_event_handler[0] = register_pending_event;
   input_event_id[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
   input_event_handler[1] = unregister_pending_event;

   if (need_thread) {
      input_need_thread = TRUE;
      input_event_id[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
      reserved_events = 3;
   }
   else {
      input_need_thread = FALSE;
      reserved_events = 2;
   }

   input_events = reserved_events;

   ack_event = CreateEvent(NULL, FALSE, FALSE, NULL);
}



/* input_exit: [primary thread]
 *  Shuts down the module.
 */
void input_exit(void)
{
   int i;

   for (i=0; i<reserved_events; i++)
      CloseHandle(input_event_id[i]);

   input_events = 0;

   CloseHandle(ack_event);
}

