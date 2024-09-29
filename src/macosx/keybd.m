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


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"

#ifndef A5O_MACOSX
#error Something is wrong with the makefile
#endif



/* Dictionary to translate OS X modifier codes to Allegro modifier codes
 * and key codes.
 */
static unsigned const int mod_info[5][3] = {
   { NSAlphaShiftKeyMask, A5O_KEYMOD_CAPSLOCK, A5O_KEY_CAPSLOCK },
   { NSShiftKeyMask,      A5O_KEYMOD_SHIFT,    A5O_KEY_LSHIFT   },
   { NSControlKeyMask,    A5O_KEYMOD_CTRL,     A5O_KEY_LCTRL    },
   { NSAlternateKeyMask,  A5O_KEYMOD_ALT,      A5O_KEY_ALT      },
   { NSCommandKeyMask,    A5O_KEYMOD_COMMAND,  A5O_KEY_COMMAND  }
};



static bool osx_keyboard_init(void);
static void osx_keyboard_exit(void);
static A5O_KEYBOARD* osx_get_keyboard(void);
static A5O_KEYBOARD keyboard;
static A5O_KEYBOARD_STATE kbdstate;



/* translate_modifier_flags:
 *  Translate a bitmask of OS X modifier flags to Allegro's modifier flags
 */
static int translate_modifier_flags(int osx_mods)
{
   int allegro_mods = 0;
   int i;

   for (i = 0; i < 5; i++) {
      if (osx_mods & mod_info[i][0])
         allegro_mods |= mod_info[i][1];
   }

   return allegro_mods;
}



/* _al_osx_switch_keyboard_focus:
 *  Handle a focus switch event.
 */
void _al_osx_switch_keyboard_focus(A5O_DISPLAY *dpy, bool switch_in)
{
   _al_event_source_lock(&keyboard.es);

   if (switch_in)
      kbdstate.display = dpy;
   else
      kbdstate.display = NULL;

   _al_event_source_unlock(&keyboard.es);
}



static void _handle_key_press(A5O_DISPLAY* dpy, int unicode, int scancode,
   int modifiers, bool is_repeat)
{
   _al_event_source_lock(&keyboard.es);
   {
      /* Generate the press event if necessary. */
      if (_al_event_source_needs_to_generate_event(&keyboard.es)) {
         A5O_EVENT event;
         event.keyboard.type = A5O_EVENT_KEY_DOWN;
         event.keyboard.timestamp = al_get_time();
         event.keyboard.display   = dpy;
         event.keyboard.keycode   = scancode;
         event.keyboard.unichar   = 0;
         event.keyboard.modifiers = modifiers;
         event.keyboard.repeat    = false;
         if (!is_repeat) {
            _al_event_source_emit_event(&keyboard.es, &event);
         }
         if (unicode > 0) {
            /* Apple maps function, arrow, and other keys to Unicode points. */
            /* http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/CORPCHAR.TXT */
            if (unicode >= 0xF700 && unicode <= 0xF747) {
               unicode = 0;
            }
            event.keyboard.type = A5O_EVENT_KEY_CHAR;
            event.keyboard.unichar = unicode;
            event.keyboard.modifiers = modifiers;
            event.keyboard.repeat = is_repeat;
            _al_event_source_emit_event(&keyboard.es, &event);
         }
      }
   }
   /* Maintain the kbdstate array. */
   _AL_KEYBOARD_STATE_SET_KEY_DOWN(kbdstate, scancode);
   _al_event_source_unlock(&keyboard.es);
}



static void _handle_key_release(A5O_DISPLAY* dpy, int modifiers, int scancode)
{
   _al_event_source_lock(&keyboard.es);
   {
      /* Generate the release event if necessary. */
      if (_al_event_source_needs_to_generate_event(&keyboard.es)) {
         A5O_EVENT event;
         event.keyboard.type = A5O_EVENT_KEY_UP;
         event.keyboard.timestamp = al_get_time();
         event.keyboard.display   = dpy;
         event.keyboard.keycode   = scancode;
         event.keyboard.unichar   = 0;
         event.keyboard.modifiers = modifiers;
         _al_event_source_emit_event(&keyboard.es, &event);
      }
   }
   /* Maintain the kbdstate array. */
   _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(kbdstate, scancode);
   _al_event_source_unlock(&keyboard.es);
}



