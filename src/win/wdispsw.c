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

static int allegro_thread_priority = 0;
static int switch_mode = SWITCH_PAUSE;



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

   return 0;
}



/* sys_switch_in:
 *  move application in foreground mode
 */
void sys_switch_in(void)
{
   _TRACE("switch in\n");
   app_foreground = TRUE;

   wnd_acquire_keyboard();
   wnd_acquire_mouse();

   if (win_gfx_driver && win_gfx_driver->switch_in)
      win_gfx_driver->switch_in();

   _switch_in();

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

   _switch_out();
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
