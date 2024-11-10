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

#import <Carbon/Carbon.h>

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif



/* Dictionary to translate OS X modifier codes to Allegro modifier codes
 * and key codes.
 */
static unsigned const int mod_info[5][3] = {
   { NSAlphaShiftKeyMask, ALLEGRO_KEYMOD_CAPSLOCK, ALLEGRO_KEY_CAPSLOCK },
   { NSShiftKeyMask,      ALLEGRO_KEYMOD_SHIFT,    ALLEGRO_KEY_LSHIFT   },
   { NSControlKeyMask,    ALLEGRO_KEYMOD_CTRL,     ALLEGRO_KEY_LCTRL    },
   { NSAlternateKeyMask,  ALLEGRO_KEYMOD_ALT,      ALLEGRO_KEY_ALT      },
   { NSCommandKeyMask,    ALLEGRO_KEYMOD_COMMAND,  ALLEGRO_KEY_COMMAND  }
};



static bool osx_keyboard_init(void);
static void osx_keyboard_exit(void);
static ALLEGRO_KEYBOARD* osx_get_keyboard(void);
static ALLEGRO_KEYBOARD keyboard;
static ALLEGRO_KEYBOARD_STATE kbdstate;
static UInt32 dead_key_state;



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
void _al_osx_switch_keyboard_focus(ALLEGRO_DISPLAY *dpy, bool switch_in)
{
   _al_event_source_lock(&keyboard.es);

   if (switch_in)
      kbdstate.display = dpy;
   else
      kbdstate.display = NULL;

   _al_event_source_unlock(&keyboard.es);
}



