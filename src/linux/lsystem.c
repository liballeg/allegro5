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
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintunix.h"
#include "allegro/aintvga.h"
#include "linalleg.h"

#ifdef HAVE_SYS_IO_H
/* This exists in Red Hat systems, at least, and defines the iopl function
 * which, contrary to the documentation, is left out of unistd.h.
 */
#include <sys/io.h>
#endif

#ifndef ALLEGRO_LINUX
   #error Something is wrong with the makefile
#endif


static int  sys_linux_init(void);
static void sys_linux_exit(void);
static void sys_linux_get_executable_name(char *output, int size);
static void sys_linux_message (char *msg);

#define make_getter(x) static _DRIVER_INFO *get_##x##_driver_list (void) { return _linux_##x##_driver_list; }
	make_getter (gfx)
	make_getter (keyboard)
	make_getter (mouse)
	make_getter (timer)
	make_getter (joystick)
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
   sys_linux_get_executable_name,
   _unix_find_resource,
   NULL, /* set_window_title */
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
   _unix_yield_timeslice,
   get_gfx_driver_list,
   NULL, /* digi_driver_list */
   NULL, /* midi_driver_list */
   get_keyboard_driver_list,
   get_mouse_driver_list,
   get_joystick_driver_list,
   get_timer_driver_list
};


int __al_linux_have_ioperms = 0;


typedef RETSIGTYPE (*temp_sighandler_t)(int);
static temp_sighandler_t old_sig_abrt, old_sig_fpe, old_sig_ill, old_sig_segv, old_sig_term, old_sig_int, old_sig_kill, old_sig_quit;


/* signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE signal_handler (int num)
{
	allegro_exit();
	fprintf (stderr, "Shutting down Allegro due to signal #%d\n", num);
	raise (num);
}


/* linux_interrupts_handler:
 *  Used for handling asynchronous event processing.
 */
static void _linux_interrupts_handler (unsigned long interval)
{
	if (_unix_timer_interrupt)
		_unix_timer_interrupt (interval);
	__al_linux_update_standard_drivers();
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

	/* Initialise the console subsystem */
	if (__al_linux_init_console()) return -1;

	/* Initialise VGA helpers */
	if (__al_linux_have_ioperms)
		if (__al_linux_init_vga_helpers()) return -1;

	/* install emergency-exit signal handlers */
	old_sig_abrt = signal(SIGABRT, signal_handler);
	old_sig_fpe  = signal(SIGFPE,  signal_handler);
	old_sig_ill  = signal(SIGILL,  signal_handler);
	old_sig_segv = signal(SIGSEGV, signal_handler);
	old_sig_term = signal(SIGTERM, signal_handler);
	old_sig_int  = signal(SIGINT,  signal_handler);
#ifdef SIGKILL
	old_sig_kill = signal(SIGKILL, signal_handler);
#endif
#ifdef SIGQUIT
	old_sig_quit = signal(SIGQUIT, signal_handler);
#endif

	/* Initialise async event processing */
	if (_sigalrm_init(_linux_interrupts_handler)) {
		/* shutdown everything.  */
		sys_linux_exit();
		return -1;
	}
	al_linux_set_async_mode (ASYNC_DEFAULT);

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
	al_linux_set_async_mode (ASYNC_OFF);
	_sigalrm_exit();

	/* remove emergency exit signal handlers */
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

	__al_linux_done_console();
	__al_linux_shutdown_vga_helpers();
	__al_linux_shutdown_memory();
	iopl (0);
}



/* sys_linux_get_executable_name:
 *  Return full path to the current executable.
 */
static void sys_linux_get_executable_name (char *output, int size)
{
	do_uconvert (__crt0_argv[0], U_ASCII, output, U_CURRENT, size);
}


/* sys_linux_message:
 *  Display a message on our original console.
 */
static void sys_linux_message (char *msg)
{
   char *tmp = malloc(4096);
   int ret;
   msg = uconvert(msg, U_CURRENT, tmp, U_ASCII, 4096);

   do {
      ret = write(STDERR_FILENO, msg, strlen(msg));
      if ((ret < 0) && (errno != EINTR))
	 break;
   } while (ret < strlen(msg));

   __al_linux_got_text_message = TRUE;

   free(tmp);
}



