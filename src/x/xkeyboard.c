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
 *      X keyboard driver.
 *
 *      By Elias Pschernig.
 *
 *      See readme.txt for copyright information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/Xlocale.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "allegro.h"
#include "xalleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

// TODO: add KEY_UNKNOWN1 ... keys to Allegro, up KEY_MAX to 127
// TODO: scancode_to_ascii vtable entry? Maybe just have a fallback one in
//       keyboard.c, which does KEY_A -> 'a' and so on.

static int x_to_allegro_keycode[256];
#ifdef ALLEGRO_USE_XIM
static XIM xim = NULL;
static XIC xic = NULL;
#endif
static XModifierKeymap *xmodmap = NULL;
static int xkeyboard_installed = 0;
static int used[KEY_MAX];
static int sym_per_key;
static int min_keycode, max_keycode;
static KeySym *keysyms = NULL;

/* This table can be ammended to provide more reasonable defaults for
 * mappings other than US/UK. They are used to map X11 KeySyms as found in
 * X11/keysym.h to Allegro's KEY_* codes. This will only work well on US/UK
 * keyboards since Allegro simply doesn't have KEY_* codes for non US/UK
 * keyboards. So with other mappings, the unmapped keys will be distributed
 * arbitrarily to the remaining KEY_* codes.
 *
 * TODO: Better to just map them to KEY_UNKNOWN1 KEY_UNKNOWN2 ...
 *
 * Double mappings should be avoided, else they can lead to different keys
 * producing the same KEY_* code on some mappings.
 *
 * In cases where there is no other way to detect a key, since we have no
 * ASCII applied to it, like KEY_LEFT or KEY_PAUSE, multiple mappings should
 * be ok though. This table will never be able to be 100% perfect, so just
 * try to make it work for as many as possible, using additional hacks in
 * some cases. There is also still the possibility to override keys with
 * the [xkeyboard] config section, so users can always re-map keys. (E.g.
 * to play an Allegro game which hard-coded KEY_Y and KEY_X for left and
 * right.)
 */
