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
 *      System driver for the QNX realtime platform.
 *
 *      By Angelo Mottola.
 *
 *      Signals handling derived from Unix version by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#include "qnxalleg.h"
#include "allegro/aintern.h"
#include "allegro/aintqnx.h"

#ifndef ALLEGRO_QNX
#error something is wrong with the makefile
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>



/* Global variables */
PtWidget_t       *ph_window = NULL;
PhEvent_t        *ph_event = NULL;



/* Timer driver */
static _DRIVER_INFO qnx_timer_driver_list[] = {
   { TIMERDRV_UNIX,    &timerdrv_unix,  TRUE  },
   { 0,                NULL,            0     }
};



static RETSIGTYPE (*old_sig_abrt)(int num);
static RETSIGTYPE (*old_sig_fpe)(int num);
static RETSIGTYPE (*old_sig_ill)(int num);
static RETSIGTYPE (*old_sig_segv)(int num);
static RETSIGTYPE (*old_sig_term)(int num);
static RETSIGTYPE (*old_sig_int)(int num);
#ifdef SIGKILL
static RETSIGTYPE (*old_sig_kill)(int num);
#endif
#ifdef SIGQUIT
static RETSIGTYPE (*old_sig_quit)(int num);
#endif

static void qnx_events_handler();



/* qnx_signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE qnx_signal_handler(int num)
{
   if (_sigalrm_interrupts_disabled()) {
      /* Can not shutdown X-Windows, restore old signal handlers and slam the door.  */
      signal(SIGABRT, old_sig_abrt);
      signal(SIGFPE,  old_sig_fpe);
      signal(SIGILL,  old_sig_ill);
      signal(SIGSEGV, old_sig_segv);
      signal(SIGTERM, old_sig_term);
      signal(SIGINT,  old_sig_int);
#ifdef SIGKILL
      signal(SIGKILL, old_sig_kill);
#endif
#ifdef SIGQUIT
      signal(SIGQUIT, old_sig_quit);
#endif
      raise(num);
      abort();
   }
   else {
      allegro_exit();
      fprintf(stderr, "Shutting down Allegro due to signal #%d\n", num);
      raise(num);
   }
}



/* qnx_interrupts_handler:
 *  Used for handling asynchronous event processing.
 */
static void qnx_interrupts_handler(unsigned long interval)
{
   if (_unix_timer_interrupt)
      (*_unix_timer_interrupt)(interval);

   qnx_events_handler();
}



/* qnx_events_handler:
 *  Handles pending system events.
 */
void qnx_events_handler()
{
   PhKeyEvent_t *key_event;
   
   DISABLE();	

   /* Checks if there is a pending event */
   if (PhEventPeek(ph_event, EVENT_SIZE) == Ph_EVENT_MSG) {
      switch(ph_event->type) {
         case Ph_EV_KEY:
            /* Key press/release event */
            if (!_keyboard_installed)
               break;
            key_event = (PhKeyEvent_t *)PhGetData(ph_event);
            /* Here we assume key_scan member to be always valid */
            if (key_event->key_flags & Pk_KF_Key_Down) {
               qnx_keyboard_handler(TRUE, key_event->key_scan);
            }
            else {
               qnx_keyboard_handler(FALSE, key_event->key_scan);
            }
            break;
      }
   }      

   ENABLE();
}



/* qnx_sys_init:
 *  Initializes the QNX system driver.
 */
int qnx_sys_init(void)
{
   PtArg_t arg;
   PhDim_t dim;
   
   /* install emergency-exit signal handlers */
   old_sig_abrt = signal(SIGABRT, qnx_signal_handler);
   old_sig_fpe  = signal(SIGFPE,  qnx_signal_handler);
   old_sig_ill  = signal(SIGILL,  qnx_signal_handler);
   old_sig_segv = signal(SIGSEGV, qnx_signal_handler);
   old_sig_term = signal(SIGTERM, qnx_signal_handler);
   old_sig_int  = signal(SIGINT,  qnx_signal_handler);

#ifdef SIGKILL
   old_sig_kill = signal(SIGKILL, qnx_signal_handler);
#endif

#ifdef SIGQUIT
   old_sig_quit = signal(SIGQUIT, qnx_signal_handler);
#endif

   dim.w = 320;
   dim.h = 200;
   PtSetArg(&arg, Pt_ARG_DIM, &dim, 0);
   
   ph_window = PtAppInit(NULL, NULL, NULL, 1, &arg);
   ph_event = (PhEvent_t *)malloc(EVENT_SIZE);

   if ((!ph_window) || (!ph_event) || _sigalrm_init(qnx_interrupts_handler)) {
      qnx_sys_exit();
      return -1;
   }
   
   PgSetRegion(PtWidgetRid(ph_window));
   PtRealizeWidget(ph_window);

   os_type = OSTYPE_QNX;

   set_display_switch_mode(SWITCH_BACKGROUND);

   return 0;
}



/* qnx_sys_exit:
 *  Shuts down the QNX system driver.
 */
void qnx_sys_exit(void)
{
   _sigalrm_exit();
   
   signal(SIGABRT, old_sig_abrt);
   signal(SIGFPE,  old_sig_fpe);
   signal(SIGILL,  old_sig_ill);
   signal(SIGSEGV, old_sig_segv);
   signal(SIGTERM, old_sig_term);
   signal(SIGINT,  old_sig_int);

#ifdef SIGKILL
   signal(SIGKILL, old_sig_kill);
#endif

#ifdef SIGQUIT
   signal(SIGQUIT, old_sig_quit);
#endif
   if (ph_event) {
      free(ph_event);
      ph_event = NULL;
   }
}



void qnx_sys_message(AL_CONST char *msg)
{
   const char *button[] = { "&Ok" };

   fprintf(stderr, "%s", msg);
   DISABLE();
   PtAlert(NULL, NULL, "Title", NULL, msg, NULL, 1, button, NULL, 1, 1, Pt_MODAL);
   ENABLE();
}



_DRIVER_INFO *qnx_timer_drivers(void)
{
   return qnx_timer_driver_list;
}