/* Mac keycode to Allegro scancode conversion table */
static const int mac_to_scancode[128] =
{
   /* 0x00 */ A5O_KEY_A,        A5O_KEY_S,          A5O_KEY_D,          A5O_KEY_F,
   /* 0x04 */ A5O_KEY_H,        A5O_KEY_G,          A5O_KEY_Z,          A5O_KEY_X,
   /* 0x08 */ A5O_KEY_C,        A5O_KEY_V,          0,                      A5O_KEY_B,
   /* 0x0c */ A5O_KEY_Q,        A5O_KEY_W,          A5O_KEY_E,          A5O_KEY_R,
   /* 0x10 */ A5O_KEY_Y,        A5O_KEY_T,          A5O_KEY_1,          A5O_KEY_2,
   /* 0x14 */ A5O_KEY_3,        A5O_KEY_4,          A5O_KEY_6,          A5O_KEY_5,
   /* 0x18 */ A5O_KEY_EQUALS,   A5O_KEY_9,          A5O_KEY_7,          A5O_KEY_MINUS,
   /* 0x1c */ A5O_KEY_8,        A5O_KEY_0,          A5O_KEY_CLOSEBRACE, A5O_KEY_O,
   /* 0x20 */ A5O_KEY_U,        A5O_KEY_OPENBRACE,  A5O_KEY_I,          A5O_KEY_P,
   /* 0x24 */ A5O_KEY_ENTER,    A5O_KEY_L,          A5O_KEY_J,          A5O_KEY_QUOTE,
   /* 0x28 */ A5O_KEY_K,        A5O_KEY_SEMICOLON,  A5O_KEY_BACKSLASH,  A5O_KEY_COMMA,
   /* 0x2c */ A5O_KEY_SLASH,    A5O_KEY_N,          A5O_KEY_M,          A5O_KEY_FULLSTOP,
   /* 0x30 */ A5O_KEY_TAB,      A5O_KEY_SPACE,      A5O_KEY_BACKQUOTE,  A5O_KEY_BACKSPACE,
   /* 0x34 */ A5O_KEY_ENTER,    A5O_KEY_ESCAPE,     0,                      A5O_KEY_COMMAND,
   /* 0x38 */ A5O_KEY_LSHIFT,   A5O_KEY_CAPSLOCK,   A5O_KEY_ALT,        A5O_KEY_LEFT,
   /* 0x3c */ A5O_KEY_RIGHT,    A5O_KEY_DOWN,       A5O_KEY_UP,         0,
   /* 0x40 */ 0,                    A5O_KEY_FULLSTOP,   0,                      A5O_KEY_PAD_ASTERISK,
   /* 0x44 */ 0,                    A5O_KEY_PAD_PLUS,   0,                      A5O_KEY_NUMLOCK,
   /* 0x48 */ 0,                    0,                      0,                      A5O_KEY_PAD_SLASH,
   /* 0x4c */ A5O_KEY_PAD_ENTER,0,                      A5O_KEY_PAD_MINUS,  0,
   /* 0x50 */ 0,                    A5O_KEY_PAD_EQUALS, A5O_KEY_PAD_0,      A5O_KEY_PAD_1,
   /* 0x54 */ A5O_KEY_PAD_2,    A5O_KEY_PAD_3,      A5O_KEY_PAD_4,      A5O_KEY_PAD_5,
   /* 0x58 */ A5O_KEY_PAD_6,    A5O_KEY_PAD_7,      0,                      A5O_KEY_PAD_8,
   /* 0x5c */ A5O_KEY_PAD_9,    0,                      0,                      0,
   /* 0x60 */ A5O_KEY_F5,       A5O_KEY_F6,         A5O_KEY_F7,         A5O_KEY_F3,
   /* 0x64 */ A5O_KEY_F8,       A5O_KEY_F9,         0,                      A5O_KEY_F11,
   /* 0x68 */ 0,                    A5O_KEY_PRINTSCREEN,0,                      A5O_KEY_SCROLLLOCK,
   /* 0x6c */ 0,                    A5O_KEY_F10,        0,                      A5O_KEY_F12,
   /* 0x70 */ 0,                    A5O_KEY_PAUSE,      A5O_KEY_INSERT,     A5O_KEY_HOME,
   /* 0x74 */ A5O_KEY_PGUP,     A5O_KEY_DELETE,     A5O_KEY_F4,         A5O_KEY_END,
   /* 0x78 */ A5O_KEY_F2,       A5O_KEY_PGDN,       A5O_KEY_F1,         A5O_KEY_LEFT,
   /* 0x7c */ A5O_KEY_RIGHT,    A5O_KEY_DOWN,       A5O_KEY_UP,         0
};



