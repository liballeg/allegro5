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
 *      Virtual console handling routines for Linux console Allegro.
 *
 *      By George Foot, based on Marek Habersack's code.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>
#include <sys/ioctl.h>
#include <linux/vt.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "allegro/internal/aintvga.h"
#include "linalleg.h"

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#if !defined(_POSIX_MAPPED_FILES) || !defined(HAVE_MMAP)
#error "Sorry, mapped files are required for Allegro/Linux to work!"
#endif


static int switch_mode = SWITCH_PAUSE;

#define MAX_SWITCH_CALLBACKS  8

static void (*switch_in_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static void (*switch_out_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static int vtswitch_initialised = 0;
static struct vt_mode startup_vtmode;

volatile int __al_linux_switching_blocked = 0;

volatile static int console_active = 1;            /* are we active? */
volatile static int console_should_be_active = 1;  /* should we be? */



int __al_linux_set_display_switch_mode (int mode)
{
	int i;

	/* Clean up after the previous mode, if necessary */
	if (switch_mode == SWITCH_NONE) __al_linux_switching_blocked--;

	switch_mode = mode;

	/* Initialise the new mode */
	if (switch_mode == SWITCH_NONE) __al_linux_switching_blocked++;

	/* Clear callbacks and return success */
	for (i=0; i<MAX_SWITCH_CALLBACKS; i++)
		switch_in_cb[i] = switch_out_cb[i] = NULL;

	return 0;
}



int __al_linux_set_display_switch_callback (int dir, void (*cb) (void))
{
	int i;

	if (switch_mode == SWITCH_NONE) return -1;

	for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
		if (dir == SWITCH_IN) {
			if (!switch_in_cb[i]) {
				switch_in_cb[i] = cb;
				return 0;
			}
		}
		else {
			if (!switch_out_cb[i]) {
				switch_out_cb[i] = cb;
				return 0;
			}
		}
	}

	return -1;
}



void __al_linux_remove_display_switch_callback (void (*cb) (void))
{
	int i;

	for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
		if (switch_in_cb[i] == cb)
			switch_in_cb[i] = NULL;

		if (switch_out_cb[i] == cb)
			switch_out_cb[i] = NULL;
	}
}



/* go_away:
 *  Performs a switch away.
 */
static void go_away()
{
	int i;

	for (i=0; i<MAX_SWITCH_CALLBACKS; i++)
		if (switch_out_cb[i])
			switch_out_cb[i]();

	_unix_bg_man->disable_interrupts();
	if ((switch_mode == SWITCH_PAUSE) || (switch_mode == SWITCH_AMNESIA))
		if (timer_driver) timer_driver->exit();

	/* Disable input devices while we're away */
	__al_linux_disable_standard_driver(STD_MOUSE);
	__al_linux_disable_standard_driver(STD_KBD);

	_save_switch_state(switch_mode);

	if (gfx_driver && gfx_driver->save_video_state)
		gfx_driver->save_video_state();

	ioctl (__al_linux_console_fd, VT_RELDISP, 1); 
	console_active = 0; 

	__al_linux_switching_blocked--;

	if ((switch_mode == SWITCH_PAUSE) || (switch_mode == SWITCH_AMNESIA)) {
		__al_linux_wait_for_display();
		if (timer_driver) timer_driver->init();
	}

	_unix_bg_man->enable_interrupts();
}



/* come_back:
 *  Performs a switch back.
 */
static void come_back()
{
	int i;

	_unix_bg_man->disable_interrupts();

	if (gfx_driver && gfx_driver->restore_video_state)
		gfx_driver->restore_video_state();

	_restore_switch_state();

	ioctl(__al_linux_console_fd, VT_RELDISP, VT_ACKACQ);
	console_active = 1;

	__al_linux_enable_standard_driver(STD_KBD);
	__al_linux_enable_standard_driver(STD_MOUSE);

	_unix_bg_man->enable_interrupts();

	for (i=0; i<MAX_SWITCH_CALLBACKS; i++)
		if (switch_in_cb[i])
			switch_in_cb[i]();

	__al_linux_switching_blocked--;
}



