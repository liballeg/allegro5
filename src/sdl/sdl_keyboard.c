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
 *      SDL keyboard driver.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/allegro_internal_sdl.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

typedef struct ALLEGRO_KEYBOARD_SDL
{
   ALLEGRO_KEYBOARD keyboard;
   int table[1024];
   int inverse[1024];
   int unicode[1024];
   int inverse_unicode[1024];
   int last_down_keycode;
   bool create_extra_char[1024];
   ALLEGRO_DISPLAY *display;
} ALLEGRO_KEYBOARD_SDL;

static ALLEGRO_KEYBOARD_DRIVER *vt;
static ALLEGRO_KEYBOARD_SDL *keyboard;


static unsigned int get_modifiers(int modifiers)
{
   int result = 0;

   if (modifiers & KMOD_LSHIFT) result |= ALLEGRO_KEYMOD_SHIFT;
   if (modifiers & KMOD_RSHIFT) result |= ALLEGRO_KEYMOD_SHIFT;
   if (modifiers & KMOD_LCTRL) result |= ALLEGRO_KEYMOD_CTRL;
   if (modifiers & KMOD_RCTRL) result |= ALLEGRO_KEYMOD_CTRL;
   if (modifiers & KMOD_LALT) result |= ALLEGRO_KEYMOD_ALT;
   if (modifiers & KMOD_RALT) result |= ALLEGRO_KEYMOD_ALT;
   if (modifiers & KMOD_LGUI) result |= ALLEGRO_KEYMOD_LWIN;
   if (modifiers & KMOD_RGUI) result |= ALLEGRO_KEYMOD_RWIN;
   if (modifiers & KMOD_NUM) result |= ALLEGRO_KEYMOD_NUMLOCK;
   if (modifiers & KMOD_CAPS) result |= ALLEGRO_KEYMOD_CAPSLOCK;
   if (modifiers & KMOD_MODE) result |= ALLEGRO_KEYMOD_ALTGR;

   return result;
}

void _al_sdl_keyboard_event(SDL_Event *e)
{
   if (!keyboard)
      return;
   if (e->type == SDL_WINDOWEVENT) {
      if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
         keyboard->display = _al_sdl_find_display(e->window.windowID);
      }
      else {
         keyboard->display = NULL;
      }
      return;
   }
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = &keyboard->keyboard.es;
   _al_event_source_lock(es);
   event.keyboard.timestamp = al_get_time();
   event.keyboard.display = NULL;
   event.keyboard.repeat = false;

   if (e->type == SDL_TEXTINPUT) {
      ALLEGRO_USTR_INFO info;
      ALLEGRO_USTR const *u = al_ref_cstr(&info, e->text.text);
      int pos = 0;
      while (true) {
         int32_t c = al_ustr_get_next(u, &pos);
         if (c <= 0)
            break;
         event.keyboard.type = ALLEGRO_EVENT_KEY_CHAR;
         event.keyboard.modifiers = get_modifiers(SDL_GetModState());
         if (keyboard->last_down_keycode != 0) {
            event.keyboard.keycode = keyboard->last_down_keycode;
         } else {
            event.keyboard.keycode = c < 1024 ? keyboard->inverse_unicode[c] : 0;
         }
         event.keyboard.unichar = c;
         event.keyboard.display = _al_sdl_find_display(e->text.windowID);
         _al_event_source_emit_event(es, &event);
      }
   }
   else if (e->type == SDL_KEYDOWN) {
      event.keyboard.type = ALLEGRO_EVENT_KEY_DOWN;
      event.keyboard.modifiers = get_modifiers(e->key.keysym.mod);
      event.keyboard.keycode = keyboard->last_down_keycode = keyboard->table[e->key.keysym.scancode];
      event.keyboard.display = _al_sdl_find_display(e->key.windowID);
      if (!e->key.repeat) {
         _al_event_source_emit_event(es, &event);
      }

      if (keyboard->create_extra_char[e->key.keysym.scancode]) {
         event.keyboard.type = ALLEGRO_EVENT_KEY_CHAR;
         _al_event_source_emit_event(es, &event);
      }
   }
   else if (e->type == SDL_KEYUP) {
      event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
      event.keyboard.modifiers = get_modifiers(e->key.keysym.mod);
      event.keyboard.keycode = keyboard->table[e->key.keysym.scancode];
      event.keyboard.display = _al_sdl_find_display(e->key.windowID);
      _al_event_source_emit_event(es, &event);
   }

   keyboard->display = event.keyboard.display;

   _al_event_source_unlock(es);
}

