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
 *      Main system driver for the X-Windows library.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>



void (*_xwin_keyboard_interrupt)(int pressed, int code) = 0;
void (*_xwin_keyboard_focused)(int focused, int state) = 0;
void (*_xwin_mouse_interrupt)(int x, int y, int z, int buttons) = 0;


static int _xwin_sysdrv_init(void);
static void _xwin_sysdrv_exit(void);
static void _xwin_sysdrv_set_window_title(AL_CONST char *name);
static void _xwin_sysdrv_set_window_close_hook(void (*proc)(void));
static int _xwin_sysdrv_display_switch_mode(int mode);
static int _xwin_sysdrv_desktop_color_depth(void);
static int _xwin_sysdrv_get_desktop_resolution(int *width, int *height);
static _DRIVER_INFO *_xwin_sysdrv_gfx_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_digi_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_midi_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_keyboard_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_mouse_drivers(void);
#ifdef ALLEGRO_LINUX
static _DRIVER_INFO *_xwin_sysdrv_joystick_drivers(void);
#endif
static _DRIVER_INFO *_xwin_sysdrv_timer_drivers(void);


/* the main system driver for running under X-Windows */
SYSTEM_DRIVER system_xwin =
{
   SYSTEM_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows",
   _xwin_sysdrv_init,
   _xwin_sysdrv_exit,
   _unix_get_executable_name,
   _unix_find_resource,
   _xwin_sysdrv_set_window_title,
   NULL, /* set_window_close_button */
   _xwin_sysdrv_set_window_close_hook,
   NULL, /* message */
   NULL, /* assert */
   NULL, /* save_console_state */
   NULL, /* restore_console_state */
   NULL, /* create_bitmap */
   NULL, /* created_bitmap */
   NULL, /* create_sub_bitmap */
   NULL, /* created_sub_bitmap */
   NULL, /* destroy_bitmap */
   NULL, /* read_hardware_palette */
   NULL, /* set_palette_range */
   NULL, /* get_vtable */
   _xwin_sysdrv_display_switch_mode,
   NULL, /* set_display_switch_callback */
   NULL, /* remove_display_switch_callback */
   NULL, /* display_switch_lock */
   _xwin_sysdrv_desktop_color_depth,
   _xwin_sysdrv_get_desktop_resolution,
   _unix_yield_timeslice,
   _xwin_sysdrv_gfx_drivers,
   _xwin_sysdrv_digi_drivers,
   _xwin_sysdrv_midi_drivers,
   _xwin_sysdrv_keyboard_drivers,
   _xwin_sysdrv_mouse_drivers,
#ifdef ALLEGRO_LINUX
   _xwin_sysdrv_joystick_drivers,
#else
   NULL, /* joystick_driver_list */
#endif
   _xwin_sysdrv_timer_drivers
};



static RETSIGTYPE (*old_sig_abrt)(int num);
static RETSIGTYPE (*old_sig_fpe)(int num);
static RETSIGTYPE (*old_sig_ill)(int num);
static RETSIGTYPE (*old_sig_segv)(int num);
static RETSIGTYPE (*old_sig_term)(int num);
static RETSIGTYPE (*old_sig_int)(int num);
#ifdef SIGQUIT
static RETSIGTYPE (*old_sig_quit)(int num);
#endif

/* _xwin_signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE _xwin_signal_handler(int num)
{
   if (_unix_bg_man->interrupts_disabled() || _xwin.lock_count) {
      /* Can not shutdown X-Windows, restore old signal handlers and slam the door.  */
      signal(SIGABRT, old_sig_abrt);
      signal(SIGFPE,  old_sig_fpe);
      signal(SIGILL,  old_sig_ill);
      signal(SIGSEGV, old_sig_segv);
      signal(SIGTERM, old_sig_term);
      signal(SIGINT,  old_sig_int);
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



/* _xwin_bg_handler:
 *  Really used for synchronous stuff.
 */
static void _xwin_bg_handler (int threaded)
{
   _xwin_handle_input();
}


/* _xwin_sysdrv_init:
 *  Top level system driver wakeup call.
 */