static struct {
   KeySym keysym;
   int allegro_key;
}
translation_table[] = {
   {XK_a, KEY_A},
   {XK_b, KEY_B},
   {XK_c, KEY_C},
   {XK_d, KEY_D},
   {XK_e, KEY_E},
   {XK_f, KEY_F},
   {XK_g, KEY_G},
   {XK_h, KEY_H},
   {XK_i, KEY_I},
   {XK_j, KEY_J},
   {XK_k, KEY_K},
   {XK_l, KEY_L},
   {XK_m, KEY_M},
   {XK_n, KEY_N},
   {XK_o, KEY_O},
   {XK_p, KEY_P},
   {XK_q, KEY_Q},
   {XK_r, KEY_R},
   {XK_s, KEY_S},
   {XK_t, KEY_T},
   {XK_u, KEY_U},
   {XK_v, KEY_V},
   {XK_w, KEY_W},
   {XK_x, KEY_X},
   {XK_y, KEY_Y},
   {XK_z, KEY_Z},
   {XK_0, KEY_0},
   {XK_1, KEY_1},
   {XK_2, KEY_2},
   {XK_3, KEY_3},
   {XK_4, KEY_4},
   {XK_5, KEY_5},
   {XK_6, KEY_6},
   {XK_7, KEY_7},
   {XK_8, KEY_8},
   {XK_9, KEY_9},

   /* Double mappings for numeric keyboard.
    * If an X server actually uses both at the same time, Allegro will
    * detect them as the same. But normally, an X server just reports it as
    * either of them, and therefore we always get the keys as KEY_*_PAD.
    */
   {XK_KP_0, KEY_0_PAD},
   {XK_KP_Insert, KEY_0_PAD},
   {XK_KP_1, KEY_1_PAD},
   {XK_KP_End, KEY_1_PAD},
   {XK_KP_2, KEY_2_PAD},
   {XK_KP_Down, KEY_2_PAD},
   {XK_KP_3, KEY_3_PAD},
   {XK_KP_Page_Down, KEY_3_PAD},
   {XK_KP_4, KEY_4_PAD},
   {XK_KP_Left, KEY_4_PAD},
   {XK_KP_5, KEY_5_PAD},
   {XK_KP_Begin, KEY_5_PAD},
   {XK_KP_6, KEY_6_PAD},
   {XK_KP_Right, KEY_6_PAD},
   {XK_KP_7, KEY_7_PAD},
   {XK_KP_Home, KEY_7_PAD},
   {XK_KP_8, KEY_8_PAD},
   {XK_KP_Up, KEY_8_PAD},
   {XK_KP_9, KEY_9_PAD},
   {XK_KP_Page_Up, KEY_9_PAD},
   {XK_KP_Delete, KEY_DEL_PAD},
   {XK_KP_Decimal, KEY_DEL_PAD},

   /* Double mapping!
    * Same as above - but normally, the X server just reports one or the
    * other for the Pause key, and the other is not reported for any key.
    */
   {XK_Pause, KEY_PAUSE},
   {XK_Break, KEY_PAUSE},

   {XK_F1, KEY_F1},
   {XK_F2, KEY_F2},
   {XK_F3, KEY_F3},
   {XK_F4, KEY_F4},
   {XK_F5, KEY_F5},
   {XK_F6, KEY_F6},
   {XK_F7, KEY_F7},
   {XK_F8, KEY_F8},
   {XK_F9, KEY_F9},
   {XK_F10, KEY_F10},
   {XK_F11, KEY_F11},
   {XK_F12, KEY_F12},
   {XK_Escape, KEY_ESC},
   {XK_grave, KEY_TILDE}, /* US left of 1 */
   {XK_minus, KEY_MINUS}, /* US right of 0 */
   {XK_equal, KEY_EQUALS}, /* US 2 right of 0 */
   {XK_BackSpace, KEY_BACKSPACE},
   {XK_Tab, KEY_TAB},
   {XK_bracketleft, KEY_OPENBRACE}, /* US right of P */
   {XK_bracketright, KEY_CLOSEBRACE}, /* US 2 right of P */
   {XK_Return, KEY_ENTER},
   {XK_semicolon, KEY_COLON}, /* US right of L */
   {XK_apostrophe, KEY_QUOTE}, /* US 2 right of L */
   {XK_backslash, KEY_BACKSLASH}, /* US 3 right of L */
   {XK_less, KEY_BACKSLASH2}, /* US left of Y */
   {XK_comma, KEY_COMMA}, /* US right of M */
   {XK_period, KEY_STOP}, /* US 2 right of M */
   {XK_slash, KEY_SLASH}, /* US 3 right of M */
   {XK_space, KEY_SPACE},
   {XK_Insert, KEY_INSERT},
   {XK_Delete, KEY_DEL},
   {XK_Home, KEY_HOME},
   {XK_End, KEY_END},
   {XK_Page_Up, KEY_PGUP},
   {XK_Page_Down, KEY_PGDN},
   {XK_Left, KEY_LEFT},
   {XK_Right, KEY_RIGHT},
   {XK_Up, KEY_UP},
   {XK_Down, KEY_DOWN},
   {XK_KP_Divide, KEY_SLASH_PAD},
   {XK_KP_Multiply, KEY_ASTERISK},
   {XK_KP_Subtract, KEY_MINUS_PAD},
   {XK_KP_Add, KEY_PLUS_PAD},
   {XK_KP_Enter, KEY_ENTER_PAD},
   {XK_Print, KEY_PRTSCR},

   //{, KEY_ABNT_C1},
   //{, KEY_YEN},
   //{, KEY_KANA},
   //{, KEY_CONVERT},
   //{, KEY_NOCONVERT},
   //{, KEY_AT},
   //{, KEY_CIRCUMFLEX},
   //{, KEY_COLON2},
   //{, KEY_KANJI},
   {XK_KP_Equal, KEY_EQUALS_PAD},  /* MacOS X */
   //{, KEY_BACKQUOTE},  /* MacOS X */
   //{, KEY_SEMICOLON},  /* MacOS X */
   //{, KEY_COMMAND},  /* MacOS X */

   {XK_Shift_L, KEY_LSHIFT},
   {XK_Shift_R, KEY_RSHIFT},
   {XK_Control_L, KEY_LCONTROL},
   {XK_Control_R, KEY_RCONTROL},
   {XK_Alt_L, KEY_ALT},

   /* Double mappings. This is a bit of a problem, since you can configure
    * X11 differently to what report for those keys.
    */
   {XK_Alt_R, KEY_ALTGR},
   {XK_ISO_Level3_Shift, KEY_ALTGR},
   {XK_Meta_L, KEY_LWIN},
   {XK_Super_L, KEY_LWIN},
   {XK_Meta_R, KEY_RWIN},
   {XK_Super_R, KEY_RWIN},

   {XK_Menu, KEY_MENU},
   {XK_Scroll_Lock, KEY_SCRLOCK},
   {XK_Num_Lock, KEY_NUMLOCK},
   {XK_Caps_Lock, KEY_CAPSLOCK}
};