static void adde(int sdl_scancode, int allegro_keycode, int unicode, bool extra)
{
   if (sdl_scancode >= 1024) {
      ALLEGRO_WARN("Cannot map SDL scancode %d.\n", sdl_scancode);
      return;
   }
   keyboard->table[sdl_scancode] = allegro_keycode;
   keyboard->inverse[allegro_keycode] = sdl_scancode;
   keyboard->unicode[sdl_scancode] = unicode;
   keyboard->inverse_unicode[unicode] = allegro_keycode;
   keyboard->create_extra_char[sdl_scancode] = extra;
}

static void add(int sdl_scancode, int allegro_keycode, int unicode)
{
   adde(sdl_scancode, allegro_keycode, unicode, false);
}

#define ADD_SAME(X, c) add(SDL_SCANCODE_##X, ALLEGRO_KEY_##X, c)
#define ADD_SAMEC(X) add(SDL_SCANCODE_##X, ALLEGRO_KEY_##X, tolower(#X[0]))
#define ADD_SAMEE(X, c) adde(SDL_SCANCODE_##X, ALLEGRO_KEY_##X, c, true)

static bool sdl_init_keyboard(void)
{
   keyboard = al_calloc(1, sizeof *keyboard);
   _al_event_source_init(&keyboard->keyboard.es);

   add(SDL_SCANCODE_UNKNOWN, 0, 0);
   ADD_SAMEC(A);
   ADD_SAMEC(B);
   ADD_SAMEC(C);
   ADD_SAMEC(D);
   ADD_SAMEC(E);
   ADD_SAMEC(F);
   ADD_SAMEC(G);
   ADD_SAMEC(H);
   ADD_SAMEC(I);
   ADD_SAMEC(J);
   ADD_SAMEC(K);
   ADD_SAMEC(L);
   ADD_SAMEC(M);
   ADD_SAMEC(N);
   ADD_SAMEC(O);
   ADD_SAMEC(P);
   ADD_SAMEC(Q);
   ADD_SAMEC(R);
   ADD_SAMEC(S);
   ADD_SAMEC(T);
   ADD_SAMEC(U);
   ADD_SAMEC(V);
   ADD_SAMEC(W);
   ADD_SAMEC(X);
   ADD_SAMEC(Y);
   ADD_SAMEC(Z);
   ADD_SAMEC(1);
   ADD_SAMEC(2);
   ADD_SAMEC(3);
   ADD_SAMEC(4);
   ADD_SAMEC(5);
   ADD_SAMEC(6);
   ADD_SAMEC(7);
   ADD_SAMEC(8);
   ADD_SAMEC(9);
   ADD_SAMEC(0);
   adde(SDL_SCANCODE_RETURN, ALLEGRO_KEY_ENTER, 13, true);
   ADD_SAMEE(ESCAPE, 27);
   ADD_SAMEE(BACKSPACE, 8);
   ADD_SAMEE(TAB, 9);
   ADD_SAME(SPACE, ' ');
   ADD_SAME(MINUS, '-');
   ADD_SAME(EQUALS, '=');
   add(SDL_SCANCODE_LEFTBRACKET, ALLEGRO_KEY_OPENBRACE, '[');
   add(SDL_SCANCODE_RIGHTBRACKET, ALLEGRO_KEY_CLOSEBRACE, ']');
   ADD_SAME(BACKSLASH, '\\');
   add(SDL_SCANCODE_NONUSHASH, 0, '#');
   ADD_SAME(SEMICOLON, ';');
   add(SDL_SCANCODE_APOSTROPHE, ALLEGRO_KEY_QUOTE, '\'');
   add(SDL_SCANCODE_GRAVE, ALLEGRO_KEY_TILDE, '~');
   ADD_SAME(COMMA, ',');
   add(SDL_SCANCODE_PERIOD, ALLEGRO_KEY_FULLSTOP, '.');
   ADD_SAME(SLASH, '/');
   ADD_SAME(CAPSLOCK, 0);
   ADD_SAMEE(F1, 0);
   ADD_SAMEE(F2, 0);
   ADD_SAMEE(F3, 0);
   ADD_SAMEE(F4, 0);
   ADD_SAMEE(F5, 0);
   ADD_SAMEE(F6, 0);
   ADD_SAMEE(F7, 0);
   ADD_SAMEE(F8, 0);
   ADD_SAMEE(F9, 0);
   ADD_SAMEE(F10, 0);
   ADD_SAMEE(F11, 0);
   ADD_SAMEE(F12, 0);
   ADD_SAME(PRINTSCREEN, 0);
   ADD_SAME(SCROLLLOCK, 0);
   ADD_SAME(PAUSE, 0);
   ADD_SAMEE(INSERT, 0);
   ADD_SAMEE(HOME, 0);
   add(SDL_SCANCODE_PAGEUP, ALLEGRO_KEY_PGUP, 0);
   ADD_SAMEE(DELETE, 0);
   ADD_SAMEE(END, 0);
   add(SDL_SCANCODE_PAGEDOWN, ALLEGRO_KEY_PGDN, 0);
   ADD_SAMEE(RIGHT, 0);
   ADD_SAMEE(LEFT, 0);
   ADD_SAMEE(DOWN, 0);
   ADD_SAMEE(UP, 0);
   add(SDL_SCANCODE_NUMLOCKCLEAR, ALLEGRO_KEY_NUMLOCK, 0);
   add(SDL_SCANCODE_KP_DIVIDE, ALLEGRO_KEY_PAD_SLASH, '/');
   add(SDL_SCANCODE_KP_MULTIPLY, ALLEGRO_KEY_PAD_ASTERISK, '*');
   add(SDL_SCANCODE_KP_MINUS, ALLEGRO_KEY_PAD_MINUS, '-');
   add(SDL_SCANCODE_KP_PLUS, ALLEGRO_KEY_PAD_PLUS, '+');
   add(SDL_SCANCODE_KP_ENTER, ALLEGRO_KEY_PAD_ENTER, 13);
   add(SDL_SCANCODE_KP_1, ALLEGRO_KEY_PAD_1, '1');
   add(SDL_SCANCODE_KP_2, ALLEGRO_KEY_PAD_2, '2');
   add(SDL_SCANCODE_KP_3, ALLEGRO_KEY_PAD_3, '3');
   add(SDL_SCANCODE_KP_4, ALLEGRO_KEY_PAD_4, '4');
   add(SDL_SCANCODE_KP_5, ALLEGRO_KEY_PAD_5, '5');
   add(SDL_SCANCODE_KP_6, ALLEGRO_KEY_PAD_6, '6');
   add(SDL_SCANCODE_KP_7, ALLEGRO_KEY_PAD_7, '7');
   add(SDL_SCANCODE_KP_8, ALLEGRO_KEY_PAD_8, '8');
   add(SDL_SCANCODE_KP_9, ALLEGRO_KEY_PAD_9, '9');
   add(SDL_SCANCODE_KP_0, ALLEGRO_KEY_PAD_0, '0');
   add(SDL_SCANCODE_KP_PERIOD, ALLEGRO_KEY_PAD_DELETE, 0);
   add(SDL_SCANCODE_NONUSBACKSLASH, 0, '\\');
   add(SDL_SCANCODE_APPLICATION, 0, 0);
   add(SDL_SCANCODE_POWER, 0, 0);
   add(SDL_SCANCODE_KP_EQUALS, ALLEGRO_KEY_PAD_EQUALS, '=');
   add(SDL_SCANCODE_F13, 0, 0);
   add(SDL_SCANCODE_F14, 0, 0);
   add(SDL_SCANCODE_F15, 0, 0);
   add(SDL_SCANCODE_F16, 0, 0);
   add(SDL_SCANCODE_F17, 0, 0);
   add(SDL_SCANCODE_F18, 0, 0);
   add(SDL_SCANCODE_F19, 0, 0);
   add(SDL_SCANCODE_F20, 0, 0);
   add(SDL_SCANCODE_F21, 0, 0);
   add(SDL_SCANCODE_F22, 0, 0);
   add(SDL_SCANCODE_F23, 0, 0);
   add(SDL_SCANCODE_F24, 0, 0);
   add(SDL_SCANCODE_EXECUTE, 0, 0);
   add(SDL_SCANCODE_HELP, 0, 0);
   ADD_SAME(MENU, 0);
   add(SDL_SCANCODE_SELECT, 0, 0);
   add(SDL_SCANCODE_STOP, 0, 0);
   add(SDL_SCANCODE_AGAIN, 0, 0);
   add(SDL_SCANCODE_UNDO, 0, 0);
   add(SDL_SCANCODE_CUT, 0, 0);
   add(SDL_SCANCODE_COPY, 0, 0);
   add(SDL_SCANCODE_PASTE, 0, 0);
   add(SDL_SCANCODE_FIND, 0, 0);
   add(SDL_SCANCODE_MUTE, 0, 0);
   add(SDL_SCANCODE_VOLUMEUP, 0, 0);
   add(SDL_SCANCODE_VOLUMEDOWN, 0, 0);
   add(SDL_SCANCODE_KP_COMMA, 0, ',');
   add(SDL_SCANCODE_KP_EQUALSAS400, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL1, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL2, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL3, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL4, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL5, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL6, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL7, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL8, 0, 0);
   add(SDL_SCANCODE_INTERNATIONAL9, 0, 0);
   add(SDL_SCANCODE_LANG1, 0, 0);
   add(SDL_SCANCODE_LANG2, 0, 0);
   add(SDL_SCANCODE_LANG3, 0, 0);
   add(SDL_SCANCODE_LANG4, 0, 0);
   add(SDL_SCANCODE_LANG5, 0, 0);
   add(SDL_SCANCODE_LANG6, 0, 0);
   add(SDL_SCANCODE_LANG7, 0, 0);
   add(SDL_SCANCODE_LANG8, 0, 0);
   add(SDL_SCANCODE_LANG9, 0, 0);
   add(SDL_SCANCODE_ALTERASE, 0, 0);
   add(SDL_SCANCODE_SYSREQ, 0, 0);
   add(SDL_SCANCODE_CANCEL, 0, 0);
   add(SDL_SCANCODE_CLEAR, 0, 0);
   add(SDL_SCANCODE_PRIOR, 0, 0);
   add(SDL_SCANCODE_RETURN2, 0, 0);
   add(SDL_SCANCODE_SEPARATOR, 0, 0);
   add(SDL_SCANCODE_OUT, 0, 0);
   add(SDL_SCANCODE_OPER, 0, 0);
   add(SDL_SCANCODE_CLEARAGAIN, 0, 0);
   add(SDL_SCANCODE_CRSEL, 0, 0);
   add(SDL_SCANCODE_EXSEL, 0, 0);
   add(SDL_SCANCODE_KP_00, 0, 0);
   add(SDL_SCANCODE_KP_000, 0, 0);
   add(SDL_SCANCODE_THOUSANDSSEPARATOR, 0, 0);
   add(SDL_SCANCODE_DECIMALSEPARATOR, 0, 0);
   add(SDL_SCANCODE_CURRENCYUNIT, 0, 0);
   add(SDL_SCANCODE_CURRENCYSUBUNIT, 0, 0);
   add(SDL_SCANCODE_KP_LEFTPAREN, 0, 0);
   add(SDL_SCANCODE_KP_RIGHTPAREN, 0, 0);
   add(SDL_SCANCODE_KP_LEFTBRACE, 0, '{');
   add(SDL_SCANCODE_KP_RIGHTBRACE, 0, '}');
   add(SDL_SCANCODE_KP_TAB, 0, 9);
   add(SDL_SCANCODE_KP_BACKSPACE, 0, 8);
   add(SDL_SCANCODE_KP_A, 0, 0);
   add(SDL_SCANCODE_KP_B, 0, 0);
   add(SDL_SCANCODE_KP_C, 0, 0);
   add(SDL_SCANCODE_KP_D, 0, 0);
   add(SDL_SCANCODE_KP_E, 0, 0);
   add(SDL_SCANCODE_KP_F, 0, 0);
   add(SDL_SCANCODE_KP_XOR, 0, 0);
   add(SDL_SCANCODE_KP_POWER, 0, 0);
   add(SDL_SCANCODE_KP_PERCENT, 0, 0);
   add(SDL_SCANCODE_KP_LESS, 0, '<');
   add(SDL_SCANCODE_KP_GREATER, 0, '>');
   add(SDL_SCANCODE_KP_AMPERSAND, 0, '&');
   add(SDL_SCANCODE_KP_DBLAMPERSAND, 0, 0);
   add(SDL_SCANCODE_KP_VERTICALBAR, 0, '|');
   add(SDL_SCANCODE_KP_DBLVERTICALBAR, 0, 0);
   add(SDL_SCANCODE_KP_COLON, 0, ':');
   add(SDL_SCANCODE_KP_HASH, 0, '#');
   add(SDL_SCANCODE_KP_SPACE, 0, 0);
   add(SDL_SCANCODE_KP_AT, 0, '@');
   add(SDL_SCANCODE_KP_EXCLAM, 0, '!');
   add(SDL_SCANCODE_KP_MEMSTORE, 0, 0);
   add(SDL_SCANCODE_KP_MEMRECALL, 0, 0);
   add(SDL_SCANCODE_KP_MEMCLEAR, 0, 0);
   add(SDL_SCANCODE_KP_MEMADD, 0, 0);
   add(SDL_SCANCODE_KP_MEMSUBTRACT, 0, 0);
   add(SDL_SCANCODE_KP_MEMMULTIPLY, 0, 0);
   add(SDL_SCANCODE_KP_MEMDIVIDE, 0, 0);
   add(SDL_SCANCODE_KP_PLUSMINUS, 0, 0);
   add(SDL_SCANCODE_KP_CLEAR, 0, 0);
   add(SDL_SCANCODE_KP_CLEARENTRY, 0, 0);
   add(SDL_SCANCODE_KP_BINARY, 0, 0);
   add(SDL_SCANCODE_KP_OCTAL, 0, 0);
   add(SDL_SCANCODE_KP_DECIMAL, 0, 0);
   add(SDL_SCANCODE_KP_HEXADECIMAL, 0, 0);
   ADD_SAME(LCTRL, 0);
   ADD_SAME(LSHIFT, 0);
   add(SDL_SCANCODE_LALT, ALLEGRO_KEY_ALT, 0);
   add(SDL_SCANCODE_LGUI, ALLEGRO_KEY_LWIN, 0);
   ADD_SAME(RCTRL, 0);
   ADD_SAME(RSHIFT, 0);
   add(SDL_SCANCODE_RALT, ALLEGRO_KEY_ALTGR, 0);
   add(SDL_SCANCODE_RGUI, ALLEGRO_KEY_RWIN, 0);
   add(SDL_SCANCODE_MODE, 0, 0);
   add(SDL_SCANCODE_AUDIONEXT, 0, 0);
   add(SDL_SCANCODE_AUDIOPREV, 0, 0);
   add(SDL_SCANCODE_AUDIOSTOP, 0, 0);
   add(SDL_SCANCODE_AUDIOPLAY, 0, 0);
   add(SDL_SCANCODE_AUDIOMUTE, 0, 0);
   add(SDL_SCANCODE_MEDIASELECT, 0, 0);
   add(SDL_SCANCODE_WWW, 0, 0);
   add(SDL_SCANCODE_MAIL, 0, 0);
   add(SDL_SCANCODE_CALCULATOR, 0, 0);
   add(SDL_SCANCODE_COMPUTER, 0, 0);
   add(SDL_SCANCODE_AC_SEARCH, 0, 0);
   add(SDL_SCANCODE_AC_HOME, 0, 0);
   add(SDL_SCANCODE_AC_BACK, 0, 0);
   add(SDL_SCANCODE_AC_FORWARD, 0, 0);
   add(SDL_SCANCODE_AC_STOP, 0, 0);
   add(SDL_SCANCODE_AC_REFRESH, 0, 0);
   add(SDL_SCANCODE_AC_BOOKMARKS, 0, 0);
   add(SDL_SCANCODE_BRIGHTNESSDOWN, 0, 0);
   add(SDL_SCANCODE_BRIGHTNESSUP, 0, 0);
   add(SDL_SCANCODE_DISPLAYSWITCH, 0, 0);
   add(SDL_SCANCODE_KBDILLUMTOGGLE, 0, 0);
   add(SDL_SCANCODE_KBDILLUMDOWN, 0, 0);
   add(SDL_SCANCODE_KBDILLUMUP, 0, 0);
   add(SDL_SCANCODE_EJECT, 0, 0);
   add(SDL_SCANCODE_SLEEP, 0, 0);

   _al_sdl_event_hack();

   return true;
}