/* poll_console_switch:
 *  Checks whether a switch is needed and not blocked; if so,
 *  makes the switch.
 */
static void poll_console_switch (void)
{
	if (console_active == console_should_be_active) return;
	if (__al_linux_switching_blocked) return;

	__al_linux_switching_blocked++;

	if (console_should_be_active)
		come_back();
	else
		go_away();
}



/* vt_switch_requested:
 *  This is our signal handler; it gets called whenever a switch is
 *  requested, because either SIGRELVT or SIGACQVT is raised.
 */
static RETSIGTYPE vt_switch_requested(int signo)
{
	switch (signo) {
		case SIGRELVT:
			console_should_be_active = 0;
			break;
		case SIGACQVT:
			console_should_be_active = 1;
			break;
		default:
			return;
	}
	poll_console_switch();
}



/* __al_linux_acquire_bitmap:
 *  Increases the switching_blocked counter to block switches during
 *  critical code (e.g. code accessing unsaved graphics controller 
 *  registers; or in background mode, code that does anything at all
 *  connected with the screen bitmap).
 */
void __al_linux_acquire_bitmap(BITMAP *bmp)
{
	__al_linux_switching_blocked++;
}



/* __al_linux_release_bitmap:
 *  Decreases the blocking count and polls for switching.
 */
void __al_linux_release_bitmap(BITMAP *bmp)
{
	__al_linux_switching_blocked--;
	poll_console_switch();
}



/* __al_linux_display_switch_lock:
 *  System driver routine for locking the display around crucial bits of 
 *  code, for example when changing video modes.
 */
void __al_linux_display_switch_lock(int lock, int foreground)
{
	if (foreground) {
		__al_linux_wait_for_display();
	}

	if (lock) {
		__al_linux_switching_blocked++;
	}
	else {
		__al_linux_switching_blocked--;
		poll_console_switch();
	}
}



/* __al_linux_init_vtswitch:
 *  Takes control over our console.  It means we'll be notified when user
 *  switches to and from the graphics console.
 */
int __al_linux_init_vtswitch(void)
{
	struct sigaction sa;
	struct vt_mode vtm;

	if (vtswitch_initialised) return 0;  /* shouldn't happen */

	__al_linux_switching_blocked = (switch_mode == SWITCH_NONE) ? 1 : 0;
	console_active = console_should_be_active = 1;

	/* Hook the signals */
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGIO); /* block async IO during the VT switch */
	sa.sa_flags = 0;
	sa.sa_handler = vt_switch_requested;
	if ((sigaction(SIGRELVT, &sa, NULL) < 0) || (sigaction(SIGACQVT, &sa, NULL) < 0)) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to control VT switching"));
		return 1;
	}

	/* Save old mode, take control, and arrange for the signals
	 * to be raised. */
	ioctl(__al_linux_console_fd, VT_GETMODE, &startup_vtmode);
	vtm = startup_vtmode;

	vtm.mode = VT_PROCESS;
	vtm.relsig = SIGRELVT;
	vtm.acqsig = SIGACQVT;

	ioctl(__al_linux_console_fd, VT_SETMODE, &vtm);

	vtswitch_initialised = 1;

	return 0;
}



/* __al_linux_done_vtswitch:
 *  Undoes the effect of `init_vtswitch'.
 */
int __al_linux_done_vtswitch(void)
{
	struct sigaction sa;

	if (!vtswitch_initialised) return 0;  /* shouldn't really happen either */

	/* !trout gfoot.  Must turn off the signals before unhooking them... */
	ioctl(__al_linux_console_fd, VT_SETMODE, &startup_vtmode);

	sigemptyset (&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = SA_RESTART;
	sigaction (SIGRELVT, &sa, NULL);
	sigaction (SIGACQVT, &sa, NULL);

	vtswitch_initialised = 0;

	return 0;
}


