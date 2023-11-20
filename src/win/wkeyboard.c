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
 *      Windows keyboard driver.
 *
 *      By Milan Mimica.
 *
 *      See readme.txt for copyright information.
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintwin.h"

/* Missing from MSVC 2005 headers. */
#ifndef MAPVK_VSC_TO_VK_EX
   #define MAPVK_VSC_TO_VK_EX 3
#endif

static bool installed = false;
static ALLEGRO_KEYBOARD the_keyboard;
static ALLEGRO_KEYBOARD_STATE the_state;
static int modifiers = 0;

/* lookup table for converting virtualkey VK_* codes into Allegro ALLEGRO_KEY_* codes */
/* For handling of extended keys, extkey_to_keycode() takes priority over this. */
/* Last unknown key sequence: 39*/
static const unsigned char hw_to_mycode[256] =
{
   /* 0x00 */    0,                       ALLEGRO_KEY_UNKNOWN+0,     ALLEGRO_KEY_UNKNOWN+1,      ALLEGRO_KEY_UNKNOWN+2,
   /* 0x04 */    ALLEGRO_KEY_UNKNOWN+3,   ALLEGRO_KEY_UNKNOWN+4,     ALLEGRO_KEY_UNKNOWN+5,      0,
   /* 0x08 */    ALLEGRO_KEY_BACKSPACE,   ALLEGRO_KEY_TAB,           0,                          0,
   /* 0x0C */    ALLEGRO_KEY_PAD_5,       ALLEGRO_KEY_ENTER,         0,                          0,
   /* 0x10 */    0/*L or R shift*/,       ALLEGRO_KEY_LCTRL,         ALLEGRO_KEY_ALT,            ALLEGRO_KEY_PAUSE,
   /* 0x14 */    ALLEGRO_KEY_CAPSLOCK,    ALLEGRO_KEY_KANA,          0,                          ALLEGRO_KEY_UNKNOWN+6,
   /* 0x18 */    ALLEGRO_KEY_UNKNOWN+7,   ALLEGRO_KEY_KANJI,         0,                          ALLEGRO_KEY_ESCAPE,
   /* 0x1C */    ALLEGRO_KEY_CONVERT,     ALLEGRO_KEY_NOCONVERT,     ALLEGRO_KEY_UNKNOWN+8,      ALLEGRO_KEY_UNKNOWN+9,
   /* 0x20 */    ALLEGRO_KEY_SPACE,       ALLEGRO_KEY_PAD_9,         ALLEGRO_KEY_PAD_3,          ALLEGRO_KEY_PAD_1,
   /* 0x24 */    ALLEGRO_KEY_PAD_7,       ALLEGRO_KEY_PAD_4,         ALLEGRO_KEY_PAD_8,          ALLEGRO_KEY_PAD_6,
   /* 0x28 */    ALLEGRO_KEY_PAD_2,       ALLEGRO_KEY_UNKNOWN+10,    ALLEGRO_KEY_UNKNOWN+11,     ALLEGRO_KEY_UNKNOWN+12,
   /* 0x2C */    ALLEGRO_KEY_PRINTSCREEN, ALLEGRO_KEY_PAD_0,         ALLEGRO_KEY_PAD_DELETE,     ALLEGRO_KEY_UNKNOWN+13,
   /* 0x30 */    ALLEGRO_KEY_0,           ALLEGRO_KEY_1,             ALLEGRO_KEY_2,              ALLEGRO_KEY_3,
   /* 0x34 */    ALLEGRO_KEY_4,           ALLEGRO_KEY_5,             ALLEGRO_KEY_6,              ALLEGRO_KEY_7,
   /* 0x38 */    ALLEGRO_KEY_8,           ALLEGRO_KEY_9,             0,                          0,
   /* 0x3C */    0,                       0,                         0,                          0,
   /* 0x40 */    0,                       ALLEGRO_KEY_A,             ALLEGRO_KEY_B,              ALLEGRO_KEY_C,
   /* 0x44 */    ALLEGRO_KEY_D,           ALLEGRO_KEY_E,             ALLEGRO_KEY_F,              ALLEGRO_KEY_G,
   /* 0x48 */    ALLEGRO_KEY_H,           ALLEGRO_KEY_I,             ALLEGRO_KEY_J,              ALLEGRO_KEY_K,
   /* 0x4C */    ALLEGRO_KEY_L,           ALLEGRO_KEY_M,             ALLEGRO_KEY_N,              ALLEGRO_KEY_O,
   /* 0x50 */    ALLEGRO_KEY_P,           ALLEGRO_KEY_Q,             ALLEGRO_KEY_R,              ALLEGRO_KEY_S,
   /* 0x54 */    ALLEGRO_KEY_T,           ALLEGRO_KEY_U,             ALLEGRO_KEY_V,              ALLEGRO_KEY_W,
   /* 0x58 */    ALLEGRO_KEY_X,           ALLEGRO_KEY_Y,             ALLEGRO_KEY_Z,              ALLEGRO_KEY_LWIN,
   /* 0x5C */    ALLEGRO_KEY_RWIN,        ALLEGRO_KEY_MENU,          0,                          0,
   /* 0x60 */    ALLEGRO_KEY_PAD_0,       ALLEGRO_KEY_PAD_1,         ALLEGRO_KEY_PAD_2,          ALLEGRO_KEY_PAD_3,
   /* 0x64 */    ALLEGRO_KEY_PAD_4,       ALLEGRO_KEY_PAD_5,         ALLEGRO_KEY_PAD_6,          ALLEGRO_KEY_PAD_7,
   /* 0x68 */    ALLEGRO_KEY_PAD_8,       ALLEGRO_KEY_PAD_9,         ALLEGRO_KEY_PAD_ASTERISK,   ALLEGRO_KEY_PAD_PLUS,
   /* 0x6C */    ALLEGRO_KEY_UNKNOWN+15,  ALLEGRO_KEY_PAD_MINUS,     ALLEGRO_KEY_PAD_DELETE,     ALLEGRO_KEY_PAD_SLASH,
   /* 0x70 */    ALLEGRO_KEY_F1,          ALLEGRO_KEY_F2,            ALLEGRO_KEY_F3,             ALLEGRO_KEY_F4,
   /* 0x74 */    ALLEGRO_KEY_F5,          ALLEGRO_KEY_F6,            ALLEGRO_KEY_F7,             ALLEGRO_KEY_F8,
   /* 0x78 */    ALLEGRO_KEY_F9,          ALLEGRO_KEY_F10,           ALLEGRO_KEY_F11,            ALLEGRO_KEY_F12,
   /* 0x7C */    ALLEGRO_KEY_UNKNOWN+17,  ALLEGRO_KEY_UNKNOWN+18,    ALLEGRO_KEY_UNKNOWN+19,     ALLEGRO_KEY_UNKNOWN+20,
   /* 0x80 */    ALLEGRO_KEY_UNKNOWN+21,  ALLEGRO_KEY_UNKNOWN+22,    ALLEGRO_KEY_UNKNOWN+23,     ALLEGRO_KEY_UNKNOWN+24,
   /* 0x84 */    ALLEGRO_KEY_UNKNOWN+25,  ALLEGRO_KEY_UNKNOWN+26,    ALLEGRO_KEY_UNKNOWN+27,     ALLEGRO_KEY_UNKNOWN+28,
   /* 0x88 */    0,                       0,                         0,                          0,
   /* 0x8C */    0,                       0,                         0,                          0,
   /* 0x90 */    ALLEGRO_KEY_NUMLOCK,     ALLEGRO_KEY_SCROLLLOCK,    0,                          0,
   /* 0x94 */    0,                       0,                         0,                          0,
   /* 0x98 */    0,                       0,                         0,                          0,
   /* 0x9C */    0,                       0,                         0,                          0,
   /* 0xA0 */    ALLEGRO_KEY_LSHIFT,      ALLEGRO_KEY_RSHIFT,        ALLEGRO_KEY_LCTRL,          ALLEGRO_KEY_RCTRL,
   /* 0xA4 */    ALLEGRO_KEY_ALT,         ALLEGRO_KEY_ALTGR,         0,                          0,
   /* 0xA8 */    0,                       0,                         0,                          0,
   /* 0xAC */    0,                       0,                         0,                          0,
   /* 0xB0 */    0,                       0,                         0,                          0,
   /* 0xB4 */    0,                       0,                         0,                          0,
   /* 0xB8 */    0,                       0,                         ALLEGRO_KEY_SEMICOLON,      ALLEGRO_KEY_EQUALS,
   /* 0xBC */    ALLEGRO_KEY_COMMA,       ALLEGRO_KEY_MINUS,         ALLEGRO_KEY_FULLSTOP,       ALLEGRO_KEY_SLASH,
   /* 0xC0 */    ALLEGRO_KEY_TILDE,       0,                         0,                          0,
   /* 0xC4 */    0,                       0,                         0,                          0,
   /* 0xC8 */    0,                       0,                         0,                          0,
   /* 0xCC */    0,                       0,                         0,                          0,
   /* 0xD0 */    0,                       0,                         0,                          0,
   /* 0xD4 */    0,                       0,                         0,                          0,
   /* 0xD8 */    0,                       0,                         0,                          ALLEGRO_KEY_OPENBRACE,
   /* 0xDC */    ALLEGRO_KEY_BACKSLASH,   ALLEGRO_KEY_CLOSEBRACE,    ALLEGRO_KEY_QUOTE,          0,
   /* 0xE0 */    0,                       0,                         ALLEGRO_KEY_BACKSLASH2,     0,
   /* 0xE4 */    0,                       ALLEGRO_KEY_UNKNOWN+29,    0,                          0,
   /* 0xE8 */    0,                       0,                         0,                          0,
   /* 0xEC */    0,                       0,                         0,                          0,
   /* 0xF0 */    0,                       0,                         0,                          0,
   /* 0xF4 */    0,                       0,                         ALLEGRO_KEY_UNKNOWN+30,     ALLEGRO_KEY_UNKNOWN+31,
   /* 0xF8 */    ALLEGRO_KEY_UNKNOWN+32,  ALLEGRO_KEY_UNKNOWN+33,    ALLEGRO_KEY_UNKNOWN+34,     ALLEGRO_KEY_UNKNOWN+35,
   /* 0xFC */    ALLEGRO_KEY_UNKNOWN+36,  ALLEGRO_KEY_UNKNOWN+37,    ALLEGRO_KEY_UNKNOWN+38,     0
   };


