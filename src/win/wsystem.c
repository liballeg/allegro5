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

#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"
#include "wddraw.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


static int sys_directx_init(void);
static void sys_directx_exit(void);
static void sys_directx_get_executable_name(char *output, int size);
static void sys_directx_set_window_title(AL_CONST char *name);
static void sys_directx_message(AL_CONST char *msg);
static void sys_directx_assert(AL_CONST char *msg);
static void sys_directx_save_console_state(void);
static void sys_directx_restore_console_state(void);
static int sys_directx_desktop_color_depth(void);
static void sys_directx_yield_timeslice(void);
static int sys_directx_trace_handler(AL_CONST char *msg);
static _DRIVER_INFO *sys_directx_timer_drivers(void);
static _DRIVER_INFO *sys_directx_keyboard_drivers(void);



/* the main system driver for running under DirectX */
SYSTEM_DRIVER system_directx =
{
   SYSTEM_DIRECTX,
   empty_string,                /* char *name; */
   empty_string,                /* char *desc; */
   "DirectX",                   /* char *ascii_name; */
   sys_directx_init,
   sys_directx_exit,
   sys_directx_get_executable_name,
   NULL,                        /* AL_METHOD(int, find_resource, (char *dest, char *resource, int size)); */
   sys_directx_set_window_title,
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
   sys_directx_yield_timeslice,
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   _get_digi_driver_list,       /* AL_METHOD(_DRIVER_INFO *, digi_drivers, (void)); */
   _get_midi_driver_list,       /* AL_METHOD(_DRIVER_INFO *, midi_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void)); */
   NULL                         /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};


static char sys_directx_desc[64];

_DRIVER_INFO _system_driver_list[] =
{
   {SYSTEM_DIRECTX, &system_directx, TRUE},
   {SYSTEM_NONE, &system_none, FALSE},
   {0, NULL, 0}
};


/* general vars */
static CRITICAL_SECTION critical_section;
static RECT wnd_rect;
HINSTANCE allegro_inst = NULL;
HANDLE allegro_thread = NULL;



/* sys_directx_init:
 *  Top level system driver wakeup call.
 */
static int sys_directx_init()
{
   char tmp[64];
   int dx_ver;
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
      current_process, &allegro_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);

   /* get versions */
   dx_ver = get_dx_ver();
   win_ver = GetVersion();
   if (win_ver < 0x80000000) {
      os_type = OSTYPE_WINNT;
   }
   else if ((win_ver & 0xFF) == 4) {
      if ((win_ver & 0xFF00) < 40)
	 os_type = OSTYPE_WIN95;
      else
	 os_type = OSTYPE_WIN98;
   }
   else
      os_type = OSTYPE_WIN3;

   usprintf(sys_directx_desc, uconvert_ascii("DirectX %u.%x", tmp), dx_ver >> 8, dx_ver & 0xff);
   system_directx.desc = sys_directx_desc;

   /* setup critical section for _enter/_exit_critical */
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
static void sys_directx_exit()
{
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
 *  Return full path to the current executable.
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
 *  Alter the application title.
 */
static void sys_directx_set_window_title(AL_CONST char *name)
{
   SetWindowText(allegro_wnd, name);
}



/* sys_directx_message:
 *  Display a message.
 */
static void sys_directx_message(AL_CONST char *msg)
{
   char *tmp1 = malloc(4096);
   char *tmp2 = malloc(128);
   char fname[256], *title;

   get_executable_name(fname, sizeof(fname));
   ustrlwr(fname);

   usetc(get_extension(fname), 0);
   if (ugetat(fname, -1) == '.')
      usetat(fname, -1, 0);

   title = get_filename(fname);

   while ((ugetc(msg) == '\r') || (ugetc(msg) == '\n'))
      msg += uwidth(msg);

   MessageBoxW(allegro_wnd,
	       (unsigned short *)uconvert(msg, U_CURRENT, tmp1, U_UNICODE, 4096),
	       (unsigned short *)uconvert(title, U_CURRENT, tmp2, U_UNICODE, 128),
	       MB_OK);

   free(tmp1);
   free(tmp2);
}



/* sys_directx_assert
 *  handle assertions
 */
static void sys_directx_assert(AL_CONST char *msg)
{
   OutputDebugString(msg);
}



/* sys_directx_save_console_state:
 *  safe window size
 */
static void sys_directx_save_console_state(void)
{
   GetWindowRect(allegro_wnd, &wnd_rect);
}



/* sys_directx_restore_console_state:
 *  restore old window size
 */
static void sys_directx_restore_console_state(void)
{
   SetWindowPos(allegro_wnd, HWND_TOP,
		wnd_rect.left, wnd_rect.top,
		wnd_rect.right - wnd_rect.left, wnd_rect.bottom - wnd_rect.top,
		SWP_SHOWWINDOW);
}



/* sys_directx_desktop_color_depth:
 *  Returns the current desktop color depth.
 */
static int sys_directx_desktop_color_depth(void)
{
/*   DEVMODE display_mode;

   display_mode.dmSize = sizeof(DEVMODE);
   display_mode.dmDriverExtra = 0;
   if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &display_mode) == 0)
      return (0);

   return (display_mode.dmBitsPerPel);*/

   HDC dc;
   int depth;

   dc = GetWindowDC(allegro_wnd);
   depth = GetDeviceCaps(dc, BITSPIXEL);
   ReleaseDC(allegro_wnd, dc);

   return depth;
}



/* sys_directx_yield_timeslice:
 *  Yields remaining timeslice portion to the system
 */
static void sys_directx_yield_timeslice(void)
{
   Sleep(0);
}



/* sys_directx_trace_handler
 *  handle trace output
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
   char argbuf[256];
   char *argv[64];
   int argc;
   int i, q;

   /* can't use parameter because it doesn't include the executable name */
   strcpy(argbuf, GetCommandLine());

   argc = 0;
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

	 while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!uisspace(argbuf[i]))))
	    i++;

	 if (argbuf[i]) {
	    argbuf[i] = 0;
	    i++;
	 }
      }
   }

   /* call the application entry point */
   return mainfunc(argc, argv);
}



/* win_err_str:
 *  returns a error string for a window error
 */
char *win_err_str(long err)
{
   static char msg[256];

   FormatMessage(
		   FORMAT_MESSAGE_FROM_SYSTEM,
		   NULL,
		   err,
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		   (LPTSTR) & msg,
		   0,
		   NULL
       );

   return msg;
}



/* _enter_critical:
 */
void _enter_critical(void)
{
   EnterCriticalSection(&critical_section);
}



/* _exit_critical:
 */
void _exit_critical(void)
{
   LeaveCriticalSection(&critical_section);
}



/* thread_safe_trace:
 *  output trace message inside threads
 */
void thread_safe_trace(char *msg,...)
{
   char buf[256];
   va_list ap;

   va_start(ap, msg);
   vsprintf(buf, msg, ap);
   va_end(ap);

   OutputDebugString(buf);
}