/* get_state:
 *  Copy a snapshot of the keyboard state into the user's structure
 */
static void get_state(A5O_KEYBOARD_STATE *ret_state)
{
   _al_event_source_lock(&keyboard.es);
   {
      memcpy(ret_state, &kbdstate, sizeof(A5O_KEYBOARD_STATE));
   }
   _al_event_source_unlock(&keyboard.es);
}



/* clear_state:
 *  Clear the keyboard state
 */
static void clear_state(void)
{
   _al_event_source_lock(&keyboard.es);
   {
      memset(&kbdstate, 0, sizeof(kbdstate));
   }
   _al_event_source_unlock(&keyboard.es);
}



static A5O_KEYBOARD_DRIVER keyboard_macosx =
{
   KEYBOARD_MACOSX,
   "",
   "",
   "MacOS X keyboard",
   osx_keyboard_init,
   osx_keyboard_exit,
   osx_get_keyboard,
   NULL, // A5O_METHOD(bool, set_leds, (int leds));
   NULL, // A5O_METHOD(const char *, keycode_to_name, (int keycode));
   get_state,
   clear_state,
};



_AL_DRIVER_INFO _al_keyboard_driver_list[] =
{
   { KEYBOARD_MACOSX,   &keyboard_macosx,    1 },
   { 0,                 NULL,                0 }
};



/* _al_osx_get_keyboard_driver:
 *  Returns the keyboard driver.
 */
A5O_KEYBOARD_DRIVER* _al_osx_get_keyboard_driver(void)
{
   return &keyboard_macosx;
}



/* _al_osx_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void _al_osx_keyboard_handler(int pressed, NSEvent *event, A5O_DISPLAY* dpy)
{
   /* We need to distinguish between the raw character code (needed for
    * ctrl and alt) and the "shifted" character code when neither of these
    * is held down. This is needed to get the correct behavior when caps
    * lock is on (ie, letters are upper case)
    */
   int scancode = mac_to_scancode[[event keyCode]];
   
   /* Translate OS X modifier flags to Allegro modifier flags */
   int key_shifts = translate_modifier_flags([event modifierFlags]);

   if (pressed) {     
      NSString* raw_characters = [event charactersIgnoringModifiers];
      NSString* upper_characters = [event characters];
      const unichar raw_character = ([raw_characters length] > 0) ? [raw_characters characterAtIndex: 0] : 0;
      const unichar upper_character =([upper_characters length] > 0) ?  [upper_characters characterAtIndex: 0] : 0;
      bool is_repeat = pressed ? ([event isARepeat] == YES) : false;
      /* Special processing to send character 1 for CTRL-A, 2 for CTRL-B etc. */
      if ((key_shifts & A5O_KEYMOD_CTRL) && (isalpha(raw_character)))
         _handle_key_press(dpy, tolower(raw_character) - 'a' + 1, scancode, key_shifts, is_repeat);
      else
         _handle_key_press(dpy, upper_character, scancode, key_shifts, is_repeat);
   }
   else {
      _handle_key_release(dpy, key_shifts, scancode);
   }
}



/* _al_osx_keyboard_modifier:
 *  Handles keyboard modifiers changes.
 */
void _al_osx_keyboard_modifiers(unsigned int modifiers, A5O_DISPLAY* dpy)
{
   static unsigned int old_modifiers = 0;
   int i, changed;
   int key_shifts;

   /* Translate OS X modifier flags to Allegro modifier flags */
   key_shifts = translate_modifier_flags(modifiers);

   for (i = 0; i < 5; i++) {
      changed = (modifiers ^ old_modifiers) & mod_info[i][0];
      if (changed) {
         if (modifiers & mod_info[i][0]) {
            _handle_key_press(dpy, -1, mod_info[i][2], key_shifts, false);
            if (i == 0) {
               /* Caps lock requires special handling */
               _handle_key_release(dpy, key_shifts, mod_info[0][2]);
            }
         }
         else {
            if (i == 0) {
               _handle_key_press(dpy, -1, mod_info[0][2], key_shifts, false);
            }
            
            _handle_key_release(dpy, key_shifts, mod_info[i][2]);
         }
      }
   }
   old_modifiers = modifiers;
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
 *  Returns the keyboard object.
 */
static A5O_KEYBOARD* osx_get_keyboard(void)
{
   return &keyboard;
}


/* vim: set sts=3 sw=3 et: */
