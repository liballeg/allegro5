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
 *      Main system driver for the Windows library.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <string.h>
   #include <process.h>
   #include <time.h>
#endif

#include "wddraw.h"

/* general */
HWND allegro_wnd = NULL;
int wnd_x = 0;
int wnd_y = 0;
int wnd_width = 0;
int wnd_height = 0;
int wnd_windowed = FALSE;
int wnd_sysmenu = FALSE;
int wnd_paint_back = FALSE;

struct WIN_GFX_DRIVER *win_gfx_driver = NULL;

#define ALLEGRO_WND_CLASS "AllegroWindow"
static HWND user_wnd = NULL;
static WNDPROC user_wnd_proc = NULL;
static HANDLE wnd_thread = NULL;
static HWND (*wnd_create_proc)(WNDPROC) = NULL;

static int old_style = 0;
static BOOL allow_wm_close = FALSE;

/* custom window msgs */
static UINT msg_call_proc = 0;
static UINT msg_acquire_mouse = 0;
static UINT msg_set_cursor = 0;



/* win_set_window:
 *  Selects a user defined window for Allegro
 */
void win_set_window(HWND wnd)
{
   /* todo: add code to remove old window */
   user_wnd = wnd;
}



/* win_get_window:
 *  returns the allegro window handle
 */
HWND win_get_window(void)
{
   return (user_wnd ? user_wnd : allegro_wnd);
}

void win_set_wnd_create_proc(HWND proc)
{
   wnd_create_proc = proc;
}


/* wnd_call_proc:
 *  lets call a procedure from the window thread
 */
int wnd_call_proc(int (*proc) (void))
{
   if (proc)
      return SendMessage(allegro_wnd, msg_call_proc, (DWORD) proc, 0);
   else
      return -1;
}



/* wnd_acquire_mouse:
 *  post msg to window to acquire the mouse device
 */
void wnd_acquire_mouse(void)
{
   PostMessage(allegro_wnd, msg_acquire_mouse, 0, 0);
}



/* wnd_set_cursor:
 *  post msg to window to set the mouse cursor
 */
void wnd_set_cursor(void)
{
   PostMessage(allegro_wnd, msg_set_cursor, 0, 0);
}



/* directx_wnd_proc:
 *  window proc for the Allegro window class
 */
