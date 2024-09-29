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
 *      GP2X Wiz joystick driver
 *
 *      by Trent Gamblin.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/joystick.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwiz.h"
#include "allegro5/platform/alwiz.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_gp2xwiz.h"

static A5O_JOYSTICK joy;

static A5O_JOYSTICK_STATE joystate;
static A5O_THREAD *wiz_joystick_thread;

const int POLLS_PER_SECOND = 60;

#define BUTTON(x) (buttons & x)

static void joywiz_fill_joystate(A5O_JOYSTICK_STATE *state)
{
	uint32_t buttons = lc_getbuttons();

	/* Left-right axis */
	if (BUTTON(BTN_LEFT)) {
		state->stick[0].axis[0] = -1;
	}
	else if (BUTTON(BTN_RIGHT)) {
		state->stick[0].axis[0] = 1;
	}
	else {
		state->stick[0].axis[0] = 0;
	}

	/* Up-down axis */
	if (BUTTON(BTN_UP)) {
		state->stick[0].axis[1] = -1;
	}
	else if (BUTTON(BTN_DOWN)) {
		state->stick[0].axis[1] = 1;
	}
	else {
		state->stick[0].axis[1] = 0;
	}

	/* Other buttons */
	state->button[0] = BUTTON(BTN_A);
	state->button[1] = BUTTON(BTN_B);
	state->button[2] = BUTTON(BTN_X);
	state->button[3] = BUTTON(BTN_Y);
	state->button[4] = BUTTON(BTN_L);
	state->button[5] = BUTTON(BTN_R);
	state->button[6] = BUTTON(BTN_MENU);
	state->button[7] = BUTTON(BTN_SELECT);
	state->button[8] = BUTTON(BTN_VOLUP);
	state->button[9] = BUTTON(BTN_VOLDOWN);
}


static void generate_axis_event(A5O_JOYSTICK *joy, int stick, int axis, float pos)
{
   A5O_EVENT event;

   if (!_al_event_source_needs_to_generate_event(&joy->es))
      return;

   event.joystick.type = A5O_EVENT_JOYSTICK_AXIS;
   event.joystick.timestamp = al_get_time();
   event.joystick.stick = stick;
   event.joystick.axis = axis;
   event.joystick.pos = pos;   event.joystick.button = 0;

   _al_event_source_emit_event(&joy->es, &event);
}


static void generate_button_event(A5O_JOYSTICK *joy, int button, A5O_EVENT_TYPE event_type)
{
   A5O_EVENT event;

   if (!_al_event_source_needs_to_generate_event(&joy->es))
      return;

   event.joystick.type = event_type;
   event.joystick.timestamp = al_get_time();
   event.joystick.stick = 0;
   event.joystick.axis = 0;
   event.joystick.pos = 0.0;
   event.joystick.button = button;

   _al_event_source_emit_event(&joy->es, &event);
}

static void *joywiz_thread_proc(A5O_THREAD *thread, void *unused)
{
	A5O_JOYSTICK_STATE oldstate;
	memset(&oldstate, 0, sizeof(A5O_JOYSTICK_STATE));

	(void)unused;
	
	while (!al_get_thread_should_stop(thread)) {
		joywiz_fill_joystate(&joystate);
		if (joystate.stick[0].axis[0] != oldstate.stick[0].axis[0]) {
			generate_axis_event(&joy, 0, 0,
				joystate.stick[0].axis[0]);
		}
		if (joystate.stick[0].axis[1] != oldstate.stick[0].axis[1]) {
			generate_axis_event(&joy, 0, 1,
				joystate.stick[0].axis[1]);
		}
		int i;
		for (i = 0; i < 10; i++) {
			A5O_EVENT_TYPE type;
			if (oldstate.button[i] == 0)
				type = A5O_EVENT_JOYSTICK_BUTTON_DOWN;
			else
				type = A5O_EVENT_JOYSTICK_BUTTON_UP;
			if (joystate.button[i] != oldstate.button[i]) {
				generate_button_event(&joy, i, type);
			}
		}
		oldstate = joystate;
		al_rest(1.0/POLLS_PER_SECOND);
	}

	return NULL;
}

static void joywiz_fill_joy(void)
{
	joy.info.num_sticks = 1;
	joy.info.num_buttons = 10;
	
	joy.info.stick[0].flags = A5O_JOYFLAG_DIGITAL;
	joy.info.stick[0].num_axes = 2;
	joy.info.stick[0].name = "Wiz D-pad";
	joy.info.stick[0].axis[0].name = "Left-right axis";
	joy.info.stick[0].axis[1].name = "Up-down axis";

	joy.info.button[0].name = "A";
	joy.info.button[1].name = "B";
	joy.info.button[2].name = "X";
	joy.info.button[3].name = "Y";
	joy.info.button[4].name = "L";
	joy.info.button[5].name = "R";
	joy.info.button[6].name = "Menu";
	joy.info.button[7].name = "Select";
	joy.info.button[8].name = "VolUp";
	joy.info.button[9].name = "VolDown";

	joy.num = 0;
}

static bool joywiz_init_joystick(void)
{
	lc_init_joy();

	joywiz_fill_joy();
	
	_al_event_source_init(&joy.es);

	memset(&joystate, 0, sizeof(A5O_JOYSTICK_STATE));

	wiz_joystick_thread = al_create_thread(joywiz_thread_proc, NULL);
	if (!wiz_joystick_thread) {
		return false;
	}

	al_start_thread(wiz_joystick_thread);
	
	return true;
}

static void joywiz_exit_joystick(void)
{
	al_join_thread(wiz_joystick_thread, NULL);
	al_destroy_thread(wiz_joystick_thread);
	_al_event_source_free(&joy.es);
}

static int joywiz_get_num_joysticks(void)
{
	return 1;
}

static A5O_JOYSTICK *joywiz_get_joystick(int num)
{
	(void)num; /* Only 1 supported now */
	return &joy;
}

static void joywiz_release_joystick(A5O_JOYSTICK *joy)
{
	(void)joy;
}

static void joywiz_get_joystick_state(A5O_JOYSTICK *joy, A5O_JOYSTICK_STATE *ret_state)
{
   _al_event_source_lock(&joy->es);
   {
      *ret_state = joystate;
   }
   _al_event_source_unlock(&joy->es);
}



/* the driver vtable */
A5O_JOYSTICK_DRIVER _al_joydrv_gp2xwiz =
{
   AL_JOY_TYPE_GP2XWIZ,
   "",
   "",
   "GP2X Wiz joystick",
   joywiz_init_joystick,
   joywiz_exit_joystick,
   joywiz_get_num_joysticks,
   joywiz_get_joystick,
   joywiz_release_joystick,
   joywiz_get_joystick_state
};


