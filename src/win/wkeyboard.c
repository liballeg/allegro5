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

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintwin.h"


static bool installed = false;
static ALLEGRO_KEYBOARD the_keyboard;
static ALLEGRO_KEYBOARD_STATE the_state;
static int modifiers = 0;

/* lookup table for converting (OEM dependent!) scancodes into Allegro ALLEGRO_KEY_* codes */
static const unsigned char hw_to_mycode[256] =
{
   /* 0x00 */  0,                      ALLEGRO_KEY_ESCAPE,     ALLEGRO_KEY_1,          ALLEGRO_KEY_2, 
   /* 0x04 */  ALLEGRO_KEY_3,          ALLEGRO_KEY_4,          ALLEGRO_KEY_5,          ALLEGRO_KEY_6,
   /* 0x08 */  ALLEGRO_KEY_7,          ALLEGRO_KEY_8,          ALLEGRO_KEY_9,          ALLEGRO_KEY_0, 
   /* 0x0C */  ALLEGRO_KEY_MINUS,      ALLEGRO_KEY_EQUALS,     ALLEGRO_KEY_BACKSPACE,  ALLEGRO_KEY_TAB,
   /* 0x10 */  ALLEGRO_KEY_Q,          ALLEGRO_KEY_W,          ALLEGRO_KEY_E,          ALLEGRO_KEY_R, 
   /* 0x14 */  ALLEGRO_KEY_T,          ALLEGRO_KEY_Y,          ALLEGRO_KEY_U,          ALLEGRO_KEY_I,
   /* 0x18 */  ALLEGRO_KEY_O,          ALLEGRO_KEY_P,          ALLEGRO_KEY_OPENBRACE,  ALLEGRO_KEY_CLOSEBRACE, 
   /* 0x1C */  ALLEGRO_KEY_ENTER,      ALLEGRO_KEY_LCTRL,      ALLEGRO_KEY_A,          ALLEGRO_KEY_S,
   /* 0x20 */  ALLEGRO_KEY_D,          ALLEGRO_KEY_F,          ALLEGRO_KEY_G,          ALLEGRO_KEY_H, 
   /* 0x24 */  ALLEGRO_KEY_J,          ALLEGRO_KEY_K,          ALLEGRO_KEY_L,          ALLEGRO_KEY_SEMICOLON,
   /* 0x28 */  ALLEGRO_KEY_QUOTE,      ALLEGRO_KEY_TILDE,      ALLEGRO_KEY_LSHIFT,     ALLEGRO_KEY_BACKSLASH, 
   /* 0x2C */  ALLEGRO_KEY_Z,          ALLEGRO_KEY_X,          ALLEGRO_KEY_C,          ALLEGRO_KEY_V,
   /* 0x30 */  ALLEGRO_KEY_B,          ALLEGRO_KEY_N,          ALLEGRO_KEY_M,          ALLEGRO_KEY_COMMA, 
   /* 0x34 */  ALLEGRO_KEY_FULLSTOP,   ALLEGRO_KEY_SLASH,      ALLEGRO_KEY_RSHIFT,     ALLEGRO_KEY_PAD_ASTERISK,
   /* 0x38 */  ALLEGRO_KEY_ALT,        ALLEGRO_KEY_SPACE,      ALLEGRO_KEY_CAPSLOCK,   ALLEGRO_KEY_F1, 
   /* 0x3C */  ALLEGRO_KEY_F2,         ALLEGRO_KEY_F3,         ALLEGRO_KEY_F4,         ALLEGRO_KEY_F5,
   /* 0x40 */  ALLEGRO_KEY_F6,         ALLEGRO_KEY_F7,         ALLEGRO_KEY_F8,         ALLEGRO_KEY_F9, 
   /* 0x44 */  ALLEGRO_KEY_F10,        ALLEGRO_KEY_NUMLOCK,    ALLEGRO_KEY_SCROLLLOCK, ALLEGRO_KEY_PAD_7,
   /* 0x48 */  ALLEGRO_KEY_PAD_8,      ALLEGRO_KEY_PAD_9,      ALLEGRO_KEY_PAD_MINUS,  ALLEGRO_KEY_PAD_4, 
   /* 0x4C */  ALLEGRO_KEY_PAD_5,      ALLEGRO_KEY_PAD_6,      ALLEGRO_KEY_PAD_PLUS,   ALLEGRO_KEY_PAD_1,
   /* 0x50 */  ALLEGRO_KEY_PAD_2,      ALLEGRO_KEY_PAD_3,      ALLEGRO_KEY_PAD_0,      ALLEGRO_KEY_PAD_DELETE, 
   /* 0x54 */  ALLEGRO_KEY_PRINTSCREEN,0,                      ALLEGRO_KEY_BACKSLASH2, ALLEGRO_KEY_F11,
   /* 0x58 */  ALLEGRO_KEY_F12,        0,                      0,                      ALLEGRO_KEY_LWIN, 
   /* 0x5C */  ALLEGRO_KEY_RWIN,       ALLEGRO_KEY_MENU,       0,                      0,
   /* 0x60 */  0,                      0,                      0,                      0, 
   /* 0x64 */  0,                      0,                      0,                      0,
   /* 0x68 */  0,                      0,                      0,                      0, 
   /* 0x6C */  0,                      0,                      0,                      0,
   /* 0x70 */  ALLEGRO_KEY_KANA,       0,                      0,                      ALLEGRO_KEY_ABNT_C1, 
   /* 0x74 */  0,                      0,                      0,                      0,
   /* 0x78 */  0,                      ALLEGRO_KEY_CONVERT,    0,                      ALLEGRO_KEY_NOCONVERT, 
   /* 0x7C */  0,                      ALLEGRO_KEY_YEN,        0,                      0,

   /* 0x80 */  0,                 0,                 0,                 0,
   /* 0x84 */  0,                 0,                 0,                 0,
   /* 0x88 */  0,                 0,                 0,                 0,
   /* 0x8C */  0,                 0,                 0,                 0,
   /* 0x90 */  0,                 ALLEGRO_KEY_AT,    ALLEGRO_KEY_COLON2,0,
   /* 0x94 */  ALLEGRO_KEY_KANJI, 0,                 0,                 0,
   /* 0x98 */  0,                 0,                 0,                 0,
   /* 0x9C */  ALLEGRO_KEY_PAD_ENTER,ALLEGRO_KEY_RCTRL,0,               0,
   /* 0xA0 */  0,                 0,                 0,                 0,
   /* 0xA4 */  0,                 0,                 0,                 0,
   /* 0xA8 */  0,                 0,                 0,                 0,
   /* 0xAC */  0,                 0,                 0,                 0,
   /* 0xB0 */  0,                 0,                 0,                 0,
   /* 0xB4 */  0,                 ALLEGRO_KEY_PAD_SLASH,  0,            ALLEGRO_KEY_PRINTSCREEN,
   /* 0xB8 */  ALLEGRO_KEY_ALTGR, 0,                 0,                 0,
   /* 0xBC */  0,                 0,                 0,                 0,
   /* 0xC0 */  0,                 0,                 0,                 0,
   /* 0xC4 */  0,                 ALLEGRO_KEY_PAUSE, 0,                 ALLEGRO_KEY_HOME,
   /* 0xC8 */  ALLEGRO_KEY_UP,    ALLEGRO_KEY_PGUP,  0,                 ALLEGRO_KEY_LEFT,
   /* 0xCC */  0,                 ALLEGRO_KEY_RIGHT, 0,                 ALLEGRO_KEY_END,
   /* 0xD0 */  ALLEGRO_KEY_DOWN,  ALLEGRO_KEY_PGDN,  ALLEGRO_KEY_INSERT,ALLEGRO_KEY_DELETE,
   /* 0xD4 */  0,                 0,                 0,                 0,
   /* 0xD8 */  0,                 0,                 0,                 ALLEGRO_KEY_LWIN,
   /* 0xDC */  ALLEGRO_KEY_RWIN,  ALLEGRO_KEY_MENU,  0,                 0,
   /* 0xE0 */  0,                 0,                 0,                 0,
   /* 0xE4 */  0,                 0,                 0,                 0,
   /* 0xE8 */  0,                 0,                 0,                 0,
   /* 0xEC */  0,                 0,                 0,                 0,
   /* 0xF0 */  0,                 0,                 0,                 0,
   /* 0xF4 */  0,                 0,                 0,                 0,
   /* 0xF8 */  0,                 0,                 0,                 0,
   /* 0xFC */  0,                 0,                 0,                 0
};