/* forward declarations */
static bool init_keyboard(void);
static void exit_keyboard(void);
static void get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state);
static void clear_keyboard_state(void);
static ALLEGRO_KEYBOARD *get_keyboard(void);


/* the driver vtable */
#define KEYBOARD_WINAPI        AL_ID('W','A','P','I')

static ALLEGRO_KEYBOARD_DRIVER keyboard_winapi =
{
   KEYBOARD_WINAPI,
   0,
   0,
   "WinAPI keyboard",
   init_keyboard,
   exit_keyboard,
   get_keyboard,
   NULL, /* bool set_leds(int leds) */
   NULL, /* const char* keycode_to_name(int keycode)*/
   get_keyboard_state,
   clear_keyboard_state,
};



/* list the available drivers */
_AL_DRIVER_INFO _al_keyboard_driver_list[] =
{
   {  KEYBOARD_WINAPI, &keyboard_winapi, true  },
   {  0,                NULL,              0     }
};


/* init_keyboard:
 *  Initialise the keyboard driver.
 */
static bool init_keyboard(void)
{
   memset(&the_keyboard, 0, sizeof the_keyboard);
   memset(&the_state, 0, sizeof the_state);
   modifiers = 0;

   /* Initialise the keyboard object for use as an event source. */
   _al_event_source_init(&the_keyboard.es);

   installed = true;
   return true;
}



