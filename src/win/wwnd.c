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


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <string.h>
   #include <process.h>
   #include <time.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#ifndef WM_APPCOMMAND
   /* from the Platform SDK July 2000 */
   #define WM_APPCOMMAND 0x0319
#endif


/* general */
HWND allegro_wnd = NULL;
char wnd_title[WND_TITLE_SIZE];  /* ASCII string */
int wnd_x = 0;
int wnd_y = 0;
int wnd_width = 0;
int wnd_height = 0;
int wnd_sysmenu = FALSE;

/* graphics */
WIN_GFX_DRIVER *win_gfx_driver;
CRITICAL_SECTION gfx_crit_sect;
int gfx_crit_sect_nesting = 0;

/* close button user hook */
void (*user_close_proc)(void) = NULL;

/* window thread internals */
#define ALLEGRO_WND_CLASS "AllegroWindow"
#define MAX_EVENT 4
static HWND user_wnd = NULL;
static WNDPROC user_wnd_proc = NULL;
static HANDLE wnd_thread = NULL;
static HWND (*wnd_create_proc)(WNDPROC) = NULL;
static int wnd_events = 0;
static HANDLE wnd_event_id[MAX_EVENT];
static void (*wnd_event_handler[MAX_EVENT])(void);
static int old_style = 0;

/* custom window msgs */
#define SWITCH_TIMER  1
static UINT msg_call_proc = 0;
static UINT msg_acquire_keyboard = 0;
static UINT msg_unacquire_keyboard = 0;
static UINT msg_acquire_mouse = 0;
static UINT msg_unacquire_mouse = 0;
static UINT msg_acquire_joystick = 0;
static UINT msg_unacquire_joystick = 0;
static UINT msg_suicide = 0;



/* win_set_window:
 *  Selects an user-defined window for Allegro.
 */
void win_set_window(HWND wnd)
{
   /* TODO: add code to remove old window */
   user_wnd = wnd;
}



/* win_get_window:
 *  Returns the Allegro window handle.
 */
HWND win_get_window(void)
{
   return (user_wnd ? user_wnd : allegro_wnd);
}



/* win_set_wnd_create_proc:
 *  Sets a custom window creation proc.
 */
void win_set_wnd_create_proc(HWND (*proc)(WNDPROC))
{
   wnd_create_proc = proc;
}



/* win_grab_input:
 *  Grabs the input devices.
 */
void win_grab_input(void)
{
   wnd_acquire_keyboard();
   wnd_acquire_mouse();
   wnd_acquire_joystick();
}



/* wnd_call_proc:
 *  Lets call a procedure from the window thread.
 */
int wnd_call_proc(int (*proc) (void))
{
   if (proc)
      return SendMessage(allegro_wnd, msg_call_proc, (DWORD) proc, 0);
   else
      return -1;
}



/* wnd_acquire_keyboard:
 *  Posts msg to window to acquire the keyboard device.
 */
void wnd_acquire_keyboard(void)
{
   PostMessage(allegro_wnd, msg_acquire_keyboard, 0, 0);
}



/* wnd_unacquire_keyboard:
 *  Posts msg to window to unacquire the keyboard device.
 */
void wnd_unacquire_keyboard(void)
{
   PostMessage(allegro_wnd, msg_unacquire_keyboard, 0, 0);
}



/* wnd_acquire_mouse:
 *  Posts msg to window to acquire the mouse device.
 */
void wnd_acquire_mouse(void)
{
   PostMessage(allegro_wnd, msg_acquire_mouse, 0, 0);
}



/* wnd_unacquire_mouse:
 *  Posts msg to window to unacquire the mouse device.
 */
void wnd_unacquire_mouse(void)
{
   PostMessage(allegro_wnd, msg_unacquire_mouse, 0, 0);
}



/* wnd_acquire_joystick:
 *  Posts msg to window to acquire the joystick device.
 */
void wnd_acquire_joystick(void)
{
   PostMessage(allegro_wnd, msg_acquire_joystick, 0, 0);
}



/* wnd_unacquire_joystick:
 *  Posts msg to window to unacquire the joystick device.
 */
void wnd_unacquire_joystick(void)
{
   PostMessage(allegro_wnd, msg_unacquire_joystick, 0, 0);
}



/* wnd_register_event:
 *  Adds an event to the window thread event queue.
 */
int wnd_register_event(HANDLE event_id, void (*event_handler)(void))
{
   if (wnd_events == MAX_EVENT)
      return -1;

   /* effectively add */
   wnd_event_id[wnd_events] = event_id;
   wnd_event_handler[wnd_events] = event_handler;

   /* adjust the size of the queue */
   wnd_events++;

   _TRACE("1 event registered (total = %d)\n", wnd_events);
   return 0;
}