/* forward declarations */
static bool init_keyboard(void);
static void exit_keyboard(void);
static void get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state);
static ALLEGRO_KEYBOARD *get_keyboard(void);
static const char *keycode_to_name(int keycode);


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
   keycode_to_name,
   get_keyboard_state
};



/* list the available drivers */
_DRIVER_INFO _al_keyboard_driver_list[] =
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
   /* This may help catch bugs in the user program, since the pointer
    * we return to the user is always the same.
    */
   memset(&the_keyboard, 0, sizeof the_keyboard);

   installed = false;
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

   sys = al_system_driver();
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


static void update_modifiers(int code, bool pressed)
{
#define ON_OFF(code)          \
{                             \
   if (pressed) {             \
      if (modifiers & code)   \
         modifiers &= ~code;  \
      else                    \
         modifiers |= code;   \
   }                          \
   break;                     \
}

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

      case ALLEGRO_KEY_SCROLLLOCK:
         ON_OFF(ALLEGRO_KEYMOD_SCROLLLOCK);
      case ALLEGRO_KEY_NUMLOCK:
         ON_OFF(ALLEGRO_KEYMOD_NUMLOCK);
      case ALLEGRO_KEY_CAPSLOCK:
         ON_OFF(ALLEGRO_KEYMOD_CAPSLOCK);
   }