/* exit_keyboard:
 *  Shut down the keyboard driver.
 */
static void exit_keyboard(void)
{
   _al_event_source_free(&the_keyboard.es);

   /* This may help catch bugs in the user program, since the pointer
    * we return to the user is always the same.
    */
   memset(&the_keyboard, 0, sizeof the_keyboard);

   installed = false;
}



/* _al_win_fix_modifiers:
 *  Fix the modifiers.
 */
void _al_win_fix_modifiers(void)
{
   modifiers = 0;
}



/* get_keyboard:
 *  Returns the address of a ALLEGRO_KEYBOARD structure representing the keyboard.
 */
static ALLEGRO_KEYBOARD *get_keyboard(void)
{
   return &the_keyboard;
}



/* get_keyboard_state:
 *  Copy the current keyboard state into RET_STATE.
 */
static void get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
   unsigned int i;
   ALLEGRO_DISPLAY *disp = NULL;
   ALLEGRO_SYSTEM *sys;

   sys = al_get_system_driver();
   for (i = 0; i < sys->displays._size; i++) {
      ALLEGRO_DISPLAY_WIN **d = (void*)_al_vector_ref(&sys->displays, i);
      if ((*d)->window == GetForegroundWindow()) {
         disp = (void*)*d;
         break;
      }
   }
   the_state.display = disp;
   *ret_state = the_state;
}



/* clear_keyboard_state:
 *  Clear the current keyboard state.
 */
static void clear_keyboard_state(void)
{
   memset(&the_state, 0, sizeof(the_state));
}



/* extkey_to_keycode:
 *  Given a VK code, returns the Allegro keycode for the corresponding extended
 *  key.  If no code is found, returns zero.
 */
