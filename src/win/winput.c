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


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <process.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-winput INFO: "
#define PREFIX_W                "al-winput WARNING: "
#define PREFIX_E                "al-winput ERROR: "


#define MAX_EVENTS 8

/* input thread event queue */
int _win_input_events;
HANDLE _win_input_event_id[MAX_EVENTS];
void (*_win_input_event_handler[MAX_EVENTS])(void);
ALLEGRO_MUTEX *_al_win_input_mutex;

/* pending event waiting for being processed */
static HANDLE pending_event_id;
static void (*pending_event_handler)(void);

/* internal input thread management */
static HANDLE ack_event = NULL;
static int reserved_events = 0;


/* register_pending_event: [input thread]
 *  Registers effectively the pending event.
 */
static void register_pending_event(void)
{
   /* add the pending event to the queue */
   _win_input_event_id[_win_input_events] = pending_event_id;
   _win_input_event_handler[_win_input_events] = pending_event_handler;

   /* adjust the size of the queue */
   _win_input_events++;

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
   for (i=reserved_events; i<_win_input_events; i++) {
      if (_win_input_event_id[i] == pending_event_id) {
         found = i;
         break;
      }
   }

   if (found >= 0) {
      /* shift the queue to the left */
      for (i=found; i<_win_input_events-1; i++) {
         _win_input_event_id[i] = _win_input_event_id[i+1];
         _win_input_event_handler[i] = _win_input_event_handler[i+1];
      }

      /* adjust the size of the queue */
      _win_input_events--;
   }

   /* acknowledge the unregistration */
   SetEvent(ack_event);
}



/* _win_input_register_event: [primary thread]
 *  Adds an event to the input thread event queue.
 */
int _win_input_register_event(HANDLE event_id, void (*event_handler)(void))
{
   if (_win_input_events == MAX_EVENTS)
      return -1;

   /* record the event */
   pending_event_id = event_id;
   pending_event_handler = event_handler;

   if (al_get_current_display()) {
      /* ask the input thread to register the pending event */
      SetEvent(_win_input_event_id[0]);
   }
   else {
      register_pending_event();
   }

   /* wait for the input thread to acknowledge */
   WaitForSingleObject(ack_event, INFINITE);

   _TRACE(PREFIX_I "1 input event registered (total = %d)\n", _win_input_events-reserved_events);
   return 0;
}



/* _win_input_unregister_event: [primary thread]
 *  Removes an event from the input thread event queue.
 */
void _win_input_unregister_event(HANDLE event_id)
{
   /* record the event */
   pending_event_id = event_id;

   al_lock_mutex(_al_win_input_mutex);

   if (al_get_current_display()) {
      /* ask the input thread to unregister the pending event */
      SetEvent(_win_input_event_id[1]);
   }
   else {
      unregister_pending_event();
   }

   /* wait for the input thread to acknowledge */
   WaitForSingleObject(ack_event, INFINITE);

   al_unlock_mutex(_al_win_input_mutex);

   _TRACE(PREFIX_I "1 input event unregistered (total = %d)\n", _win_input_events-reserved_events);
}



/* _win_input_init: [primary thread]
 *  Initializes the module.
 */
void _win_input_init(void)
{
   _win_input_event_id[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
   _win_input_event_handler[0] = register_pending_event;
   _win_input_event_id[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
   _win_input_event_handler[1] = unregister_pending_event;
   _win_input_events = 2;

   ack_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   _al_win_input_mutex = al_create_mutex();
}



/* _win_input_exit: [primary thread]
 *  Shuts down the module.
 */
void _win_input_exit(void)
{
   int i;

   for (i=0; i<reserved_events; i++)
      CloseHandle(_win_input_event_id[i]);

   _win_input_events = 0;

   CloseHandle(ack_event);

   al_destroy_mutex(_al_win_input_mutex);
   _al_win_input_mutex = NULL;
}

