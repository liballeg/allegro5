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
 *      Window close button support by Javier Gonzalez.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


static int sys_directx_init(void);
static void sys_directx_exit(void);
static void sys_directx_get_executable_name(char *output, int size);
static void sys_directx_set_window_title(AL_CONST char *name);
static int sys_directx_set_window_close_button(int enable);
static void sys_directx_set_window_close_hook(void (*proc)(void));
static void sys_directx_message(AL_CONST char *msg);
static void sys_directx_assert(AL_CONST char *msg);
static void sys_directx_save_console_state(void);
static void sys_directx_restore_console_state(void);
static int sys_directx_desktop_color_depth(void);
static int sys_directx_get_desktop_resolution(int *width, int *height);
static void sys_directx_yield_timeslice(void);
static int sys_directx_trace_handler(AL_CONST char *msg);


/* the main system driver for running under DirectX */
SYSTEM_DRIVER system_directx =
{
   SYSTEM_DIRECTX,
   empty_string,                /* char *name; */
   empty_string,                /* char *desc; */
   "DirectX",
   sys_directx_init,
   sys_directx_exit,
   sys_directx_get_executable_name,
   NULL,                        /* AL_METHOD(int, find_resource, (char *dest, char *resource, int size)); */
   sys_directx_set_window_title,
   sys_directx_set_window_close_button,
   sys_directx_set_window_close_hook,
   sys_directx_message,
   sys_directx_assert,
   sys_directx_save_console_state,
   sys_directx_restore_console_state,
   NULL,                        /* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
   NULL,                        /* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
   NULL,                        /* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
   NULL,                        /* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp)); */
   NULL,                        /* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
   NULL,                        /* AL_METHOD(void, read_hardware_palette, (void)); */
   NULL,                        /* AL_METHOD(void, set_palette_range, (struct RGB *p, int from, int to, int vsync)); */
   NULL,                        /* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
   sys_directx_set_display_switch_mode,
   sys_directx_set_display_switch_callback,
   sys_directx_remove_display_switch_callback,
   NULL,                        /* AL_METHOD(void, display_switch_lock, (int lock)); */
   sys_directx_desktop_color_depth,
   sys_directx_get_desktop_resolution,
   sys_directx_yield_timeslice,
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   _get_win_digi_driver_list,   /* AL_METHOD(_DRIVER_INFO *, digi_drivers, (void)); */
   _get_win_midi_driver_list,   /* AL_METHOD(_DRIVER_INFO *, midi_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void)); */
   NULL                         /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};

static char sys_directx_desc[64] = EMPTY_STRING;


_DRIVER_INFO _system_driver_list[] =
{
   {SYSTEM_DIRECTX, &system_directx, TRUE},
   {SYSTEM_NONE, &system_none, FALSE},
   {0, NULL, 0}
};


/* general vars */
HINSTANCE allegro_inst = NULL;
HANDLE allegro_thread = NULL;
int _dx_ver;

/* internals */
static CRITICAL_SECTION critical_section;
static RECT wnd_rect;



/* sys_directx_init:
 *  Top level system driver wakeup call.
 */
static int sys_directx_init(void)
{
   char tmp[64];
   unsigned long win_ver;
   HANDLE current_thread;
   HANDLE current_process;

   /* init thread */
   win_init_thread();

   allegro_inst = GetModuleHandle(NULL);

   /* get allegro thread handle */
   current_thread = GetCurrentThread();
   current_process = GetCurrentProcess();
   DuplicateHandle(current_process, current_thread,
                   current_process, &allegro_thread,
                   0, FALSE, DUPLICATE_SAME_ACCESS);

   /* get versions */
   win_ver = GetVersion();
   os_version = win_ver & 0xFF;
   os_revision = (win_ver & 0xFF00) >> 8;
   os_multitasking = TRUE;

   if (win_ver < 0x80000000) {
      if (os_version == 5) {
         if(os_revision == 1)
	    os_type = OSTYPE_WINXP;
	 else
	    os_type = OSTYPE_WIN2000;
      }
      else
         os_type = OSTYPE_WINNT;
   }
   else if (os_version == 4) {
      if (os_revision == 90)
         os_type = OSTYPE_WINME;
      else if (os_revision == 10)
         os_type = OSTYPE_WIN98;
      else
         os_type = OSTYPE_WIN95;
   }
   else
      os_type = OSTYPE_WIN3;

   _dx_ver = get_dx_ver();

   uszprintf(sys_directx_desc, sizeof(sys_directx_desc),
             uconvert_ascii("DirectX %u.%x", tmp), _dx_ver >> 8, _dx_ver & 0xff);
   system_directx.desc = sys_directx_desc;

   /* setup general critical section */
   InitializeCriticalSection(&critical_section);

   /* install a Windows specific trace handler */
   register_trace_handler(sys_directx_trace_handler);

   /* setup the display switch system */
   sys_directx_display_switch_init();

   /* either use a user window or create a new window */
   if (init_directx_window() != 0)
      goto Error;

   return 0;

 Error:
   sys_directx_exit();

   return -1;
}