static int extkey_to_keycode(int vcode)
{
   switch (vcode) {
      /* These are ordered by VK value, lowest first. */
      case VK_CANCEL:   return ALLEGRO_KEY_PAUSE;
      case VK_RETURN:   return ALLEGRO_KEY_PAD_ENTER;
      case VK_CONTROL:  return ALLEGRO_KEY_RCTRL;
      case VK_MENU:     return ALLEGRO_KEY_ALTGR;
      case VK_PRIOR:    return ALLEGRO_KEY_PGUP;
      case VK_NEXT:     return ALLEGRO_KEY_PGDN;
      case VK_END:      return ALLEGRO_KEY_END;
      case VK_HOME:     return ALLEGRO_KEY_HOME;
      case VK_LEFT:     return ALLEGRO_KEY_LEFT;
      case VK_UP:       return ALLEGRO_KEY_UP;
      case VK_RIGHT:    return ALLEGRO_KEY_RIGHT;
      case VK_DOWN:     return ALLEGRO_KEY_DOWN;
      case VK_SNAPSHOT: return ALLEGRO_KEY_PRINTSCREEN;
      case VK_INSERT:   return ALLEGRO_KEY_INSERT;
      case VK_DELETE:   return ALLEGRO_KEY_DELETE;
      case VK_LWIN:     return ALLEGRO_KEY_LWIN;
      case VK_RWIN:     return ALLEGRO_KEY_RWIN;
      case VK_APPS:     return ALLEGRO_KEY_MENU;
      case VK_DIVIDE:   return ALLEGRO_KEY_PAD_SLASH;
      case VK_NUMLOCK:  return ALLEGRO_KEY_NUMLOCK;
      default: return 0;
   }
}

static void update_modifiers(int code, bool pressed)
{
#define ON_OFF2(code)         \
{                             \
   if (!pressed)              \
      modifiers &= ~code;     \
   else                       \
      modifiers |= code;      \
   break;                     \
}

   switch (code) {
      case ALLEGRO_KEY_LSHIFT:
         ON_OFF2(ALLEGRO_KEYMOD_SHIFT);
      case ALLEGRO_KEY_RSHIFT:
         ON_OFF2(ALLEGRO_KEYMOD_SHIFT);
      case ALLEGRO_KEY_RCTRL:
         ON_OFF2(ALLEGRO_KEYMOD_CTRL);
      case ALLEGRO_KEY_LCTRL:
         ON_OFF2(ALLEGRO_KEYMOD_CTRL);
      case ALLEGRO_KEY_ALT:
         ON_OFF2(ALLEGRO_KEYMOD_ALT);
      case ALLEGRO_KEY_ALTGR:
         ON_OFF2(ALLEGRO_KEYMOD_ALTGR);
      case ALLEGRO_KEY_LWIN:
          ON_OFF2(ALLEGRO_KEYMOD_LWIN);
       case ALLEGRO_KEY_RWIN:
          ON_OFF2(ALLEGRO_KEYMOD_RWIN);
       case ALLEGRO_KEY_MENU:
          ON_OFF2(ALLEGRO_KEYMOD_MENU);
   }

#undef ON_OFF2
}



/* update_toggle_modifiers:
 *  Update the state of Num Lock, Caps Lock, and Scroll Lock.
 */
static void update_toggle_modifiers(void)
{
#define ON_OFF(code, on)      \
{                             \
   if (on)                    \
         modifiers |= code;   \
   else                       \
         modifiers &= ~code;  \
}
   /* GetKeyState appears to be the only reliable way of doing this.  GetKeyboardState
    * is updated a bit too late in some cases. Maybe it would work if WM_CHARs were
    * used, since they arrive after the WM_KEYDOWNs that trigger them.
    */
   ON_OFF(ALLEGRO_KEYMOD_NUMLOCK, GetKeyState(VK_NUMLOCK) & 1);
   ON_OFF(ALLEGRO_KEYMOD_CAPSLOCK, GetKeyState(VK_CAPITAL) & 1);
   ON_OFF(ALLEGRO_KEYMOD_SCROLLLOCK, GetKeyState(VK_SCROLL) & 1);

#undef ON_OFF
}



/* _al_win_kbd_handle_key_press:
 *  Does stuff when a key is pressed.
 */
