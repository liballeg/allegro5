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
*      MacOS X keyboard module.
*
*      By Angelo Mottola.
*
*      Based on Unix/X11 version by Michael Bukin.
*
*      See readme.txt for copyright information.
*/


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif

static bool osx_keyboard_init(void);
static void osx_keyboard_exit(void);
static ALLEGRO_KEYBOARD* osx_get_keyboard(void);
static ALLEGRO_KEYBOARD keyboard;
static ALLEGRO_KEYBOARD_STATE kbdstate;

/* _al_osx_switch_keyboard_focus:
 *  Handle a focus switch event.
 */
void _al_osx_switch_keyboard_focus(ALLEGRO_DISPLAY *dpy, bool switch_in)
{
	_al_event_source_lock(&keyboard.es);

   if (switch_in)
      kbdstate.display = dpy;
   else
      kbdstate.display = NULL;

	_al_event_source_unlock(&keyboard.es);	
}

static void _handle_key_press(ALLEGRO_DISPLAY* dpy, int unicode, int scancode, int modifiers) {
	int is_repeat=0;
	int type;
	_al_event_source_lock(&keyboard.es);
	{
		/* Generate the press event if necessary. */
		type = is_repeat ? ALLEGRO_EVENT_KEY_REPEAT : ALLEGRO_EVENT_KEY_DOWN;
		if (_al_event_source_needs_to_generate_event(&keyboard.es))
		{
			 ALLEGRO_EVENT event;
			 event.keyboard.type = type;
			 event.keyboard.timestamp = al_current_time();
			 event.keyboard.display   = dpy;
			 event.keyboard.keycode   = scancode;
			 event.keyboard.unichar   = unicode;
			 event.keyboard.modifiers = modifiers;
			 _al_event_source_emit_event(&keyboard.es, &event);
		}
	}
   /* Maintain the kbdstate array. */
   _AL_KEYBOARD_STATE_SET_KEY_DOWN(kbdstate, scancode);
	_al_event_source_unlock(&keyboard.es);	
}
static void _handle_key_release(ALLEGRO_DISPLAY* dpy, int scancode) {
	_al_event_source_lock(&keyboard.es);
	{
		/* Generate the release event if necessary. */
		if (_al_event_source_needs_to_generate_event(&keyboard.es))
		{
			ALLEGRO_EVENT event;
			event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
			event.keyboard.timestamp = al_current_time();
			event.keyboard.display   = dpy;
			event.keyboard.keycode   = scancode;
			event.keyboard.unichar   = 0;
			event.keyboard.modifiers = 0;
			_al_event_source_emit_event(&keyboard.es, &event);
		}
	}
   /* Maintain the kbdstate array. */
   _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(kbdstate, scancode);
	_al_event_source_unlock(&keyboard.es);
}
/* Mac keycode to Allegro scancode conversion table */
static const int mac_to_scancode[128] = {
	/* 0x00 */ ALLEGRO_KEY_A,          ALLEGRO_KEY_S,          ALLEGRO_KEY_D,          ALLEGRO_KEY_F,
	/* 0x04 */ ALLEGRO_KEY_H,          ALLEGRO_KEY_G,          ALLEGRO_KEY_Z,          ALLEGRO_KEY_X,
	/* 0x08 */ ALLEGRO_KEY_C,          ALLEGRO_KEY_V,          0,              ALLEGRO_KEY_B,
	/* 0x0c */ ALLEGRO_KEY_Q,          ALLEGRO_KEY_W,          ALLEGRO_KEY_E,          ALLEGRO_KEY_R,
	/* 0x10 */ ALLEGRO_KEY_Y,          ALLEGRO_KEY_T,          ALLEGRO_KEY_1,          ALLEGRO_KEY_2,
	/* 0x14 */ ALLEGRO_KEY_3,          ALLEGRO_KEY_4,          ALLEGRO_KEY_6,          ALLEGRO_KEY_5,
	/* 0x18 */ ALLEGRO_KEY_EQUALS,     ALLEGRO_KEY_9,          ALLEGRO_KEY_7,          ALLEGRO_KEY_MINUS,
	/* 0x1c */ ALLEGRO_KEY_8,          ALLEGRO_KEY_0,          ALLEGRO_KEY_CLOSEBRACE, ALLEGRO_KEY_O,
	/* 0x20 */ ALLEGRO_KEY_U,          ALLEGRO_KEY_OPENBRACE,  ALLEGRO_KEY_I,          ALLEGRO_KEY_P,
	/* 0x24 */ ALLEGRO_KEY_ENTER,      ALLEGRO_KEY_L,          ALLEGRO_KEY_J,          ALLEGRO_KEY_QUOTE,
	/* 0x28 */ ALLEGRO_KEY_K,          ALLEGRO_KEY_SEMICOLON,  ALLEGRO_KEY_BACKSLASH,  ALLEGRO_KEY_COMMA,
	/* 0x2c */ ALLEGRO_KEY_SLASH,      ALLEGRO_KEY_N,          ALLEGRO_KEY_M,          ALLEGRO_KEY_FULLSTOP,
	/* 0x30 */ ALLEGRO_KEY_TAB,        ALLEGRO_KEY_SPACE,      ALLEGRO_KEY_BACKQUOTE,  ALLEGRO_KEY_BACKSPACE,
	/* 0x34 */ ALLEGRO_KEY_ENTER,      ALLEGRO_KEY_ESCAPE,        0,              ALLEGRO_KEY_COMMAND,
	/* 0x38 */ ALLEGRO_KEY_LSHIFT,     ALLEGRO_KEY_CAPSLOCK,   ALLEGRO_KEY_ALT,        ALLEGRO_KEY_LEFT,
	/* 0x3c */ ALLEGRO_KEY_RIGHT,      ALLEGRO_KEY_DOWN,       ALLEGRO_KEY_UP,         0,
	/* 0x40 */ 0,              ALLEGRO_KEY_FULLSTOP,       0,              ALLEGRO_KEY_PAD_ASTERISK,
	/* 0x44 */ 0,              ALLEGRO_KEY_PAD_PLUS,   0,              ALLEGRO_KEY_NUMLOCK,
	/* 0x48 */ 0,              0,              0,              ALLEGRO_KEY_PAD_SLASH,
	/* 0x4c */ ALLEGRO_KEY_PAD_ENTER,  0,              ALLEGRO_KEY_PAD_MINUS,   0,
	/* 0x50 */ 0,              ALLEGRO_KEY_EQUALS_PAD, ALLEGRO_KEY_PAD_0,      ALLEGRO_KEY_PAD_1,
	/* 0x54 */ ALLEGRO_KEY_PAD_2,      ALLEGRO_KEY_PAD_3,      ALLEGRO_KEY_PAD_4,      ALLEGRO_KEY_PAD_5,
	/* 0x58 */ ALLEGRO_KEY_PAD_6,      ALLEGRO_KEY_PAD_7,      0,              ALLEGRO_KEY_PAD_8,
	/* 0x5c */ ALLEGRO_KEY_PAD_9,      0,              0,              0,
	/* 0x60 */ ALLEGRO_KEY_F5,         ALLEGRO_KEY_F6,         ALLEGRO_KEY_F7,         ALLEGRO_KEY_F3,
	/* 0x64 */ ALLEGRO_KEY_F8,         ALLEGRO_KEY_F9,         0,              ALLEGRO_KEY_F11,
	/* 0x68 */ 0,              ALLEGRO_KEY_PRINTSCREEN,     0,              ALLEGRO_KEY_SCROLLLOCK,
	/* 0x6c */ 0,              ALLEGRO_KEY_F10,        0,              ALLEGRO_KEY_F12,
	/* 0x70 */ 0,              ALLEGRO_KEY_PAUSE,      ALLEGRO_KEY_INSERT,     ALLEGRO_KEY_HOME,
	/* 0x74 */ ALLEGRO_KEY_PGUP,       ALLEGRO_KEY_DELETE,        ALLEGRO_KEY_F4,         ALLEGRO_KEY_END,
	/* 0x78 */ ALLEGRO_KEY_F2,         ALLEGRO_KEY_PGDN,       ALLEGRO_KEY_F1,         ALLEGRO_KEY_LEFT,
	/* 0x7c */ ALLEGRO_KEY_RIGHT,      ALLEGRO_KEY_DOWN,       ALLEGRO_KEY_UP,         0
};