/* sys_directx_exit:
 *  The end of the world...
 */
static void sys_directx_exit(void)
{
   /* free allocated resources */
   _free_win_digi_driver_list();
   _free_win_midi_driver_list();

   /* unhook or close window */
   exit_directx_window();

   /* shutdown display switch system */
   sys_directx_display_switch_exit();

   /* remove resources */
   DeleteCriticalSection(&critical_section);

   /* shutdown thread */
   win_exit_thread();

   allegro_inst = NULL;
}



/* sys_directx_get_executable_name:
 *  Returns full path to the current executable.
 */
static void sys_directx_get_executable_name(char *output, int size)
{
   unsigned char *cmd = GetCommandLine();
   int pos = 0;
   int i = 0;
   int q;

   while ((cmd[i]) && (uisspace(cmd[i])))
      i++;

   if ((cmd[i] == '\'') || (cmd[i] == '"'))
      q = cmd[i++];
   else
      q = 0;

   size -= ucwidth(0);

   while ((cmd[i]) && ((q) ? (cmd[i] != q) : (!uisspace(cmd[i])))) {
      size -= ucwidth(cmd[i]);
      if (size < 0)
	 break;

      pos += usetc(output + pos, cmd[i++]);
   }

   usetc(output + pos, 0);
}



/* sys_directx_set_window_title:
 *  Alters the application title.
 */
static void sys_directx_set_window_title(AL_CONST char *name)
{
   do_uconvert(name, U_CURRENT, wnd_title, U_ASCII, WND_TITLE_SIZE);
   SetWindowText(allegro_wnd, wnd_title);
}



/* sys_directx_set_window_close_button:
 *  Enables or disables the window close button.
 */