/* wnd_unregister_event:
 *  Removes an event from the windo thread event queue.
 */
void wnd_unregister_event(HANDLE event_id)
{
   int i, base;

   /* look for the requested event */
   for (i=0; i<wnd_events; i++)
      if (wnd_event_id[i] == event_id) {
         base = i;
         goto Found;
      }

   return;

 Found:
   /* shift the queue to the left if needed */
   for (i=base; i<wnd_events-1; i++) {
      wnd_event_id[i] = wnd_event_id[i+1];
      wnd_event_handler[i] = wnd_event_handler[i+1];
   }

   /* adjust the size of the queue */
   wnd_events--;
   _TRACE("1 event unregistered (total = %d)\n", wnd_events);
}



/* directx_wnd_proc:
 *  Window procedure for the Allegro window class.
 */
static LRESULT CALLBACK directx_wnd_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
   PAINTSTRUCT ps;

   if (message == msg_call_proc)
      return ((int (*)(void))wparam) ();

   if (message == msg_acquire_keyboard)
      return key_dinput_acquire();

   if (message == msg_unacquire_keyboard)
      return key_dinput_unacquire();

   if (message == msg_acquire_mouse)
      return mouse_dinput_acquire();

   if (message == msg_unacquire_mouse)
      return mouse_dinput_unacquire();

   if (message == msg_acquire_joystick)
      return joystick_dinput_acquire();

   if (message == msg_unacquire_joystick)
      return joystick_dinput_unacquire();

   if (message == msg_suicide) {
      DestroyWindow(wnd);
      return 0;
   }

   switch (message) {

      case WM_CREATE:
         if (!user_wnd_proc)
            allegro_wnd = wnd;
         break;

      case WM_DESTROY:
         if (user_wnd_proc) {
            /* We need to remove here the modules that depend upon the main window:
             *  - all the DirectX stuff (keyboard, mouse, sound),
             *  - timers.
             */
            remove_keyboard();
            remove_mouse();
            remove_sound();

            /* The system just sent a WA_INACTIVE message, so we need to wake up the
             * timer thread, supposing we are in SWITCH_PAUSE or SWITCH_AMNESIA mode.
             */
            SetEvent(_foreground_event);
            remove_timer();
         }
         else {
            PostQuitMessage(0);
         }

         allegro_wnd = NULL;
         break;

      case WM_ACTIVATE:
         if (LOWORD(wparam) == WA_INACTIVE) {
            sys_switch_out();
         }
         else if (!HIWORD(wparam)) {
            if (gfx_driver && !gfx_driver->windowed) {
               /* 1.2s delay to let Windows complete the switch in fullscreen mode */
               SetTimer(allegro_wnd, SWITCH_TIMER, 1200, NULL);
            }
            else {
               /* no delay in windowed mode */
               PostMessage(allegro_wnd, msg_call_proc, (DWORD)sys_switch_in, 0);
            }
         }
         break;

      case WM_TIMER:
         if (wparam == SWITCH_TIMER) {
            KillTimer(allegro_wnd, SWITCH_TIMER);
            sys_switch_in();
            return 0;
         }
         break;

      case WM_ENTERSIZEMOVE:
         if (win_gfx_driver && win_gfx_driver->enter_sysmode)
            win_gfx_driver->enter_sysmode();
         break;

      case WM_EXITSIZEMOVE:
         if (win_gfx_driver && win_gfx_driver->exit_sysmode)
            win_gfx_driver->exit_sysmode();
         break;

      case WM_MOVE:
         if (GetActiveWindow() == allegro_wnd) {
            if (!IsIconic(allegro_wnd)) {
               wnd_x = (short) LOWORD(lparam);
               wnd_y = (short) HIWORD(lparam);

               if (win_gfx_driver && win_gfx_driver->move)
                  win_gfx_driver->move(wnd_x, wnd_y, wnd_width, wnd_height);
            }
            else if (win_gfx_driver && win_gfx_driver->iconify) {
               win_gfx_driver->iconify();
            }
         }
         break;

      case WM_SIZE:
         wnd_width = LOWORD(lparam);
         wnd_height = HIWORD(lparam);
         break;

      case WM_PAINT:
         if (win_gfx_driver && win_gfx_driver->paint) {
            BeginPaint(wnd, &ps);
            win_gfx_driver->paint(&ps.rcPaint);
            EndPaint(wnd, &ps);
         }
         break;

      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
         /* disable the default message-based key handler
          * needed to prevent conflicts under Win2k
          */
         if (!user_wnd_proc || _keyboard_installed)
            return 0;
         break;

      case WM_APPCOMMAND:
         /* disable the default message-based key handler
          * needed to prevent conflicts under Win2k
          */
         if (!user_wnd_proc || _keyboard_installed)
            return TRUE;
         break;

      case WM_INITMENUPOPUP:
         wnd_sysmenu = TRUE;
         mouse_set_sysmenu(TRUE);

         if (win_gfx_driver && win_gfx_driver->enter_sysmode)
            win_gfx_driver->enter_sysmode();
         break;

      case WM_MENUSELECT:
         if ((HIWORD(wparam) == 0xFFFF) && (!lparam)) {
            wnd_sysmenu = FALSE;
            mouse_set_sysmenu(FALSE);

            if (win_gfx_driver && win_gfx_driver->exit_sysmode)
               win_gfx_driver->exit_sysmode();

            /* needed to get rid of the lost mouse pointer
             * because the system doesn't send any WM_PAINT message.
             */
            InvalidateRect(allegro_wnd, NULL, TRUE);
         }
         break;

      case WM_CLOSE:
         if (!user_wnd_proc) {
            if (user_close_proc) {
               (*user_close_proc)();
            }
            else {
               /* default window close message */
               if (MessageBox(wnd, ALLEGRO_WINDOW_CLOSE_MESSAGE, wnd_title,
                              MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
                  ExitProcess(0);
            }
            return 0;
         }
         break;
   }

   /* pass message to default window proc */
   if (user_wnd_proc)
      return CallWindowProc(user_wnd_proc, wnd, message, wparam, lparam);
   else
      return DefWindowProc(wnd, message, wparam, lparam);
}



/* adjust_window:
 *  Moves and resizes the window if we have full control over it.
 */
int adjust_window(int x, int y, int w, int h)
{
   RECT win_size = {x, y, x+w, y+h };

   if (!user_wnd) {
      /* retrieve the size of the decorated window */
      AdjustWindowRect(&win_size, GetWindowLong(allegro_wnd, GWL_STYLE), FALSE);
   
      /* display the window */
      MoveWindow(allegro_wnd, win_size.left, win_size.top,
                 win_size.right - win_size.left, win_size.bottom - win_size.top, TRUE);

      /* check that the actual window size is the one requested */
      GetClientRect(allegro_wnd, &win_size);
      if (((win_size.right - win_size.left) != w) || ((win_size.bottom - win_size.top) != h))
         return -1;

      wnd_x = x;
      wnd_y = y;
      wnd_width = w;
      wnd_height = h;
   }

   return 0;
}



/* restore_window_style:
 *  Restores the previous style of the window.
 */
void restore_window_style(void)
{
   SetWindowLong(allegro_wnd, GWL_STYLE, old_style);
   SetWindowPos(allegro_wnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}



/* create_directx_window:
 *  Creates the Allegro window.
 */
static HWND create_directx_window(void)
{
   char fname[1024];
   HWND wnd;
   WNDCLASS wnd_class;

   /* setup the window class */
   wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wnd_class.lpfnWndProc = directx_wnd_proc;
   wnd_class.cbClsExtra = 0;
   wnd_class.cbWndExtra = 0;
   wnd_class.hInstance = allegro_inst;
   wnd_class.hIcon = LoadIcon(allegro_inst, "allegro_icon");
   if (!wnd_class.hIcon)
      wnd_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
   wnd_class.hbrBackground = CreateSolidBrush(0);
   wnd_class.lpszMenuName = NULL;
   wnd_class.lpszClassName = ALLEGRO_WND_CLASS;

   RegisterClass(&wnd_class);

   /* what are we called? */
   get_executable_name(fname, sizeof(fname));
   ustrlwr(fname);

   usetc(get_extension(fname), 0);
   if (ugetat(fname, -1) == '.')
      usetat(fname, -1, 0);

   do_uconvert(get_filename(fname), U_CURRENT, wnd_title, U_ASCII, WND_TITLE_SIZE);

   /* create the window now */
   wnd = CreateWindowEx(WS_EX_APPWINDOW, ALLEGRO_WND_CLASS, wnd_title,
                        WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
                        -100, -100, 0, 0,
                        NULL, NULL, allegro_inst, NULL);
   if (!wnd) {
      _TRACE("CreateWindowEx() failed (%s)\n", win_err_str(GetLastError()));
      return NULL;
   }

   ShowWindow(wnd, SW_SHOWNORMAL);
   SetForegroundWindow(wnd);
   UpdateWindow(wnd);

   return wnd;
}



/* wnd_thread_proc:
 *  Thread function that handles the messages of the directx window.
 */
static void wnd_thread_proc(HANDLE setup_event)
{
   int result;
   MSG msg;

   thread_init();
   _TRACE("window thread starts\n");   

   /* setup window */
   if (!wnd_create_proc)
      allegro_wnd = create_directx_window();
   else
      allegro_wnd = wnd_create_proc(directx_wnd_proc);

   if (!allegro_wnd)
      goto End;

   /* now the thread it running successfully, let's acknowledge */
   SetEvent(setup_event);

   /* message loop */
   while (TRUE) {
      result = MsgWaitForMultipleObjects(wnd_events, wnd_event_id, FALSE, INFINITE, QS_ALLINPUT);
      if ((result >= WAIT_OBJECT_0) && (result < WAIT_OBJECT_0 + wnd_events)) {
         /* one of the registered events is in signaled state */
         (*wnd_event_handler[result - WAIT_OBJECT_0])();
      }
      else if (result == WAIT_OBJECT_0 + wnd_events) {
         /* messages are waiting in the queue */
         while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (GetMessage(&msg, NULL, 0, 0)) {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
            }
            else {
               goto End;
            }
         }
      }
   }

 End:
   _TRACE("window thread exits\n");
   thread_exit();
}



/* init_directx_window:
 *  If the user has called win_set_window, the user window will be hooked to
 *  receive messages from Allegro. Otherwise a thread is created that creates
 *  a new window.
 */
int init_directx_window(void)
{
   RECT win_rect;
   HANDLE events[2];
   long result;

   /* setup globals */
   msg_call_proc = RegisterWindowMessage("Allegro call proc");
   msg_acquire_keyboard = RegisterWindowMessage("Allegro keyboard acquire proc");
   msg_unacquire_keyboard = RegisterWindowMessage("Allegro keyboard unacquire proc");
   msg_acquire_mouse = RegisterWindowMessage("Allegro mouse acquire proc");
   msg_unacquire_mouse = RegisterWindowMessage("Allegro mouse unacquire proc");
   msg_acquire_joystick = RegisterWindowMessage("Allegro joystick acquire proc");
   msg_unacquire_joystick = RegisterWindowMessage("Allegro joystick unacquire proc");
   msg_suicide = RegisterWindowMessage("Allegro window suicide");

   /* prepare window for Allegro */
   if (user_wnd) {
      /* hook the user window */
      user_wnd_proc = (WNDPROC) SetWindowLong(user_wnd, GWL_WNDPROC, (long)directx_wnd_proc);
      if (!user_wnd_proc)
         return -1;

      allegro_wnd = user_wnd;

      /* retrieve the window dimensions */
      GetWindowRect(allegro_wnd, &win_rect);
      ClientToScreen(allegro_wnd, (LPPOINT)&win_rect);
      ClientToScreen(allegro_wnd, (LPPOINT)&win_rect + 1);
      wnd_x = win_rect.left;
      wnd_y = win_rect.top;
      wnd_width = win_rect.right - win_rect.left;
      wnd_height = win_rect.bottom - win_rect.top;
   }
   else {
      /* create window thread */
      events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);        /* acknowledges that thread is up */
      events[1] = (HANDLE) _beginthread(wnd_thread_proc, 0, events[0]);
      result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

      CloseHandle(events[0]);

      switch (result) {
	 case WAIT_OBJECT_0:    /* window was created successfully */
	    wnd_thread = events[1];
	    SetThreadPriority(wnd_thread, THREAD_PRIORITY_ABOVE_NORMAL);
	    break;

	 default:               /* thread failed to create window */
	    return -1;
      } 

      /* this should never happen because the thread would also stop */
      if (allegro_wnd == NULL)
	 return -1;
   }

   /* initialize gfx critical section */
   InitializeCriticalSection(&gfx_crit_sect);

   /* save window style */
   old_style = GetWindowLong(allegro_wnd, GWL_STYLE);

   return 0;
}



/* exit_directx_window:
 *  If a user window was hooked, the old window proc is set. Otherwise
 *  the created window is destroyed.
 */
void exit_directx_window(void)
{
   if (user_wnd) {
      /* restore old window proc */
      SetWindowLong(user_wnd, GWL_WNDPROC, (long)user_wnd_proc);
      user_wnd_proc = NULL;
      user_wnd = NULL;
      allegro_wnd = NULL;
   }
   else {
      /* destroy the window: we cannot directly use DestroyWindow()
       * because we are not running in the same thread as that of the window.
       */
      PostMessage(allegro_wnd, msg_suicide, 0, 0);

      /* wait until the window thread ends */
      WaitForSingleObject(wnd_thread, INFINITE);
      wnd_thread = NULL;
   }

   DeleteCriticalSection(&gfx_crit_sect);
}
