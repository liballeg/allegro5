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
 *      Linux joystick driver.
 *
 *      By George Foot.
 *
 *      Modified by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define ALLEGRO_NO_KEY_DEFINES

#include "allegro.h"
#include "allegro/platform/aintunix.h"


#ifdef HAVE_LINUX_JOYSTICK_H

#include <linux/joystick.h>


#define TOTAL_JOYSTICK_AXES	(MAX_JOYSTICK_STICKS * MAX_JOYSTICK_AXIS)

static int joy_fd[MAX_JOYSTICKS];
static JOYSTICK_AXIS_INFO *axis[MAX_JOYSTICKS][TOTAL_JOYSTICK_AXES];


static int joy_init (void)
{
	JOYSTICK_INFO *j;
	char tmp[128], tmp1[128], tmp2[128];
	int version;
	char num_axes, num_buttons;
	int throttle;
	int i, s, a, b;

	for (i = 0; i < MAX_JOYSTICKS; i++) {
		snprintf (tmp, sizeof(tmp), "/dev/input/js%d", i);
		joy_fd[i] = open (tmp, O_RDONLY);
		if (joy_fd[i] == -1) {
			snprintf (tmp, sizeof(tmp), "/dev/js%d", i);
			joy_fd[i] = open (tmp, O_RDONLY);
			if (joy_fd[i] == -1) 
				break;
		}

		ioctl (joy_fd[i], JSIOCGVERSION, &version);
		/* TODO: Check version? */

		ioctl (joy_fd[i], JSIOCGAXES, &num_axes);
		ioctl (joy_fd[i], JSIOCGBUTTONS, &num_buttons);

		if (num_axes > TOTAL_JOYSTICK_AXES) num_axes = TOTAL_JOYSTICK_AXES;
		if (num_buttons > MAX_JOYSTICK_BUTTONS) num_buttons = MAX_JOYSTICK_BUTTONS;

		/* User is allowed to override our simple assumption of which
		 * axis number (kernel) the throttle is located at. */
		uszprintf(tmp, sizeof(tmp), uconvert_ascii("throttle_axis_%d", tmp1), i);
		throttle = get_config_int(uconvert_ascii("joystick", tmp1), tmp, -1);
		if (throttle == -1) {
			throttle = get_config_int(uconvert_ascii("joystick", tmp1), 
						  uconvert_ascii("throttle_axis", tmp2), -1);
		}

		/* Each pair of axes is assumed to make up a stick unless it 
		 * is the sole remaining axis, or has been user specified, in 
		 * which case it is a throttle. */

		j = &joy[i];
		j->flags = JOYFLAG_ANALOGUE;

		for (s = 0, a = 0; a < num_axes; s++) {
			if ((a == throttle) || (a == num_axes-1)) {
				/* One axis throttle */
				j->stick[s].flags = JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
				j->stick[s].num_axis = 1;
				j->stick[s].axis[0].name = get_config_text("Throttle");
				j->stick[s].name = ustrdup (j->stick[s].axis[0].name);
				axis[i][a++] = &j->stick[s].axis[0];
			}
			else {
				/* Two axis stick. */
				j->stick[s].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
				j->stick[s].num_axis = 2;
				j->stick[s].axis[0].name = get_config_text("X");
				j->stick[s].axis[1].name = get_config_text("Y");
				j->stick[s].name = malloc (32);
				uszprintf ((char *)j->stick[s].name, 32, get_config_text("Stick %d"), s+1);
				axis[i][a++] = &j->stick[s].axis[0];
				axis[i][a++] = &j->stick[s].axis[1];
			}
		}

		j->num_sticks = s;

		for (b = 0; b < num_buttons; b++) {
			j->button[b].name = malloc (16);
			uszprintf ((char *)j->button[b].name, 16, uconvert_ascii("%c", tmp), 'A' + b);
		}
		j->num_buttons = num_buttons;
	}

	num_joysticks = i;
	if (num_joysticks == 0) {
		uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open %s: %s"),
			   uconvert_ascii ("/dev/js0", tmp), ustrerror (errno));
		return -1;
	}

	return 0;
}


static void joy_exit (void)
{
	int i, j;
	for (i = 0; i < num_joysticks; i++) {
		close (joy_fd[i]);
		for (j = 0; j < joy[i].num_sticks; j++)
			free ((void *)joy[i].stick[j].name);
		for (j = 0; j < joy[i].num_buttons; j++)
			free ((void *)joy[i].button[j].name);
	}
}


static void set_axis (JOYSTICK_AXIS_INFO *axis, int value)
{
	axis->pos = value * 127 / 32767;
	axis->d1 = (value < -8192);
	axis->d2 = (value > 8192);
}

static int joy_poll (void)
{
	fd_set set;
	struct timeval tv;
	struct js_event e;
	int i, ready;

	for (i = 0; i < num_joysticks; i++) {
		tv.tv_sec = tv.tv_usec = 0;
		FD_ZERO (&set);
		FD_SET (joy_fd[i], &set);
		ready = select (FD_SETSIZE, &set, NULL, NULL, &tv);
		if (ready <= 0) continue;
		read (joy_fd[i], &e, sizeof e);
		if (e.type & JS_EVENT_BUTTON) {
			if (e.number < MAX_JOYSTICK_BUTTONS)
				joy[i].button[e.number].b = e.value;
		} else if (e.type & JS_EVENT_AXIS) {
			if (e.number < TOTAL_JOYSTICK_AXES)
				set_axis (axis[i][e.number], e.value);
		}
	}

	return 0;
}


static int joy_save (void)
{
	return 0;
}

static int joy_load (void)
{
	return 0;
}


static AL_CONST char *joy_calib_name (int n)
{
	return NULL;
}

static int joy_calib (int n)
{
	return -1;
}


JOYSTICK_DRIVER joystick_linux_analogue = {
	JOY_TYPE_LINUX_ANALOGUE,
	empty_string,
	empty_string,
	"Linux analogue joystick(s)",
	joy_init,
	joy_exit,
	joy_poll,
	joy_save,
	joy_load,
	joy_calib_name,
	joy_calib
};

#endif

/* list the available drivers */
_DRIVER_INFO _linux_joystick_driver_list[] = {
#ifdef HAVE_LINUX_JOYSTICK_H
	{    JOY_TYPE_LINUX_ANALOGUE,  &joystick_linux_analogue,  TRUE   },
#endif
	{    JOY_TYPE_NONE,            &joystick_none,            TRUE   },
	{    0,                        0,                         0      }
};