static void _handle_key_press(ALLEGRO_DISPLAY* dpy, int unicode, int scancode,
   int modifiers, bool is_repeat)
{
   _al_event_source_lock(&keyboard.es);
   {
      /* Generate the press event if necessary. */
      if (_al_event_source_needs_to_generate_event(&keyboard.es)) {
         ALLEGRO_EVENT event;
         event.keyboard.type = ALLEGRO_EVENT_KEY_DOWN;
         event.keyboard.timestamp = al_get_time();
         event.keyboard.display   = dpy;
         event.keyboard.keycode   = scancode;
         event.keyboard.unichar   = 0;
         event.keyboard.modifiers = modifiers;
         event.keyboard.repeat    = false;
         if (!is_repeat) {
            _al_event_source_emit_event(&keyboard.es, &event);
         }
         if (unicode != 0) {
            if (unicode < 0) {
               unicode = 0;
            }
            event.keyboard.type = ALLEGRO_EVENT_KEY_CHAR;
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



static void _handle_key_release(ALLEGRO_DISPLAY* dpy, int modifiers, int scancode)
{
   _al_event_source_lock(&keyboard.es);
   {
      /* Generate the release event if necessary. */
      if (_al_event_source_needs_to_generate_event(&keyboard.es)) {
         ALLEGRO_EVENT event;
         event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
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
   /* 0x00 */ ALLEGRO_KEY_A,        ALLEGRO_KEY_S,          ALLEGRO_KEY_D,          ALLEGRO_KEY_F,
   /* 0x04 */ ALLEGRO_KEY_H,        ALLEGRO_KEY_G,          ALLEGRO_KEY_Z,          ALLEGRO_KEY_X,
   /* 0x08 */ ALLEGRO_KEY_C,        ALLEGRO_KEY_V,          ALLEGRO_KEY_BACKSLASH2, ALLEGRO_KEY_B,
   /* 0x0c */ ALLEGRO_KEY_Q,        ALLEGRO_KEY_W,          ALLEGRO_KEY_E,          ALLEGRO_KEY_R,
   /* 0x10 */ ALLEGRO_KEY_Y,        ALLEGRO_KEY_T,          ALLEGRO_KEY_1,          ALLEGRO_KEY_2,
   /* 0x14 */ ALLEGRO_KEY_3,        ALLEGRO_KEY_4,          ALLEGRO_KEY_6,          ALLEGRO_KEY_5,
   /* 0x18 */ ALLEGRO_KEY_EQUALS,   ALLEGRO_KEY_9,          ALLEGRO_KEY_7,          ALLEGRO_KEY_MINUS,
   /* 0x1c */ ALLEGRO_KEY_8,        ALLEGRO_KEY_0,          ALLEGRO_KEY_CLOSEBRACE, ALLEGRO_KEY_O,
   /* 0x20 */ ALLEGRO_KEY_U,        ALLEGRO_KEY_OPENBRACE,  ALLEGRO_KEY_I,          ALLEGRO_KEY_P,
   /* 0x24 */ ALLEGRO_KEY_ENTER,    ALLEGRO_KEY_L,          ALLEGRO_KEY_J,          ALLEGRO_KEY_QUOTE,
   /* 0x28 */ ALLEGRO_KEY_K,        ALLEGRO_KEY_SEMICOLON,  ALLEGRO_KEY_BACKSLASH,  ALLEGRO_KEY_COMMA,
   /* 0x2c */ ALLEGRO_KEY_SLASH,    ALLEGRO_KEY_N,          ALLEGRO_KEY_M,          ALLEGRO_KEY_FULLSTOP,
   /* 0x30 */ ALLEGRO_KEY_TAB,      ALLEGRO_KEY_SPACE,      ALLEGRO_KEY_BACKQUOTE,  ALLEGRO_KEY_BACKSPACE,
   /* 0x34 */ ALLEGRO_KEY_ENTER,    ALLEGRO_KEY_ESCAPE,     0,                      ALLEGRO_KEY_COMMAND,
   /* 0x38 */ ALLEGRO_KEY_LSHIFT,   ALLEGRO_KEY_CAPSLOCK,   ALLEGRO_KEY_ALT,        ALLEGRO_KEY_LEFT,
   /* 0x3c */ ALLEGRO_KEY_RIGHT,    ALLEGRO_KEY_DOWN,       ALLEGRO_KEY_UP,         0,
   /* 0x40 */ 0,                    ALLEGRO_KEY_PAD_DELETE, 0,                      ALLEGRO_KEY_PAD_ASTERISK,
   /* 0x44 */ 0,                    ALLEGRO_KEY_PAD_PLUS,   0,                      ALLEGRO_KEY_NUMLOCK,
   /* 0x48 */ 0,                    0,                      0,                      ALLEGRO_KEY_PAD_SLASH,
   /* 0x4c */ ALLEGRO_KEY_PAD_ENTER,0,                      ALLEGRO_KEY_PAD_MINUS,  0,
   /* 0x50 */ 0,                    ALLEGRO_KEY_PAD_EQUALS, ALLEGRO_KEY_PAD_0,      ALLEGRO_KEY_PAD_1,
   /* 0x54 */ ALLEGRO_KEY_PAD_2,    ALLEGRO_KEY_PAD_3,      ALLEGRO_KEY_PAD_4,      ALLEGRO_KEY_PAD_5,
   /* 0x58 */ ALLEGRO_KEY_PAD_6,    ALLEGRO_KEY_PAD_7,      0,                      ALLEGRO_KEY_PAD_8,
   /* 0x5c */ ALLEGRO_KEY_PAD_9,    0,                      0,                      0,
   /* 0x60 */ ALLEGRO_KEY_F5,       ALLEGRO_KEY_F6,         ALLEGRO_KEY_F7,         ALLEGRO_KEY_F3,
   /* 0x64 */ ALLEGRO_KEY_F8,       ALLEGRO_KEY_F9,         0,                      ALLEGRO_KEY_F11,
   /* 0x68 */ 0,                    ALLEGRO_KEY_PRINTSCREEN,0,                      ALLEGRO_KEY_SCROLLLOCK,
   /* 0x6c */ 0,                    ALLEGRO_KEY_F10,        0,                      ALLEGRO_KEY_F12,
   /* 0x70 */ 0,                    ALLEGRO_KEY_PAUSE,      ALLEGRO_KEY_INSERT,     ALLEGRO_KEY_HOME,
   /* 0x74 */ ALLEGRO_KEY_PGUP,     ALLEGRO_KEY_DELETE,     ALLEGRO_KEY_F4,         ALLEGRO_KEY_END,
   /* 0x78 */ ALLEGRO_KEY_F2,       ALLEGRO_KEY_PGDN,       ALLEGRO_KEY_F1,         ALLEGRO_KEY_LEFT,
   /* 0x7c */ ALLEGRO_KEY_RIGHT,    ALLEGRO_KEY_DOWN,       ALLEGRO_KEY_UP,         0
};



/* get_state:
 *  Copy a snapshot of the keyboard state into the user's structure
 */
static void get_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
   _al_event_source_lock(&keyboard.es);
   {
      memcpy(ret_state, &kbdstate, sizeof(ALLEGRO_KEYBOARD_STATE));
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



static ALLEGRO_KEYBOARD_DRIVER keyboard_macosx =
{
   KEYBOARD_MACOSX,
   "",
   "",
   "MacOS X keyboard",
   osx_keyboard_init,
   osx_keyboard_exit,
   osx_get_keyboard,
   NULL, // ALLEGRO_METHOD(bool, set_leds, (int leds));
   NULL, // ALLEGRO_METHOD(const char *, keycode_to_name, (int keycode));
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
ALLEGRO_KEYBOARD_DRIVER* _al_osx_get_keyboard_driver(void)
{
   return &keyboard_macosx;
}



/* _al_osx_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void _al_osx_keyboard_handler(int pressed, NSEvent *event, ALLEGRO_DISPLAY* dpy)
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
      int32_t unichar = 0;
      bool new_input = _al_get_keyboard_compat_version() >= AL_ID(5, 2, 10, 0);
      NSString *raw_characters = [event charactersIgnoringModifiers];
      NSString *characters = [event characters];
      UniChar raw_character = ([raw_characters length] > 0) ? [raw_characters characterAtIndex: 0] : 0;
      UniChar character = ([characters length] > 0) ? [characters characterAtIndex: 0] : 0;

      if (new_input) {
         /* https://stackoverflow.com/a/22677690 */
         TISInputSourceRef keyboard_input = TISCopyCurrentKeyboardInputSource();
         CFDataRef layout_data = TISGetInputSourceProperty(keyboard_input, kTISPropertyUnicodeKeyLayoutData);
         const UCKeyboardLayout *layout = (const UCKeyboardLayout *)CFDataGetBytePtr(layout_data);

         CGEventFlags modifier_flags = [event modifierFlags];
         UInt32 modifier_key_state = (modifier_flags >> 16) & 0xff;

         UniChar unicode_string[5];
         unicode_string[4] = 0;
         UniCharCount unicode_length;

         UCKeyTranslate(layout,
                        [event keyCode],
                        kUCKeyActionDown,
                        modifier_key_state,
                        LMGetKbdType(),
                        0,
                        &dead_key_state,
                        4,
                        &unicode_length,
                        unicode_string);

         if (unicode_length > 0) {
            ALLEGRO_USTR *ustr = al_ustr_new_from_utf16(unicode_string);
            unichar = al_ustr_get(ustr, 0);
            if (unichar < 0)
               unichar = 0;
            /* For some reason, pad enter sends a ^C. */
            if (scancode == ALLEGRO_KEY_PAD_ENTER && unichar == 3)
               unichar = '\r';
            /* Single out the few printable characters under 32 */
            if (unichar < ' ' && (unichar != '\r' && unichar != '\t' && unichar != '\b'))
               unichar = 0;
            al_ustr_free(ustr);
         }
         CFRelease(keyboard_input);
      }
      else
         unichar = character;

      /* Apple maps function, arrow, and other keys to Unicode points.
         We want to generate CHAR events for them, so we'll override the translation logic.
        _handle_key_press will set the unichar back to 0 for these keys. */
      if (character >= 0xF700 && character <= 0xF747)
         unichar = -1;
      /* The delete key. */
      if (character == 0xF728 && new_input)
         unichar = 127;
      /* Special processing to send character 1 for CTRL-A, 2 for CTRL-B etc. */
      if ((key_shifts & ALLEGRO_KEYMOD_CTRL) && (isalpha(raw_character)))
         unichar = tolower(raw_character) - 'a' + 1;
      bool is_repeat = pressed ? ([event isARepeat] == YES) : false;
      _handle_key_press(dpy, unichar, scancode, key_shifts, is_repeat);
   }
   else {
      _handle_key_release(dpy, key_shifts, scancode);
   }
}



/* _al_osx_keyboard_modifier:
 *  Handles keyboard modifiers changes.
 */
void _al_osx_keyboard_modifiers(unsigned int modifiers, ALLEGRO_DISPLAY* dpy)
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
            _handle_key_press(dpy, 0, mod_info[i][2], key_shifts, false);
            if (i == 0) {
               /* Caps lock requires special handling */
               _handle_key_release(dpy, key_shifts, mod_info[0][2]);
            }
         }
         else {
            if (i == 0) {
               _handle_key_press(dpy, 0, mod_info[0][2], key_shifts, false);
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
static ALLEGRO_KEYBOARD* osx_get_keyboard(void)
{
   return &keyboard;
}


/* vim: set sts=3 sw=3 et: */