#undef ON_OFF
#undef ON_OFF2
}


/* wkbd_handle_key_press:
 *  Does stuff when a key is pressed.
 */
void wkbd_handle_key_press(int ccode, int scode, bool repeated,
                           ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_EVENT event;
   int my_code = hw_to_mycode[scode];

   if (!installed)
      return;

   update_modifiers(my_code, true);

   _AL_KEYBOARD_STATE_SET_KEY_DOWN(the_state, my_code);

   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event.keyboard.type = repeated ? ALLEGRO_EVENT_KEY_REPEAT : ALLEGRO_EVENT_KEY_DOWN;
   event.keyboard.timestamp = al_current_time();
   event.keyboard.display = (void*)win_disp;
   event.keyboard.keycode = my_code;
   event.keyboard.unichar = ccode;
   event.keyboard.modifiers = modifiers;

   _al_event_source_lock(&the_keyboard.es);
   _al_event_source_emit_event(&the_keyboard.es, &event);
   _al_event_source_unlock(&the_keyboard.es);
}



/* wkbd_handle_key_release:
 *  Does stuff when a key is released.
 */
void wkbd_handle_key_release(int scode, ALLEGRO_DISPLAY_WIN *win_disp)
{
   ALLEGRO_EVENT event;
   int my_code = hw_to_mycode[scode];

   if (!installed)
     return;

   update_modifiers(my_code, false);

   _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(the_state, my_code);
   
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
   event.keyboard.timestamp = al_current_time();
   event.keyboard.display = (void*)win_disp;
   event.keyboard.keycode = my_code;
   event.keyboard.unichar = 0;
   event.keyboard.modifiers = 0;

   _al_event_source_lock(&the_keyboard.es);
   _al_event_source_emit_event(&the_keyboard.es, &event);
   _al_event_source_unlock(&the_keyboard.es);
}


static const char *keycode_to_name(int keycode)
{
   static char key_name[255];
   int scode;

   for (scode = 0; scode < 256; scode++) {
      if (hw_to_mycode[scode] == keycode) {
         int param = scode << 16;
         if (GetKeyNameText(param, key_name, 255))
            return key_name;
         else
            return NULL;
      }
   }

   return NULL;
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