static int sys_directx_set_window_close_button(int enable)
{
   DWORD class_style;
   HMENU sys_menu;

   /* we get the old class style */
   class_style = GetClassLong(allegro_wnd, GCL_STYLE);

   /* and the system menu handle */
   sys_menu = GetSystemMenu(allegro_wnd, FALSE);

   /* disable or enable the no_close_button flag
    * and the close menu option
    */
   if (enable) {
      class_style &= ~CS_NOCLOSE;
      EnableMenuItem(sys_menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
   }
   else {
      class_style |= CS_NOCLOSE;
      EnableMenuItem(sys_menu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
   }

   /* change the class to the new style */
   SetClassLong(allegro_wnd, GCL_STYLE, class_style);

   /* redraw the whole window to display the changes in the button
    * note we use this because UpdateWindow() only works for the client area
    */
   RedrawWindow(allegro_wnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);

   return 0;
}



/* sys_directx_set_window_close_hook:
 *  Sets the close button user hook or restores default one.
 */
static void sys_directx_set_window_close_hook(void (*proc)(void))
{
   user_close_proc = proc;
}



/* sys_directx_message:
 *  Displays a message.
 */
static void sys_directx_message(AL_CONST char *msg)
{
   char *tmp1 = malloc(ALLEGRO_MESSAGE_SIZE);
   char tmp2[WND_TITLE_SIZE*2];

   while ((ugetc(msg) == '\r') || (ugetc(msg) == '\n'))
      msg += uwidth(msg);

   MessageBoxW(allegro_wnd,
	       (unsigned short *)uconvert(msg, U_CURRENT, tmp1, U_UNICODE, ALLEGRO_MESSAGE_SIZE),
	       (unsigned short *)uconvert(wnd_title, U_ASCII, tmp2, U_UNICODE, sizeof(tmp2)),
	       MB_OK);

   free(tmp1);
}



/* sys_directx_assert
 *  handles assertions
 */
static void sys_directx_assert(AL_CONST char *msg)
{
   OutputDebugString(msg);
}



/* sys_directx_save_console_state:
 *  safes window size
 */
static void sys_directx_save_console_state(void)
{
   GetWindowRect(allegro_wnd, &wnd_rect);
}



/* sys_directx_restore_console_state:
 *  restores old window size
 */
static void sys_directx_restore_console_state(void)
{
   /* unacquire input devices */
   wnd_unacquire_keyboard();
   wnd_unacquire_mouse();

   /* re-size and hide window */
   SetWindowPos(allegro_wnd, HWND_TOP, wnd_rect.left, wnd_rect.top,
		wnd_rect.right - wnd_rect.left, wnd_rect.bottom - wnd_rect.top,
		SWP_NOCOPYBITS);
   ShowWindow(allegro_wnd, SW_SHOW);
}



/* sys_directx_desktop_color_depth:
 *  returns the current desktop color depth
 */
static int sys_directx_desktop_color_depth(void)
{
   /* The regular way of retrieving the desktop
    * color depth is broken under Windows 95:
    *
    *   DEVMODE display_mode;
    *
    *   display_mode.dmSize = sizeof(DEVMODE);
    *   display_mode.dmDriverExtra = 0;
    *   if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &display_mode) == 0)
    *      return 0;
    *
    *   return display_mode.dmBitsPerPel;
    */

   HDC dc;
   int depth;

   dc = GetDC(NULL);
   depth = GetDeviceCaps(dc, BITSPIXEL);
   ReleaseDC(NULL, dc);

   return depth;
}



/* sys_directx_get_desktop_resolution:
 *  returns the current desktop resolution
 */
static int sys_directx_get_desktop_resolution(int *width, int *height)
{
   /* same thing for the desktop resolution:
    *
    *   DEVMODE display_mode;
    *
    *   display_mode.dmSize = sizeof(DEVMODE);
    *   display_mode.dmDriverExtra = 0;
    *   if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &display_mode) == 0)
    *      return -1;
    *
    *   *width  = display_mode.dmPelsWidth;
    *   *height = display_mode.dmPelsHeight;
    *
    *   return 0;
    */

   HDC dc;

   dc = GetDC(NULL);
   *width  = GetDeviceCaps(dc, HORZRES);
   *height = GetDeviceCaps(dc, VERTRES);
   ReleaseDC(NULL, dc);

   return 0;
}



/* sys_directx_yield_timeslice:
 *  yields remaining timeslice portion to the system
 */
static void sys_directx_yield_timeslice(void)
{
   Sleep(0);
}



/* sys_directx_trace_handler
 *  handles trace output
 */
static int sys_directx_trace_handler(AL_CONST char *msg)
{
   OutputDebugString(msg);
   return 0;
}



/* _WinMain:
 *  Entry point for Windows GUI programs, hooked by a macro in alwin.h,
 *  which makes it look as if the application can still have a normal
 *  main() function.
 */
int _WinMain(void *_main, void *hInst, void *hPrev, char *Cmd, int nShow)
{
   int (*mainfunc) (int argc, char *argv[]) = (int (*)(int, char *[]))_main;
   char *argbuf;
   char *cmdline;
   char **argv;
   int argc;
   int argc_max;
   int i, q;

   /* can't use parameter because it doesn't include the executable name */
   cmdline = GetCommandLine();
   i = strlen(cmdline) + 1;
   argbuf = malloc(i);
   memcpy(argbuf, cmdline, i);

   argc = 0;
   argc_max = 64;
   argv = malloc(sizeof(char *) * argc_max);
   if (!argv) {
      free(argbuf);
      return 1;
   }

   i = 0;

   /* parse commandline into argc/argv format */
   while (argbuf[i]) {
      while ((argbuf[i]) && (uisspace(argbuf[i])))
	 i++;

      if (argbuf[i]) {
	 if ((argbuf[i] == '\'') || (argbuf[i] == '"')) {
	    q = argbuf[i++];
	    if (!argbuf[i])
	       break;
	 }
	 else
	    q = 0;

	 argv[argc++] = &argbuf[i];

         if (argc >= argc_max) {
            argc_max += 64;
            argv = realloc(argv, sizeof(char *) * argc_max);
            if (!argv) {
               free(argbuf);
               return 1;
            }
         }

	 while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!uisspace(argbuf[i]))))
	    i++;

	 if (argbuf[i]) {
	    argbuf[i] = 0;
	    i++;
	 }
      }
   }

   argv[argc] = NULL;

   /* call the application entry point */
   i = mainfunc(argc, argv);

   free(argv);
   free(argbuf);

   return i;
}



/* win_err_str:
 *  returns a error string for a window error
 */
char *win_err_str(long err)
{
   static char msg[256];

   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPTSTR)&msg, 0, NULL);

   return msg;
}



/* _enter_critical:
 *  requires exclusive ownership on the code
 */
void _enter_critical(void)
{
   EnterCriticalSection(&critical_section);
}



/* _exit_critical:
 *  releases exclusive ownership on the code
 */
void _exit_critical(void)
{
   LeaveCriticalSection(&critical_section);
}



/* thread_safe_trace:
 *  outputs trace message inside threads
 */
void thread_safe_trace(char *msg,...)
{
   char buf[256];
   va_list ap;

   /* todo: use vsnprintf() */
   va_start(ap, msg);
   vsprintf(buf, msg, ap);
   va_end(ap);

   OutputDebugString(buf);
}