/* update_shifts
 *  Update Allegro's key_shifts variable, directly from the corresponding
 *  X11 modifiers state.
 */
static void update_shifts(XKeyEvent *event)
{
   int mask = 0;
   int i;
   /* Assume standard modifiers - may not work in every X11 server. */
   static int flags[8][2] = {
      {KB_SHIFT_FLAG, ShiftMask},
      {KB_CAPSLOCK_FLAG, LockMask},
      {KB_CTRL_FLAG, ControlMask},
      {KB_ALT_FLAG, Mod1Mask},
      {KB_NUMLOCK_FLAG, Mod2Mask},
      {KB_SCROLOCK_FLAG, Mod3Mask},
      {KB_LWIN_FLAG | KB_RWIN_FLAG, Mod4Mask}, /* Should we use only one? */
      {KB_MENU_FLAG, Mod5Mask} /* AltGr */
   };

   for (i = 0; i < 8; i++)
   {
      int j;
      /* This is the state of the modifiers just before the key
       * press/release.
       */
      if (event->state & flags[i][1])
	 mask |= flags[i][0];
      /* In case a modifier key itself was pressed, we now need to update
       * the above state for Allegro, which wants the state after the
       * press, not before as reported by X.
       */
      if (event->keycode) {
	 for (j = 0; j < xmodmap->max_keypermod; j++) {
	    if (event->keycode == xmodmap->modifiermap[i * xmodmap->max_keypermod + j]) {
	       /* Modifier key pressed - so set flag. */
	       if (event->type == KeyPress)
	       {
		  mask |= flags[i][0];
	       }
	       /* Modifier key released - so remove flag. */
	       if (event->type == KeyRelease)
		  mask &= ~flags[i][0];
	    }
	 }
      }
   }
   _key_shifts = mask;
}



/* find_unknown_key_assignment
 *  In some cases, X11 doesn't report any KeySym for a key - so the earliest
 *  time we can map it to an Allegro key is when it is first pressed.
 */
static int find_unknown_key_assignment (int i)
{
   int j;
   for (j = 1; j < KEY_MAX; j++) {
      if (!used[j]) {
	 x_to_allegro_keycode[i] = j;
	 used[j] = 1;
	 break;
      }
   }

   if (j == KEY_MAX) {
      TRACE ("Fatal: You have more keys reported by X than Allegro's "
	     "maximum of %i keys. Please send a bug report.\n", KEY_MAX);
      x_to_allegro_keycode[i] = 0;
   }

   TRACE ("Key %i missing:", i);
   for (j = 0; j < sym_per_key; j++)
      TRACE (" %s", XKeysymToString(
	 keysyms[sym_per_key * (i - min_keycode) + j]));
   TRACE (" - assigned to %i.\n", x_to_allegro_keycode[i]);

   return x_to_allegro_keycode[i];
}



/* _xwin_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void _xwin_keyboard_handler(XKeyEvent *event)
{
   int keycode;
   if (!xkeyboard_installed)
      return;

   keycode = x_to_allegro_keycode[event->keycode];
   if (keycode == -1)
      keycode = find_unknown_key_assignment (event->keycode);

   update_shifts (event);

   if (event->type == KeyPress) { /* Key pressed.  */
      int len;
      char buffer[16];
      char buffer2[16];
      int unicode = 0, r = 0;

#if defined (ALLEGRO_USE_XIM) && defined(X_HAVE_UTF8_STRING)
      if (xic)
	 len = Xutf8LookupString(xic, event, buffer, sizeof buffer, NULL, NULL);
      else
#endif
      /* XLookupString is supposed to only use ASCII. */
	 len = XLookupString(event, buffer, sizeof buffer, NULL, NULL);
      buffer[len] = '\0';
      uconvert(buffer, U_UTF8, buffer2, U_UNICODE, sizeof buffer2);
      unicode = *(unsigned short *)buffer2;

#ifdef ALLEGRO_USE_XIM
      r = XFilterEvent ((XEvent *)event, _xwin.window);
#endif
      if (keycode || unicode) {
	 /* If we have a keycode, we want it to go to Allegro immediately, so the
	  * key[] array is updated, and the user callbacks are called. OTOH, a key
	  * should not be added to the keyboard buffer (parameter -1) if it was
          * filtered out as a compose key, or if it is a modifier key.
	  */
	 if (r || keycode >= KEY_MODIFIERS)
	    unicode = -1;
	 else {
	    /* Historically, Allegro expects to get only the scancode when Alt is
	     * held down.
	     */
	    if (_key_shifts & KB_ALT_FLAG)
	       unicode = 0;
         }
	 _handle_key_press(unicode, keycode);
      }
   }
   else { /* Key release. */
      _handle_key_release(keycode);
   }
}



