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
 *      Display switch handling.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"


#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


BOOL app_foreground = TRUE;
HANDLE _foreground_event = NULL;

#define MAX_SWITCH_CALLBACKS  8
static int allegro_thread_priority = 0;
static int switch_mode = SWITCH_PAUSE;
static void (*switch_in_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static void (*switch_out_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };



/* sys_directx_display_switch_init:
 */
void sys_directx_display_switch_init(void)
{
   _foreground_event = CreateEvent(NULL, TRUE, TRUE, NULL);
   set_display_switch_mode(SWITCH_PAUSE);
}



/* sys_directx_display_switch_exit:
 */
void sys_directx_display_switch_exit(void)
{
   CloseHandle(_foreground_event);
}



/* sys_directx_set_display_switch_mode:
 */
int sys_directx_set_display_switch_mode(int mode)
{
   int i;

   switch (mode)
   {
      case SWITCH_BACKGROUND:
      case SWITCH_PAUSE:
         if (win_gfx_driver && !win_gfx_driver->has_backing_store)
            return -1;
	 break;

      case SWITCH_BACKAMNESIA:
      case SWITCH_AMNESIA:
         if (!win_gfx_driver || win_gfx_driver->has_backing_store)
            return -1;
	 break;

      default:
	 return -1;
   } 

   switch_mode = mode;

   /* Clear callbacks and return success */
   for (i=0; i<MAX_SWITCH_CALLBACKS; i++)
      switch_in_cb[i] = switch_out_cb[i] = NULL;

   return 0;
}



/* sys_directx_set_display_switch_callback:
 */
int sys_directx_set_display_switch_callback(int dir, void (*cb)(void))
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (dir == SWITCH_IN) {
	 if (!switch_in_cb[i]) {
	    switch_in_cb[i] = cb;
	    return 0;
	 }
      }
      else {
	 if (!switch_out_cb[i]) {
	    switch_out_cb[i] = cb;
	    return 0;
	 }
      }
   }

   return -1;
}



/* sys_directx_remove_display_switch_callback:
 */
void sys_directx_remove_display_switch_callback(void (*cb)(void))
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (switch_in_cb[i] == cb)
	 switch_in_cb[i] = NULL;

      if (switch_out_cb[i] == cb)
	 switch_out_cb[i] = NULL;
   }
}



/* sys_switch_in:
 *  move application in foreground mode
 */
void sys_switch_in(void)
{
   _TRACE("switch in\n");
   app_foreground = TRUE;

   if (win_gfx_driver && win_gfx_driver->switch_in)
      win_gfx_driver->switch_in();

   wnd_acquire_keyboard();
   wnd_acquire_mouse();
   gfx_directx_restore();
   sys_directx_switch_in_callback();

   /* handle switch modes */
   switch (get_display_switch_mode())
   {
      case SWITCH_AMNESIA:
      case SWITCH_PAUSE:
	 _TRACE("AMNESIA or PAUSE mode recovery\n"); 

	 /* restore old priority and wake up */
	 SetThreadPriority(allegro_thread, allegro_thread_priority);
	 SetEvent(_foreground_event);
	 break;

      default:
	 break;
   } 
}



/* sys_switch_out:
 *  move application in background mode
 */
void sys_switch_out(void)
{
   _TRACE("switch out\n");

   app_foreground = FALSE;

   sys_directx_switch_out_callback();
   mouse_dinput_unacquire();
   key_dinput_unacquire();
   midi_switch_out();

   if (win_gfx_driver && win_gfx_driver->switch_out)
      win_gfx_driver->switch_out();

   /* handle switch modes */
   switch(get_display_switch_mode())
   {
      case SWITCH_AMNESIA:
      case SWITCH_PAUSE:
	 _TRACE("AMNESIA or PAUSE suspension\n");
	 ResetEvent(_foreground_event);

	 /* for the case that the thread doesn't suspend lower its priority
	  * do this only if a window of another process is active */ 
	 allegro_thread_priority = GetThreadPriority(allegro_thread); 
	 if ((HINSTANCE)GetWindowLong(GetForegroundWindow(), GWL_HINSTANCE)!=allegro_inst)
	    SetThreadPriority(allegro_thread, THREAD_PRIORITY_LOWEST); 

	 break;

      default:
	 break;
   }
}



/* thread_switch_out:
 *  Called by threads that wants to be handled by system driver on switch out
 */
void thread_switch_out()
{ 
   /* handle switch modes */
   switch(get_display_switch_mode())
   { 
      case SWITCH_AMNESIA:
      case SWITCH_PAUSE:
	 WaitForSingleObject(_foreground_event, INFINITE);
	 break;

      default:
	 break;
   } 
}



/* sys_directx_switch_out_callback:
 */
void sys_directx_switch_out_callback(void)
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (switch_out_cb[i])
	 switch_out_cb[i]();
   }
}



/* sys_directx_switch_in_callback:
 */
void sys_directx_switch_in_callback(void)
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (switch_in_cb[i])
	 switch_in_cb[i]();
   }
}