static LRESULT CALLBACK directx_wnd_proc(HWND wnd, UINT message,
					 WPARAM wparam, LPARAM lparam)
{
   PAINTSTRUCT ps;

   if (message == msg_call_proc)
      return ((int (*)(void))wparam) ();

   if (message == msg_acquire_mouse)
      return mouse_dinput_acquire();

   if (message == msg_set_cursor)
      return mouse_set_cursor();

   switch (message) {

      case WM_CREATE:
	 allegro_wnd = wnd;
	 break;

      case WM_SETCURSOR:
	 return mouse_set_cursor();

      case WM_DESTROY:
	 allegro_wnd = NULL;
	 PostQuitMessage(0);
	 break;

      case WM_ACTIVATE:
	 if (LOWORD(wparam) == WA_INACTIVE)
	    sys_switch_out();
	 else if (!(BOOL) HIWORD(wparam))
	    PostMessage(allegro_wnd, msg_call_proc, (DWORD) sys_switch_in, 0);
	 break;

      case WM_ENTERSIZEMOVE:
         if (win_gfx_driver && win_gfx_driver->enter_size_move)
            win_gfx_driver->enter_size_move();
         break;

      case WM_MOVE:
	 if (GetActiveWindow() == allegro_wnd) {
	    if (!IsIconic(allegro_wnd)) {
	       wnd_x = (short) LOWORD(lparam);
	       wnd_y = (short) HIWORD(lparam);

               if (win_gfx_driver && win_gfx_driver->move)
                  win_gfx_driver->move(wnd_x, wnd_y, wnd_width, wnd_height);
	    }
	    else if (win_gfx_driver && win_gfx_driver->iconify)
               win_gfx_driver->iconify();
	 }
	 break;

      case WM_SIZE:
	 wnd_width = LOWORD(lparam);
	 wnd_height = HIWORD(lparam);
	 break;

      case WM_NCPAINT:
	 if (!wnd_paint_back)
	    return -1;
	 break;

      case WM_PAINT:
         BeginPaint(wnd, &ps);
         if (win_gfx_driver && win_gfx_driver->paint)
            win_gfx_driver->paint(&ps.rcPaint);
         EndPaint(wnd, &ps);
	 break;

      case WM_INITMENUPOPUP:
	 wnd_sysmenu = TRUE;
	 mouse_sysmenu_changed();
         if (win_gfx_driver && win_gfx_driver->init_menu_popup)
            win_gfx_driver->init_menu_popup();
	 break;

      case WM_MENUSELECT:
	 if ((HIWORD(wparam) == 0xFFFF) && (!lparam)) {
	    wnd_sysmenu = FALSE;
	    mouse_sysmenu_changed();
            if (win_gfx_driver && win_gfx_driver->menu_select)
               win_gfx_driver->menu_select(wnd_x, wnd_y, wnd_width, wnd_height);
	 }
	 break;

      case WM_CLOSE:
	 if (!allow_wm_close) {
            if (wnd_windowed) { /* is this line required? */
               /* dialog code to be inserted here */
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



/* hook_user_window:
 *  inserts the directx_wnd_proc into the proc chain of current window
 */
static int hook_user_window(void)
{
   user_wnd_proc = (WNDPROC) SetWindowLong(user_wnd, GWL_WNDPROC, (long)directx_wnd_proc);
   if (user_wnd_proc)
      return 0;
   else
      return -1;
}



/* restore_window_style:
 */
void restore_window_style(void)
{
   SetWindowLong(allegro_wnd, GWL_STYLE, old_style);
}



/* create_directx_window:
 *  create the Allegro window
 */
static HWND create_directx_window(void)
{
   static char title[64];
   char fname[256];
   HWND wnd;
   WNDCLASS wnd_class;
   int err;

   /* setup the window class */
   wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wnd_class.lpfnWndProc = directx_wnd_proc;
   wnd_class.cbClsExtra = 0;
   wnd_class.cbWndExtra = 0;
   wnd_class.hInstance = allegro_inst;
   wnd_class.hIcon = LoadIcon(allegro_inst, IDI_APPLICATION);
   wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
   wnd_class.hbrBackground = CreateSolidBrush(0); /* black bg & color key */
   wnd_class.lpszMenuName = NULL;
   wnd_class.lpszClassName = ALLEGRO_WND_CLASS;

   RegisterClass(&wnd_class);

   /* what are we called? */
   get_executable_name(fname, sizeof(fname));
   ustrlwr(fname);

   usetc(get_extension(fname), 0);
   if (ugetat(fname, -1) == '.')
      usetat(fname, -1, 0);

   do_uconvert(get_filename(fname), U_CURRENT, title, U_ASCII, sizeof(title));

   /* create the window now */
   wnd = CreateWindowEx(  0,
			  ALLEGRO_WND_CLASS,
			  title,
			  WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
			  -1000, -1000, 0, 0,
			  NULL, NULL,
			  allegro_inst,
			  NULL);

   if (wnd == NULL) {
      err = GetLastError();
      _TRACE("CreateWindowEx = %s (%x)\n", win_err_str(err), err);
      return NULL;
   }

   ShowWindow(wnd, SW_SHOWNORMAL);
   UpdateWindow(wnd);

   return wnd;
}



/* wnd_thread_proc:
 *  thread that handles the messages of the directx window
 */
static void wnd_thread_proc(HANDLE setup_event)
{
   MSG msg;

   win_init_thread();

   /* setup window */
   if (!wnd_create_proc)
        allegro_wnd = create_directx_window();
   else
        allegro_wnd = wnd_create_proc(directx_wnd_proc);
   if (allegro_wnd == NULL)
      goto Error;

   /* now the thread it running successfully, let's acknowledge */
   SetEvent(setup_event);

   /* message loop */
   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

 Error:
   win_exit_thread();

   _TRACE("window thread exits\n");
}



/* init_directx_window:
 *  If the user has called win_set_window, the user window will be hooked to
 *  receive messages from Allegro. Otherwise a thread is created that creates
 *  a new window.
 */
int init_directx_window(void)
{
   HANDLE events[2];
   long result;

   /* setup globals */
   msg_call_proc = RegisterWindowMessage("Allegro call proc");
   msg_acquire_mouse = RegisterWindowMessage("Allegro mouse acquire proc");
   msg_set_cursor = RegisterWindowMessage("Allegro mouse cursor proc");

   allow_wm_close = FALSE;

   /* prepare window for allegro */
   if (user_wnd) {
      /* hook the user window */
      if (hook_user_window() != 0)
	 return -1;
      allegro_wnd = user_wnd;
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
	    break;

	 default:               /* thread failed to create window */
	    return -1;
      } 

      /* this should never happen because the thread would also stop */
      if (allegro_wnd == NULL)
	 return -1;
   }

   /* save window style */
   old_style = GetWindowLong(allegro_wnd, GWL_STYLE);

   return 0;
}



/* exit_directx_window:
 *  if a user window was hooked, the old window proc is set. Otherwise
 *  the created window is destroy.
 */
void exit_directx_window(void)
{
   if (user_wnd_proc) {
      /* restore old window proc */
      SetWindowLong(user_wnd, GWL_WNDPROC, (long)user_wnd_proc);
      user_wnd_proc = NULL;
   }
   else {
      /* close window */
      allow_wm_close = TRUE;
      PostMessage(allegro_wnd, WM_CLOSE, 0, 0); 

      /* wait until the window thread ends */
      WaitForSingleObject(wnd_thread, INFINITE);
      wnd_thread = NULL;
   }
}