static void sdl_exit_keyboard(void)
{
}

static ALLEGRO_KEYBOARD *sdl_get_keyboard(void)
{
   return &keyboard->keyboard;
}

static bool sdl_set_keyboard_leds(int leds)
{
   (void)leds;
   return false;
}

static char const *sdl_keycode_to_name(int keycode)
{
   return SDL_GetScancodeName(keyboard->inverse[keycode]);
}

static void sdl_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
   int i, n;
   ALLEGRO_SYSTEM_INTERFACE *sdl = _al_sdl_system_driver();
   sdl->heartbeat();
   const Uint8 *s = SDL_GetKeyboardState(&n);
   for (i = 0; i < n; i++) {
      if (s[i])
         _AL_KEYBOARD_STATE_SET_KEY_DOWN(*ret_state, keyboard->table[i]);
      else
         _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(*ret_state, keyboard->table[i]);
   }
   ret_state->display = keyboard->display;
}

static void sdl_clear_keyboard_state(void)
{
   return;
}

ALLEGRO_KEYBOARD_DRIVER *_al_sdl_keyboard_driver(void)
{
   if (vt)
      return vt;

   vt = al_calloc(1, sizeof *vt);
   vt->keydrv_id = AL_ID('S','D','L','2');
   vt->keydrv_name = "SDL2 Keyboard";
   vt->keydrv_desc = "SDL2 Keyboard";
   vt->keydrv_ascii_name = "SDL2 Keyboard";
   vt->init_keyboard = sdl_init_keyboard;
   vt->exit_keyboard = sdl_exit_keyboard;
   vt->get_keyboard = sdl_get_keyboard;
   vt->set_keyboard_leds = sdl_set_keyboard_leds;
   vt->keycode_to_name = sdl_keycode_to_name;
   vt->get_keyboard_state = sdl_get_keyboard_state;
   vt->clear_keyboard_state = sdl_clear_keyboard_state;
   return vt;
}