/* get_state:
 * Copy a snapshot of the keyboard state into the user's structure
 */
static void get_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
   _al_event_source_lock(&keyboard.es);
   {
      memcpy(ret_state, &kbdstate, sizeof(ALLEGRO_KEYBOARD_STATE));
   }
   _al_event_source_unlock(&keyboard.es);
}

static unsigned int old_mods = 0;

ALLEGRO_KEYBOARD_DRIVER keyboard_macosx =
{
	KEYBOARD_MACOSX, //int  id;
	"",//   AL_CONST char *name;
	"", // AL_CONST char *desc;
	"MacOS X keyboard",// AL_CONST char *ascii_name;
	osx_keyboard_init, // ALLEGRO_METHOD(bool, init, (void));
	osx_keyboard_exit, // ALLEGRO_METHOD(void, exit, (void));
	osx_get_keyboard, // ALLEGRO_METHOD(ALLEGRO_KEYBOARD*, get_keyboard, (void));
	NULL, // ALLEGRO_METHOD(bool, set_leds, (int leds));
	NULL, // ALLEGRO_METHOD(AL_CONST char *, keycode_to_name, (int keycode));
	get_state, // ALLEGRO_METHOD(void, get_state, (ALLEGRO_KEYBOARD_STATE *ret_state));
};
_DRIVER_INFO _al_keyboard_driver_list[] =
{
	{ KEYBOARD_MACOSX,         &keyboard_macosx,          1 },
	{ 0,                       NULL,                     0     }
};