void _al_win_kbd_handle_key_press(int scode, int vcode, bool extended,
                           bool repeated, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)win_disp;
   ALLEGRO_EVENT event;
   int my_code;
   bool actual_repeat;
   int char_count;
   int event_count;
   int i;
   BYTE ks[256];
   WCHAR buf[8] = { 0 };

   if (!installed)
      return;

   /* Check for an extended key first. */
   my_code = 0;
   if (extended)
      my_code = extkey_to_keycode(vcode);

   /* Map a non-extended key.  This also works as a fallback in case
      the key was extended, but no extended mapping was found. */
   if (my_code == 0) {
      if (vcode == VK_SHIFT) /* Left or right Shift key? */
         vcode = MapVirtualKey(scode, MAPVK_VSC_TO_VK_EX);
      my_code = hw_to_mycode[vcode];
   }
   update_modifiers(my_code, true);

   actual_repeat = repeated && _AL_KEYBOARD_STATE_KEY_DOWN(the_state, my_code);
   _AL_KEYBOARD_STATE_SET_KEY_DOWN(the_state, my_code);

   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event.keyboard.type = ALLEGRO_EVENT_KEY_DOWN;
   event.keyboard.timestamp = al_get_time();
   event.keyboard.display = display;
   event.keyboard.keycode = my_code;
   event.keyboard.unichar = 0;
   update_toggle_modifiers();
   event.keyboard.modifiers = modifiers;
   event.keyboard.repeat = false;

   _al_event_source_lock(&the_keyboard.es);

   if (my_code > 0 && !actual_repeat) {
      _al_event_source_emit_event(&the_keyboard.es, &event);
   }

   /* Send char events, but not for modifier keys or dead keys. */
   if (my_code < ALLEGRO_KEY_MODIFIERS) {
      char_count = ToUnicode(vcode, scode, GetKeyboardState(ks) ? ks : NULL, buf, 8, 0);
      /* Send ASCII code 127 for both Del keys. */
      if (char_count == 0 && vcode == VK_DELETE) {
         char_count = 1;
         buf[0] = 127;
      }
      if (char_count != -1) { /* -1 means it was a dead key. */
         event_count = char_count ? char_count : 1;
         event.keyboard.type = ALLEGRO_EVENT_KEY_CHAR;
         update_toggle_modifiers();
         event.keyboard.modifiers = modifiers;
         event.keyboard.repeat = actual_repeat;
         for (i = 0; i < event_count; i++) {
            event.keyboard.unichar = buf[i];
            _al_event_source_emit_event(&the_keyboard.es, &event);
         }
      }
   }
   _al_event_source_unlock(&the_keyboard.es);

   /* Toggle mouse grab key. */
   if (my_code && !repeated) {
      ALLEGRO_SYSTEM_WIN *system = (void *)al_get_system_driver();
      if (system->toggle_mouse_grab_keycode && my_code == system->toggle_mouse_grab_keycode &&
         (modifiers & system->toggle_mouse_grab_modifiers) == system->toggle_mouse_grab_modifiers)
      {
         if (system->mouse_grab_display == display) {
            al_ungrab_mouse();
         }
         else {
            al_grab_mouse(display);
         }
      }
   }
}



/* _al_win_kbd_handle_key_release:
 *  Does stuff when a key is released.
 */
void _al_win_kbd_handle_key_release(int scode, int vcode, bool extended, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_EVENT event;
   int my_code;

   if (!installed)
     return;

   my_code = 0;
   if (extended)
      my_code = extkey_to_keycode(vcode);

   if (my_code == 0) {
      if (vcode == VK_SHIFT)
         vcode = MapVirtualKey(scode, MAPVK_VSC_TO_VK_EX);
      my_code = hw_to_mycode[vcode];
   }
   update_modifiers(my_code, false);

   _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(the_state, my_code);

   /* Windows only sends a WM_KEYUP message for the Shift keys when
      both have been released. If one of the Shift keys is still reported
      as down, we need to release it as well. */
   if (my_code == ALLEGRO_KEY_LSHIFT && _AL_KEYBOARD_STATE_KEY_DOWN(the_state, ALLEGRO_KEY_RSHIFT))
      _al_win_kbd_handle_key_release(scode, VK_RSHIFT, extended, win_disp);
   else if (my_code == ALLEGRO_KEY_RSHIFT && _AL_KEYBOARD_STATE_KEY_DOWN(the_state, ALLEGRO_KEY_LSHIFT))
      _al_win_kbd_handle_key_release(scode, VK_LSHIFT, extended, win_disp);

   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
   event.keyboard.timestamp = al_get_time();
   event.keyboard.display = (void*)win_disp;
   event.keyboard.keycode = my_code;
   event.keyboard.unichar = 0;
   update_toggle_modifiers();
   event.keyboard.modifiers = modifiers;

   _al_event_source_lock(&the_keyboard.es);
   _al_event_source_emit_event(&the_keyboard.es, &event);
   _al_event_source_unlock(&the_keyboard.es);
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
