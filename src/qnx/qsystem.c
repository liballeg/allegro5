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

void qnx_events_handler(void);



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

   DISABLE();
   qnx_events_handler();
   ENABLE();
}



/* qnx_events_handler:
 *  Handles pending system events.
 */
void qnx_events_handler(void)
{
   PhKeyEvent_t *key_event;
   PhPointerEvent_t *mouse_event;
   PhCursorInfo_t cursor_info;
   int ig, dx, dy, buttons = 0;
   short mx, my;
   
   if (!ph_gfx_initialized)
      return;

   /* Special mouse handling for direct mode */
   if ((_mouse_installed) && (ph_screen_context)) {
      ig = PhInputGroup(NULL);
      PhQueryCursor(ig, &cursor_info);
      dx = cursor_info.pos.x - (SCREEN_W / 2);
      dy = cursor_info.pos.y - (SCREEN_H / 2);
      buttons = (cursor_info.button_state & Ph_BUTTON_SELECT ? 0x1 : 0) |
                (cursor_info.button_state & Ph_BUTTON_MENU ? 0x2 : 0) |
                (cursor_info.button_state & Ph_BUTTON_ADJUST ? 0x4 : 0);
      qnx_mouse_handler(dx, dy, 0, buttons);
      PhMoveCursorAbs(ig, SCREEN_W / 2, SCREEN_H / 2);
   }
   
   /* Checks if there is a pending event */
   if (PhEventPeek(ph_event, EVENT_SIZE) != Ph_EVENT_MSG)
      return;
   
   switch(ph_event->type) {
      
      /* Mouse related events */
      case Ph_EV_PTR_MOTION_BUTTON:
      case Ph_EV_PTR_MOTION_NOBUTTON:
      case Ph_EV_BUT_PRESS:
      case Ph_EV_BUT_RELEASE:
         if ((!_mouse_installed) || (ph_screen_context))
            break;
         mouse_event = (PhPointerEvent_t *)PhGetData(ph_event);
         dx = mouse_event->pos.x - _mouse_x;
         dy = mouse_event->pos.y - _mouse_y;
         buttons = (mouse_event->button_state & Ph_BUTTON_SELECT ? 0x1 : 0) |
                   (mouse_event->button_state & Ph_BUTTON_MENU ? 0x2 : 0) |
                   (mouse_event->button_state & Ph_BUTTON_ADJUST ? 0x4 : 0);
         qnx_mouse_handler(dx, dy, 0, buttons);
         break;
         
      /* Key press/release event */
      case Ph_EV_KEY:
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

      /* Expose event */
      case Ph_EV_EXPOSE:
         
         /* TODO: redraw view */
         
         break;
      
      /* Mouse enters/leaves window boundaries */
      case Ph_EV_BOUNDARY:
         if ((!_mouse_installed) || (ph_screen_context))
            break;
         dx = dy = 0;
         switch (ph_event->subtype) {
            case Ph_EV_PTR_ENTER:
               PhQueryCursor(PhInputGroup(NULL), &cursor_info);
               PtGetAbsPosition(ph_window, &mx, &my);
               dy = cursor_info.pos.x - mx;
               dx = cursor_info.pos.y - my;
               _mouse_on = TRUE;
               _mouse_x = dx;
               _mouse_y = dy;
               _handle_mouse_input();
               break;
            case Ph_EV_PTR_LEAVE:
               _mouse_on = FALSE;
               _handle_mouse_input();
               break;
         }
         break;

   }
}



/* qnx_sys_init:
 *  Initializes the QNX system driver.
 */
int qnx_sys_init(void)
{
   PhRegion_t region;
   PtArg_t arg[2];
   PhDim_t dim;
   char exe_name[256];
   
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
   PtSetArg(&arg[0], Pt_ARG_DIM, &dim, 0);
   qnx_get_executable_name(exe_name, 256);
   PtSetArg(&arg[1], Pt_ARG_WINDOW_TITLE, exe_name, 0);

   ph_event = (PhEvent_t *)malloc(EVENT_SIZE);
   if (!(ph_window = PtAppInit(NULL, NULL, NULL, 2, arg))) {
      qnx_sys_exit();
      return -1;
   }

   PgSetRegion(PtWidgetRid(ph_window));
   PgSetClipping(0, NULL);
   PgSetDrawBufferSize(0xffff);
   PtRealizeWidget(ph_window);

   region.cursor_type = Ph_CURSOR_NONE;
   region.events_sense = Ph_EV_KEY | Ph_EV_PTR_ALL | Ph_EV_EXPOSE | Ph_EV_BOUNDARY;
   region.rid = PtWidgetRid(ph_window);
   PhRegionChange(Ph_REGION_CURSOR | Ph_REGION_EV_SENSE, 0, &region, NULL, NULL);
   
   if (_sigalrm_init(qnx_interrupts_handler)) {
      qnx_sys_exit();
      return -1;
   }

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

   PtUnrealizeWidget(ph_window);
   if (ph_event)
      free(ph_event);
   ph_event = NULL;
}



/* qnx_sys_message:
 *  Prints out a message using a system message box.
 */
void qnx_sys_message(AL_CONST char *msg)
{
   const char *button[] = { "&Ok" };

   fprintf(stderr, "%s", msg);
   DISABLE();
   PtAlert(NULL, NULL, "Title", NULL, msg, NULL, 1, button, NULL, 1, 1, Pt_MODAL);
   ENABLE();
}



/* qnx_get_executable_name:
 *  Return full path to the current executable.
 */
void qnx_get_executable_name(char *output, int size)
{
   char *path;

   /* If argv[0] has no explicit path, but we do have $PATH, search there */
   if (!strchr (__crt0_argv[0], '/') && (path = getenv("PATH"))) {
      char *start = path, *end = path, *buffer = NULL, *temp;
      struct stat finfo;

      while (*end) {
	 end = strchr (start, ':');
	 if (!end) end = strchr (start, '\0');

	 /* Resize `buffer' for path component, slash, argv[0] and a '\0' */
	 temp = realloc (buffer, end - start + 1 + strlen (__crt0_argv[0]) + 1);
	 if (temp) {
	    buffer = temp;

	    strncpy (buffer, start, end - start);
	    *(buffer + (end - start)) = '/';
	    strcpy (buffer + (end - start) + 1, __crt0_argv[0]);

	    if ((stat(buffer, &finfo)==0) && (!S_ISDIR (finfo.st_mode))) {
	       do_uconvert (buffer, U_ASCII, output, U_CURRENT, size);
	       free (buffer);
	       return;
	    }
	 } /* else... ignore the failure; `buffer' is still valid anyway. */

	 start = end + 1;
      }
      /* Path search failed */
      free (buffer);
   }

   /* If argv[0] had a slash, or the path search failed, just return argv[0] */
   do_uconvert (__crt0_argv[0], U_ASCII, output, U_CURRENT, size);
}



_DRIVER_INFO *qnx_timer_drivers(void)
{
   return qnx_timer_driver_list;
}