ALLEGRO_KEYBOARD_DRIVER* _al_osx_get_keyboard_driver(void) {
	return &keyboard_macosx;
}
/* _al_osx_keyboard_handler:
*  Keyboard "interrupt" handler.
*/
void _al_osx_keyboard_handler(int pressed, NSEvent *event, ALLEGRO_DISPLAY* dpy)
{
   /* We need to distinguish between the raw character code (needed for
    * ctrl and alt) and the "shifted" caharacter code when neither of these
    * is held down. This is needed to get the correct behavior when caps
    * lock is on (ie, letters are upper case)
    */
	const char raw_character = [[event charactersIgnoringModifiers] characterAtIndex: 0];
	const char upper_character = [[event characters] characterAtIndex: 0];
	int scancode = mac_to_scancode[[event keyCode]];
	int modifiers = [event modifierFlags];
	
	if (pressed) {
		if (modifiers & NSAlternateKeyMask)
			_handle_key_press(dpy, 0, scancode, ALLEGRO_KEYMOD_ALT);
		else {
			if ((modifiers & NSControlKeyMask) && (isalpha(raw_character)))
				_handle_key_press(dpy, tolower(raw_character) - 'a' + 1, scancode, ALLEGRO_KEYMOD_CTRL);
			else
				_handle_key_press(dpy, upper_character, scancode, 0);
		}
//		if ((three_finger_flag) &&
//			(scancode == KEY_END) && (_key_shifts & (KB_CTRL_FLAG | KB_ALT_FLAG))) {
//			raise(SIGTERM);
//		}
	}
	else
		_handle_key_release(dpy, scancode);
}



/* osx_keyboard_modifier:
*  Handles keyboard modifiers changes.
*/
void _al_osx_keyboard_modifiers(unsigned int mods, ALLEGRO_DISPLAY* dpy)
{
	unsigned const int mod_info[5][3] = { { NSAlphaShiftKeyMask, ALLEGRO_KEYMOD_CAPSLOCK, ALLEGRO_KEY_CAPSLOCK },
	{ NSShiftKeyMask,      ALLEGRO_KEYMOD_SHIFT,    ALLEGRO_KEY_LSHIFT   },
	{ NSControlKeyMask,    ALLEGRO_KEYMOD_CTRL,     ALLEGRO_KEY_LCTRL },
	{ NSAlternateKeyMask,  ALLEGRO_KEYMOD_ALT,      ALLEGRO_KEY_ALT      },
	{ NSCommandKeyMask,    ALLEGRO_KEYMOD_COMMAND,  ALLEGRO_KEY_COMMAND  } };
	int i, changed;
	
	for (i = 0; i < 5; i++) {
		changed = (mods ^ old_mods) & mod_info[i][0];
		if (changed) {
			if (mods & mod_info[i][0]) {
//				_key_shifts |= mod_info[i][1];
				_handle_key_press(dpy, -1, mod_info[i][2], 0);
				if (i == 0)
					/* Caps lock requires special handling */
					_handle_key_release(dpy, mod_info[0][2]);
			}
			else {
//				_key_shifts &= ~mod_info[i][1];
				if (i == 0)
					_handle_key_press(dpy, -1, mod_info[0][2], 0);
				_handle_key_release(dpy, mod_info[i][2]);
			}
		}
	}
	old_mods = mods;
}



/* osx_keyboard_init:
*  Installs the keyboard handler.
*/
static bool osx_keyboard_init(void)
{
	memset(&keyboard, 0, sizeof keyboard);
   _al_osx_keyboard_was_installed(YES);
	_al_event_source_init(&keyboard.es);
	return true;
}



/* osx_keyboard_exit:
*  Removes the keyboard handler.
*/
static void osx_keyboard_exit(void)
{
	_al_event_source_free(&keyboard.es);
   _al_osx_keyboard_was_installed(NO);
}

/* osx_get_keyboard:
* returns the keyboard object
*/
static ALLEGRO_KEYBOARD* osx_get_keyboard(void)
{
	return &keyboard;
}


/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