/* _xwin_keyboard_focus_handler:
 *  Handles switching of X keyboard focus.
 */
void _xwin_keyboard_focus_handler (XFocusChangeEvent *event)
{
   /* Simulate release of all keys on focus out. */
   if (event->type == FocusOut) {
      int i;
      for (i = 0; i < KEY_MAX; i++) {
	 if (key[i])
	    _handle_key_release(i);
      }
   }
}



/* find_allegro_key
 *  Search the translation table for the Allegro key corresponding to the
 *  given KeySym.
 */
static int find_allegro_key(KeySym sym)
{
   int i;
   int n = sizeof translation_table / sizeof *translation_table;
   for (i = 0; i < n; i++) {
      if (translation_table[i].keysym == sym)
	 return translation_table[i].allegro_key;
   }
   return 0;
}



/* x_get_keyboard_mapping:
 *  Generate a mapping from X11 keycodes to Allegro KEY_* codes. We have
 *  two goals: Every keypress should be mapped to a distinct Allegro KEY_*
 *  code. And we want the KEY_* codes to match the pressed
 *  key to some extent. To do the latter, the X11 KeySyms produced by a key
 *  are examined. If a match is found in the table above, the mapping is
 *  added to the mapping table. If no known KeySym is found for a key (or
 *  the same KeySym is found for more keys) - the remaining keys are
 *  distributed arbitrarily to the remaining KEY_* codes.
 *
 *  In a future version, this could be simplified by mapping *all* the X11
 *  KeySyms to KEY_* codes.
 */
void _xwin_get_keyboard_mapping(void)
{
   int i;
   int count;
   int missing = 0;

   memset (used, 0, sizeof used);
   memset (x_to_allegro_keycode, 0, sizeof x_to_allegro_keycode);

   XLOCK ();

   XDisplayKeycodes(_xwin.display, &min_keycode, &max_keycode);
   count = 1 + max_keycode - min_keycode;

   if (keysyms)
      XFree (keysyms);
   keysyms = XGetKeyboardMapping(_xwin.display, min_keycode,
      count, &sym_per_key);

   TRACE ("xkeyboard: %i keys, %i symbols per key.\n", count, sym_per_key);

   missing = 0;
   for (i = min_keycode; i <= max_keycode; i++) {
      KeySym sym = keysyms[sym_per_key * (i - min_keycode)];

      TRACE ("key %i: %s", i, XKeysymToString(sym));

      if (sym != NoSymbol) {
	 int allegro_key = find_allegro_key(sym);

	 if (allegro_key == 0) {
	    missing++;
            TRACE (" defering.");
	 }
	 else {
	    if (used[allegro_key])
	       TRACE (" *double*");
	    x_to_allegro_keycode[i] = allegro_key;
	    used[allegro_key] = 1;
	    TRACE (" assigned to %i.", allegro_key);
	 }

	  TRACE ("\n");
      }
      else
      {
	 /* No KeySym for this key - ignore it. */
	 x_to_allegro_keycode[i] = -1;
	 TRACE (" not assigned.\n");
      }
   }

   if (missing) {
      /* The keys still not assigned are just assigned arbitrarily now. */
      for (i = min_keycode; i <= max_keycode; i++) {
	 if (x_to_allegro_keycode[i] == 0) {
	    find_unknown_key_assignment (i);
	 }
      }
   }

   if (xmodmap)
      XFreeModifiermap(xmodmap);
   xmodmap = XGetModifierMapping (_xwin.display);

   /* The [xkeymap] section can be useful, e.g. if trying to play a
    * game which has X and Y hardcoded as KEY_X and KEY_Y to mean
    * left/right movement, but on the X11 keyboard X and Y are far apart.
    * For normal use, a user never should have to touch [xkeymap] anymore
    * though, and proper written programs will not hardcode such mappings.
    */
   {
      char *section, *option_format;
      char option[128], tmp1[128], tmp2[128];
      section = uconvert_ascii("xkeymap", tmp1);
      option_format = uconvert_ascii("keycode%d", tmp2);

      for (i = min_keycode; i <= max_keycode; i++) {
	 int scancode;

	 uszprintf(option, sizeof(option), option_format, i);
	 scancode = get_config_int(section, option, -1);
	 if (scancode > 0)
	 {
	    x_to_allegro_keycode[i] = scancode;
	    TRACE ("User override: KeySym %i assigned to %i.\n", i, scancode);
	 }
      }
   }

   XUNLOCK ();
}