static int _xwin_sysdrv_init(void)
{
   char tmp[256];

   _read_os_type();

   /* install emergency-exit signal handlers */
   old_sig_abrt = signal(SIGABRT, _xwin_signal_handler);
   old_sig_fpe  = signal(SIGFPE,  _xwin_signal_handler);
   old_sig_ill  = signal(SIGILL,  _xwin_signal_handler);
   old_sig_segv = signal(SIGSEGV, _xwin_signal_handler);
   old_sig_term = signal(SIGTERM, _xwin_signal_handler);
   old_sig_int  = signal(SIGINT,  _xwin_signal_handler);

#ifdef SIGQUIT
   old_sig_quit = signal(SIGQUIT, _xwin_signal_handler);
#endif

   /* Initialise dynamic driver lists and load modules */
   _unix_driver_lists_init();
   if (_unix_gfx_driver_list)
      _driver_list_append_list(&_unix_gfx_driver_list, _xwin_gfx_driver_list);
   _unix_load_modules(SYSTEM_XWINDOWS);

#ifdef HAVE_LIBPTHREAD
   _unix_bg_man = &_bg_man_pthreads;
#else
   _unix_bg_man = &_bg_man_sigalrm;
#endif

   /* Initialise bg_man */
   if (_unix_bg_man->init()) {
      _xwin_sysdrv_exit();
      return -1;
   }

   /* If multithreaded bg_man, need to init X's lock/unlock facility. 
    * Note that no X calls must be made before this point! */
   if (_unix_bg_man->multi_threaded)
      XInitThreads();

   get_executable_name(tmp, sizeof(tmp));
   set_window_title(get_filename(tmp));

   /* Open the display, create a window, and background-process 
    * events for it all. */
   if (_xwin_open_display(0) || _xwin_create_window()
       || _unix_bg_man->register_func (_xwin_bg_handler)) {
	 _xwin_sysdrv_exit();
	 return -1;
   }

   set_display_switch_mode(SWITCH_BACKGROUND);

   return 0;
}



/* _xwin_sysdrv_exit:
 *  The end of the world...
 */
static void _xwin_sysdrv_exit(void)
{
   _xwin_close_display();
   _unix_bg_man->exit();

   _unix_unload_modules();
   _unix_driver_lists_shutdown();

   signal(SIGABRT, old_sig_abrt);
   signal(SIGFPE,  old_sig_fpe);
   signal(SIGILL,  old_sig_ill);
   signal(SIGSEGV, old_sig_segv);
   signal(SIGTERM, old_sig_term);
   signal(SIGINT,  old_sig_int);

#ifdef SIGQUIT
   signal(SIGQUIT, old_sig_quit);
#endif
}



/* _xwin_sysdrv_set_window_title:
 *  Sets window title.
 */
static void _xwin_sysdrv_set_window_title(AL_CONST char *name)
{
   char title[128];

   do_uconvert(name, U_CURRENT, title, U_ASCII, sizeof(title));

   _xwin_set_window_title(title);
}



/* _xwin_sysdrv_set_window_close_hook:
 *  Sets window close hook function.
 */
static void _xwin_sysdrv_set_window_close_hook(void (*proc)(void))
{
   _xwin.window_close_hook = proc;
}



/* _xwin_sysdrv_gfx_drivers:
 *  Get the list of graphics drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_gfx_drivers(void)
{
   return _unix_gfx_driver_list;
}



/* _xwin_sysdrv_digi_drivers:
 *  Get the list of digital sound drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_digi_drivers(void)
{
   return _unix_digi_driver_list;
}



/* _xwin_sysdrv_midi_drivers:
 *  Get the list of MIDI drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_midi_drivers(void)
{
   return _unix_midi_driver_list;
}



/* _xwin_sysdrv_keyboard_drivers:
 *  Get the list of keyboard drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_keyboard_drivers(void)
{
   return _xwin_keyboard_driver_list;
}



/* _xwin_sysdrv_mouse_drivers:
 *  Get the list of mouse drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_mouse_drivers(void)
{
   return _xwin_mouse_driver_list;
}



#ifdef ALLEGRO_LINUX
/* _xwin_sysdrv_joystick_drivers:
 *  Get the list of joystick drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_joystick_drivers(void)
{
   return _linux_joystick_driver_list;
}
#endif



/* _xwin_sysdrv_timer_drivers:
 *  Get the list of timer drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_timer_drivers(void)
{
   return _xwin_timer_driver_list;
}



/* _xwin_sysdrv_display_switch_mode:
 *  Tries to set the display switching mode (this is mostly just to
 *  return sensible values to the caller: most of the modes don't
 *  properly apply to X).
 */
static int _xwin_sysdrv_display_switch_mode(int mode)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   if (_xwin.in_dga_mode) {
      if (mode != SWITCH_NONE)
	 return -1;
   }
   else 
#endif
   {
      if (mode != SWITCH_BACKGROUND)
	 return -1;
   }

   return 0;
}



/* _xwin_sysdrv_desktop_color_depth:
 *  Returns the current desktop color depth.
 */
static int _xwin_sysdrv_desktop_color_depth(void)
{
   if (_xwin.window_depth <= 8)
      return 8;
   else if (_xwin.window_depth <= 15)
      return 15;
   else if (_xwin.window_depth == 16)
      return 16;
   else
      return 32;
}



/* _xwin_sysdrv_get_desktop_resolution:
 *  Returns the current desktop resolution.
 */
static int _xwin_sysdrv_get_desktop_resolution(int *width, int *height)
{
   *width = DisplayWidth(_xwin.display, _xwin.screen);
   *height = DisplayHeight(_xwin.display, _xwin.screen);

   return 0;
}

