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

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintlnx.h"

#include "allegro5/linalleg.h"

#ifndef ALLEGRO_LINUX
   #error Something is wrong with the makefile
#endif


static int  sys_linux_init(void);
static void sys_linux_exit(void);
static void sys_linux_message (const char *msg);
static void sys_linux_save_console_state(void);
static void sys_linux_restore_console_state(void);


/* driver list getters */
#define make_getter(x,y) static _AL_DRIVER_INFO *get_##y##_driver_list (void) { return x##_##y##_driver_list; }
	make_getter (_unix, gfx)
	make_getter (_unix, digi)
	make_getter (_unix, midi)
	make_getter (_al_linux, keyboard)
	make_getter (_al_linux, mouse)
	make_getter (_al_linux, joystick)
#undef make_getter


/* the main system driver for running on the Linux console */
SYSTEM_DRIVER system_linux =
{
   SYSTEM_LINUX,
   "",
   "",
   "Linux console",
   sys_linux_init,
   sys_linux_exit,
   _unix_get_executable_name,
   _unix_get_path,
   _unix_find_resource,
   NULL, /* set_window_title */
   NULL, /* set_close_button_callback */
   sys_linux_message,
   NULL, /* assert */
   sys_linux_save_console_state,
   sys_linux_restore_console_state,
   NULL, /* create_bitmap */
   NULL, /* created_bitmap */
   NULL, /* create_sub_bitmap */
   NULL, /* created_sub_bitmap */
   NULL, /* destroy_bitmap */
   NULL, /* read_hardware_palette */
   NULL, /* set_palette_range */
   NULL, /* get_vtable */
   __al_linux_set_display_switch_mode,
   __al_linux_display_switch_lock,
   NULL, /* desktop_color_depth */
   NULL, /* get_desktop_resolution */
   NULL, /* get_gfx_safe_mode */
   _unix_yield_timeslice,
   get_gfx_driver_list,
   get_digi_driver_list,
   get_midi_driver_list,
   get_keyboard_driver_list,
   get_mouse_driver_list,
   get_joystick_driver_list
};


int __al_linux_have_ioperms = 0;


typedef void (*temp_sighandler_t)(int);
static temp_sighandler_t old_sig_abrt, old_sig_fpe, old_sig_ill, old_sig_segv, old_sig_term, old_sig_int, old_sig_quit;


/* signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static void signal_handler (int num)
{
	al_uninstall_system();
	fprintf (stderr, "Shutting down Allegro due to signal #%d\n", num);
	raise (num);
}


/* __al_linux_bgman_init:
 *  Starts asynchronous processing.
 */
static int __al_linux_bgman_init (void)
{
	_unix_bg_man = &_bg_man_pthreads;
	if (_unix_bg_man->init())
		return -1;
	/*
	if (_unix_bg_man->register_func (__al_linux_update_standard_drivers))
		return -1;
	*/
	return 0;
}


/* __al_linux_bgman_exit:
 *  Stops asynchronous processing.
 */
static void __al_linux_bgman_exit (void)
{
	_unix_bg_man->exit();
}



/* sys_linux_init:
 *  Top level system driver wakeup call.
 */
static int sys_linux_init (void)
{
	/* Get OS type */
	_unix_read_os_type();
	if (os_type != OSTYPE_LINUX) return -1; /* FWIW */

	/* This is the only bit that needs root privileges.  First
	 * we attempt to set our euid to 0, in case this is the
	 * second time we've been called. */
	__al_linux_have_ioperms  = !seteuid (0);
	__al_linux_have_ioperms &= !__al_linux_init_memory();

	/* At this stage we can drop the root privileges. */
	seteuid (getuid());

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
    	if (__al_linux_bgman_init()) {
		/* shutdown everything.  */
		sys_linux_exit();
		return -1;
	}

	/* Mark the beginning of time */
	_al_unix_init_time();

	return 0;
}



/* sys_linux_exit:
 *  The end of the world...
 */
static void sys_linux_exit (void)
{
	/* shut down asynchronous event processing */
	__al_linux_bgman_exit();

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

	__al_linux_shutdown_memory();
}



static void sys_linux_save_console_state(void)
{
   __al_linux_use_console();
}



static void sys_linux_restore_console_state(void)
{
   __al_linux_leave_console();
}



/* sys_linux_message:
 *  Display a message on our original console.
 */
static void sys_linux_message (const char *msg)
{
   char *tmp;
   int ret;
   ASSERT(msg);

   tmp = al_malloc(ALLEGRO_MESSAGE_SIZE);
   msg = uconvert(msg, U_UTF8, tmp, U_ASCII, ALLEGRO_MESSAGE_SIZE);

   do {
      ret = write(STDERR_FILENO, msg, strlen(msg));
      if ((ret < 0) && (errno != EINTR))
	 break;
   } while (ret < (int)strlen(msg));

   __al_linux_got_text_message = true;

   al_free(tmp);
}