/* x_set_leds:
 *  Update the keyboard LEDs.
 */
static void x_set_leds(int leds)
{
   XKeyboardControl values;

   if (!xkeyboard_installed)
      return;

   XLOCK();

   values.led = 1;
   values.led_mode = leds & KB_NUMLOCK_FLAG ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);

   values.led = 2;
   values.led_mode = leds & KB_CAPSLOCK_FLAG ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);

   values.led = 3;
   values.led_mode = leds & KB_SCROLOCK_FLAG ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);

   XUNLOCK();
}



/* x_keyboard_init
 *  Initialise the X11 keyboard driver.
 */
static int x_keyboard_init(void)
{
#ifdef ALLEGRO_USE_XIM
   XIMStyles *xim_styles;
   XIMStyle xim_style = 0;
   char *modifiers;
   char *imvalret;
   int i;
#endif

   if (xkeyboard_installed)
      return 0;

   XLOCK ();

#ifdef ALLEGRO_USE_XIM
   if (setlocale(LC_ALL,"") == NULL) {
      TRACE("x keyboard warning: Could not set default locale.\n");
   }

   modifiers = XSetLocaleModifiers ("@im=none");
   if (modifiers == NULL) {
      TRACE ("x keyboard warning: XSetLocaleModifiers failed.\n");
   }

   xim = XOpenIM (_xwin.display, NULL, NULL, NULL);
   if (xim == NULL) {
      TRACE("x keyboard warning: XOpenIM failed.\n");
   }

   if (xim) {
      imvalret = XGetIMValues (xim, XNQueryInputStyle, &xim_styles, NULL);
      if (imvalret != NULL || xim_styles == NULL) {
	 TRACE("x keyboard warning: Input method doesn't support any styles.\n");
      }

      if (xim_styles) {
	 xim_style = 0;
	 for (i = 0;  i < xim_styles->count_styles;  i++) {
	    if (xim_styles->supported_styles[i] ==
	       (XIMPreeditNothing | XIMStatusNothing)) {
	       xim_style = xim_styles->supported_styles[i];
	       break;
	    }
	 }

	 if (xim_style == 0) {
	    TRACE ("x keyboard warning: Input method doesn't support the style we support.\n");
	 }
	 XFree (xim_styles);
      }
   }

   if (xim && xim_style) {
      xic = XCreateIC (xim,
		       XNInputStyle, xim_style,
		       XNClientWindow, _xwin.window,
		       XNFocusWindow, _xwin.window,
		       NULL);

      if (xic == NULL) {
	 TRACE ("x keyboard warning: XCreateIC failed.\n");
      }
   }
#endif

   _xwin_get_keyboard_mapping ();

   XUNLOCK ();

   xkeyboard_installed = 1;

   return 0;
}



/* x_keyboard_exit
 *  Shut down the X11 keyboard driver.
 */
static void x_keyboard_exit(void)
{
   if (!xkeyboard_installed)
      return;
   xkeyboard_installed = 0;

   XLOCK ();

#ifdef ALLEGRO_USE_XIM

   if (xic) {
      XDestroyIC(xic);
      xic = NULL;
   }

   if (xim) {
      XCloseIM(xim);
      xim = NULL;
   }

#endif

   if (xmodmap) {
      XFreeModifiermap(xmodmap);
      xmodmap = NULL;
   }

   if (keysyms) {
      XFree (keysyms);
      keysyms = NULL;
   }

   XUNLOCK ();
}



static KEYBOARD_DRIVER keyboard_x =
{
   KEYBOARD_XWINDOWS,
   "X11 keyboard",
   "X11 keyboard",
   "X11 keyboard",
   TRUE,
   x_keyboard_init,
   x_keyboard_exit,
   NULL,   // AL_METHOD(void, poll, (void));
   x_set_leds,
   NULL,   // AL_METHOD(void, set_rate, (int delay, int rate));
   NULL,   // AL_METHOD(void, wait_for_input, (void));
   NULL,   // AL_METHOD(void, stop_waiting_for_input, (void));
   NULL    // AL_METHOD(int,  scancode_to_ascii, (int scancode));
};

/* list the available drivers */
_DRIVER_INFO _xwin_keyboard_driver_list[] =
{
   {  KEYBOARD_XWINDOWS, &keyboard_x,    TRUE  },
   {  0,                 NULL,           0     }
};
