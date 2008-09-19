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
 *      Main window creation and management.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <string.h>
   #include <process.h>
   #include <time.h>
#endif

#include "win_new.h"

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif


/* wnd_call_proc:
 *  Instructs the window thread to call the specified procedure and
 *  waits until the procedure has returned.
 */
int wnd_call_proc(int (*proc) (void))
{
   return SendMessage(GetForegroundWindow(), _al_win_msg_call_proc, (DWORD) proc, 0);
}



/* wnd_schedule_proc:
 *  instructs the window thread to call the specified procedure but
 *  doesn't wait until the procedure has returned.
 */
void wnd_schedule_proc(int (*proc) (void))
{
   PostMessage(_al_win_active_window, _al_win_msg_call_proc, (DWORD) proc, 0);
}


/* win_grab_input:
 *  Grabs the input devices.
 */
void win_grab_input(void)
{
   //wnd_schedule_proc(key_dinput_acquire);
   wnd_schedule_proc(mouse_dinput_grab);
   wnd_schedule_proc(_al_win_joystick_dinput_acquire);
}
