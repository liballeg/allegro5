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
 *      Linux console system driver.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "allegro/internal/aintvga.h"
#include "linalleg.h"

#ifdef HAVE_SYS_IO_H
/* iopl() exists in here instead of unistd.h in glibc */
#include <sys/io.h>
#endif

#ifndef ALLEGRO_LINUX
   #error Something is wrong with the makefile
#endif


static int  sys_linux_init(void);
static void sys_linux_exit(void);
static void sys_linux_message (AL_CONST char *msg);


/* driver list getters */
#define make_getter(x,y) static _DRIVER_INFO *get_##y##_driver_list (void) { return x##_##y##_driver_list; }
	make_getter (_unix, gfx)
	make_getter (_unix, digi)
	make_getter (_unix, midi)
	make_getter (_linux, keyboard)
	make_getter (_linux, mouse)
	make_getter (_linux, timer)
	make_getter (_linux, joystick)
#undef make_getter


/* the main system driver for running on the Linux console */
SYSTEM_DRIVER system_linux =
{
   SYSTEM_LINUX,
   empty_string,
   empty_string,
   "Linux console",
   sys_linux_init,
   sys_linux_exit,
   _unix_get_executable_name,
   _unix_find_resource,
   NULL, /* set_window_title */
   NULL, /* set_window_close_button */
   NULL, /* set_window_close_hook */
   sys_linux_message,
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
   __al_linux_set_display_switch_mode,
   __al_linux_set_display_switch_callback,
   __al_linux_remove_display_switch_callback,
   __al_linux_display_switch_lock,
   NULL, /* desktop_color_depth */
   NULL, /* get_desktop_resolution */
   _unix_yield_timeslice,
   get_gfx_driver_list,
   get_digi_driver_list,
   get_midi_driver_list,
   get_keyboard_driver_list,
   get_mouse_driver_list,
   get_joystick_driver_list,
   get_timer_driver_list
};


int __al_linux_have_ioperms = 0;


typedef RETSIGTYPE (*temp_sighandler_t)(int);
static temp_sighandler_t old_sig_abrt, old_sig_fpe, old_sig_ill, old_sig_segv, old_sig_term, old_sig_int, old_sig_quit;


/* signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE signal_handler (int num)
{
	allegro_exit();
	fprintf (stderr, "Shutting down Allegro due to signal #%d\n", num);
	raise (num);
}


/* linux_bg_handler:
 *  Used for handling asynchronous event processing.
 */
static void linux_bg_handler (int threaded)
{
	__al_linux_update_standard_drivers();
}


/* __al_linux_async_init:
 *  Starts asynchronous processing.
 */
static int __al_linux_async_init (void)
{
#ifdef HAVE_LIBPTHREAD
	_unix_bg_man = &_bg_man_pthreads;
#else
	_unix_bg_man = &_bg_man_sigalrm;
#endif
	if (_unix_bg_man->init() || _unix_bg_man->register_func (linux_bg_handler))
		return -1;
#ifndef HAVE_LIBPTHREAD
	al_linux_set_async_mode (ASYNC_DEFAULT);
#endif
	return 0;
}


/* __al_linux_async_exit:
 *  Stops asynchronous processing.
 */
static void __al_linux_async_exit (void)
{
#ifndef HAVE_LIBPTHREAD
	al_linux_set_async_mode (ASYNC_OFF);
#endif
	_unix_bg_man->exit();
}



/* sys_linux_init:
 *  Top level system driver wakeup call.
 */
static int sys_linux_init (void)
{
	/* Get OS type */
	_read_os_type();
	if (os_type != OSTYPE_LINUX) return -1; /* FWIW */

	/* This is the only bit that needs root privileges.  First
	 * we attempt to set our euid to 0, in case this is the
	 * second time we've been called. */
	__al_linux_have_ioperms  = !seteuid (0);
	__al_linux_have_ioperms &= !iopl (3);
	__al_linux_have_ioperms &= !__al_linux_init_memory();

	/* At this stage we can drop the root privileges. */
	seteuid (getuid());

	/* Initialise dynamic driver lists */
	_unix_driver_lists_init();
	if (_unix_gfx_driver_list)
		_driver_list_append_list(&_unix_gfx_driver_list, _linux_gfx_driver_list);

	/* Load dynamic modules */
	_unix_load_modules(SYSTEM_LINUX);
    
	/* Initialise the console subsystem */
	if (__al_linux_init_console()) return -1;

	/* Initialise VGA helpers */
	if (__al_linux_have_ioperms)
		if (__al_linux_init_vga_helpers()) return -1;

	/* Install emergency-exit signal handlers */
	old_sig_abrt = signal(SIGABRT, signal_handler);
	old_sig_fpe  = signal(SIGFPE,  signal_handler);
	old_sig_ill  = signal(SIGILL,  signal_handler);
	old_sig_segv = signal(SIGSEGV, signal_handler);
	old_sig_term = signal(SIGTERM, signal_handler);
	old_sig_int  = signal(SIGINT,  signal_handler);
#ifdef SIGQUIT
	old_sig_quit = signal(SIGQUIT, signal_handler);
#endif

	/* Initialise async event processing */
    	if (__al_linux_async_init()) {
		/* shutdown everything.  */
		sys_linux_exit();
		return -1;
	}

	/* Initialise the console switching system */
	set_display_switch_mode (SWITCH_PAUSE);
	__al_linux_init_vtswitch();

	return 0;
}



/* sys_linux_exit:
 *  The end of the world...
 */
static void sys_linux_exit (void)
{
	/* shut down the console switching system */
	__al_linux_done_vtswitch();

	/* shut down asynchronous event processing */
	__al_linux_async_exit();

	/* remove emergency exit signal handlers */
	signal(SIGABRT, old_sig_abrt);
	signal(SIGFPE,  old_sig_fpe);
	signal(SIGILL,  old_sig_ill);
	signal(SIGSEGV, old_sig_segv);
	signal(SIGTERM, old_sig_term);
	signal(SIGINT,  old_sig_int);
#ifdef SIGQUIT
	signal(SIGQUIT, old_sig_quit);
#endif

	/* shut down VGA helpers */
	__al_linux_shutdown_vga_helpers();

	/* shut down the console subsystem */
	__al_linux_done_console();

	/* unload dynamic modules */
	_unix_unload_modules();

	/* free dynamic driver lists */
	_unix_driver_lists_shutdown();

	__al_linux_shutdown_memory();
	iopl (0);
}



/* sys_linux_message:
 *  Display a message on our original console.
 */
static void sys_linux_message (AL_CONST char *msg)
{
   char *tmp = malloc(ALLEGRO_MESSAGE_SIZE);
   int ret;
   msg = uconvert(msg, U_CURRENT, tmp, U_ASCII, ALLEGRO_MESSAGE_SIZE);

   do {
      ret = write(STDERR_FILENO, msg, strlen(msg));
      if ((ret < 0) && (errno != EINTR))
	 break;
   } while (ret < strlen(msg));

   __al_linux_got_text_message = TRUE;

   free(tmp);
}



