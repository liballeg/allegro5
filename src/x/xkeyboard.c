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
 *      Modified for the new keyboard API by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

#define ALLEGRO_NO_COMPATIBILITY

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <X11/Xlocale.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
//#include "xwin.h"
#include "../xglx/xglx.h"

/*----------------------------------------------------------------------*/
static void handle_key_press(int mycode, int unichar, unsigned int modifiers,
    ALLEGRO_DISPLAY *display);
static void handle_key_release(int mycode, ALLEGRO_DISPLAY *display);
static int _key_shifts;
/*----------------------------------------------------------------------*/

#define PREFIX_I                "al-xkey INFO: "
#define PREFIX_W                "al-xkey WARNING: "
#define PREFIX_E                "al-xkey ERROR: "

#ifdef ALLEGRO_XWINDOWS_WITH_XIM
static XIM xim = NULL;
static XIC xic = NULL;
#endif
static XModifierKeymap *xmodmap = NULL;
static int xkeyboard_installed = 0;
static int used[ALLEGRO_KEY_MAX];
static int sym_per_key;
static int min_keycode, max_keycode;
static KeySym *keysyms = NULL;
static int main_pid; /* The pid to kill with ctrl-alt-del. */
static int pause_key = 0; /* Allegro's special pause key state. */
static int keycode_to_scancode[256];

/* This table can be ammended to provide more reasonable defaults for
 * mappings other than US/UK. They are used to map X11 KeySyms as found in
 * X11/keysym.h to Allegro's ALLEGRO_KEY_* codes. This will only work well on US/UK
 * keyboards since Allegro simply doesn't have ALLEGRO_KEY_* codes for non US/UK
 * keyboards. So with other mappings, the unmapped keys will be distributed
 * arbitrarily to the remaining ALLEGRO_KEY_* codes.
 *
 * Double mappings should be avoided, else they can lead to different keys
 * producing the same ALLEGRO_KEY_* code on some mappings.
 *
 * In cases where there is no other way to detect a key, since we have no
 * ASCII applied to it, like ALLEGRO_KEY_LEFT or ALLEGRO_KEY_PAUSE, multiple mappings should
 * be ok though. This table will never be able to be 100% perfect, so just
 * try to make it work for as many as possible, using additional hacks in
 * some cases. There is also still the possibility to override keys with
 * the [xkeyboard] config section, so users can always re-map keys. (E.g.
 * to play an Allegro game which hard-coded ALLEGRO_KEY_Y and ALLEGRO_KEY_X for left and
 * right.)
 */
static struct {
   KeySym keysym;
   int allegro_key;
}
translation_table[] = {
   {XK_a, ALLEGRO_KEY_A},
   {XK_b, ALLEGRO_KEY_B},
   {XK_c, ALLEGRO_KEY_C},
   {XK_d, ALLEGRO_KEY_D},
   {XK_e, ALLEGRO_KEY_E},
   {XK_f, ALLEGRO_KEY_F},
   {XK_g, ALLEGRO_KEY_G},
   {XK_h, ALLEGRO_KEY_H},
   {XK_i, ALLEGRO_KEY_I},
   {XK_j, ALLEGRO_KEY_J},
   {XK_k, ALLEGRO_KEY_K},
   {XK_l, ALLEGRO_KEY_L},
   {XK_m, ALLEGRO_KEY_M},
   {XK_n, ALLEGRO_KEY_N},
   {XK_o, ALLEGRO_KEY_O},
   {XK_p, ALLEGRO_KEY_P},
   {XK_q, ALLEGRO_KEY_Q},
   {XK_r, ALLEGRO_KEY_R},
   {XK_s, ALLEGRO_KEY_S},
   {XK_t, ALLEGRO_KEY_T},
   {XK_u, ALLEGRO_KEY_U},
   {XK_v, ALLEGRO_KEY_V},
   {XK_w, ALLEGRO_KEY_W},
   {XK_x, ALLEGRO_KEY_X},
   {XK_y, ALLEGRO_KEY_Y},
   {XK_z, ALLEGRO_KEY_Z},
   {XK_0, ALLEGRO_KEY_0},
   {XK_1, ALLEGRO_KEY_1},
   {XK_2, ALLEGRO_KEY_2},
   {XK_3, ALLEGRO_KEY_3},
   {XK_4, ALLEGRO_KEY_4},
   {XK_5, ALLEGRO_KEY_5},
   {XK_6, ALLEGRO_KEY_6},
   {XK_7, ALLEGRO_KEY_7},
   {XK_8, ALLEGRO_KEY_8},
   {XK_9, ALLEGRO_KEY_9},

   /* Double mappings for numeric keyboard.
    * If an X server actually uses both at the same time, Allegro will
    * detect them as the same. But normally, an X server just reports it as
    * either of them, and therefore we always get the keys as ALLEGRO_KEY_PAD_*.
    */
   {XK_KP_0, ALLEGRO_KEY_PAD_0},
   {XK_KP_Insert, ALLEGRO_KEY_PAD_0},
   {XK_KP_1, ALLEGRO_KEY_PAD_1},
   {XK_KP_End, ALLEGRO_KEY_PAD_1},
   {XK_KP_2, ALLEGRO_KEY_PAD_2},
   {XK_KP_Down, ALLEGRO_KEY_PAD_2},
   {XK_KP_3, ALLEGRO_KEY_PAD_3},
   {XK_KP_Page_Down, ALLEGRO_KEY_PAD_3},
   {XK_KP_4, ALLEGRO_KEY_PAD_4},
   {XK_KP_Left, ALLEGRO_KEY_PAD_4},
   {XK_KP_5, ALLEGRO_KEY_PAD_5},
   {XK_KP_Begin, ALLEGRO_KEY_PAD_5},
   {XK_KP_6, ALLEGRO_KEY_PAD_6},
   {XK_KP_Right, ALLEGRO_KEY_PAD_6},
   {XK_KP_7, ALLEGRO_KEY_PAD_7},
   {XK_KP_Home, ALLEGRO_KEY_PAD_7},
   {XK_KP_8, ALLEGRO_KEY_PAD_8},
   {XK_KP_Up, ALLEGRO_KEY_PAD_8},
   {XK_KP_9, ALLEGRO_KEY_PAD_9},
   {XK_KP_Page_Up, ALLEGRO_KEY_PAD_9},
   {XK_KP_Delete, ALLEGRO_KEY_PAD_DELETE},
   {XK_KP_Decimal, ALLEGRO_KEY_PAD_DELETE},

   /* Double mapping!
    * Same as above - but normally, the X server just reports one or the
    * other for the Pause key, and the other is not reported for any key.
    */
   {XK_Pause, ALLEGRO_KEY_PAUSE},
   {XK_Break, ALLEGRO_KEY_PAUSE},

   {XK_F1, ALLEGRO_KEY_F1},
   {XK_F2, ALLEGRO_KEY_F2},
   {XK_F3, ALLEGRO_KEY_F3},
   {XK_F4, ALLEGRO_KEY_F4},
   {XK_F5, ALLEGRO_KEY_F5},
   {XK_F6, ALLEGRO_KEY_F6},
   {XK_F7, ALLEGRO_KEY_F7},
   {XK_F8, ALLEGRO_KEY_F8},
   {XK_F9, ALLEGRO_KEY_F9},
   {XK_F10, ALLEGRO_KEY_F10},
   {XK_F11, ALLEGRO_KEY_F11},
   {XK_F12, ALLEGRO_KEY_F12},
   {XK_Escape, ALLEGRO_KEY_ESCAPE},
   {XK_grave, ALLEGRO_KEY_TILDE}, /* US left of 1 */
   {XK_minus, ALLEGRO_KEY_MINUS}, /* US right of 0 */
   {XK_equal, ALLEGRO_KEY_EQUALS}, /* US 2 right of 0 */
   {XK_BackSpace, ALLEGRO_KEY_BACKSPACE},
   {XK_Tab, ALLEGRO_KEY_TAB},
   {XK_bracketleft, ALLEGRO_KEY_OPENBRACE}, /* US right of P */
   {XK_bracketright, ALLEGRO_KEY_CLOSEBRACE}, /* US 2 right of P */
   {XK_Return, ALLEGRO_KEY_ENTER},
   {XK_semicolon, ALLEGRO_KEY_SEMICOLON}, /* US right of L */
   {XK_apostrophe, ALLEGRO_KEY_QUOTE}, /* US 2 right of L */
   {XK_backslash, ALLEGRO_KEY_BACKSLASH}, /* US 3 right of L */
   {XK_less, ALLEGRO_KEY_BACKSLASH2}, /* US left of Y */
   {XK_comma, ALLEGRO_KEY_COMMA}, /* US right of M */
   {XK_period, ALLEGRO_KEY_FULLSTOP}, /* US 2 right of M */
   {XK_slash, ALLEGRO_KEY_SLASH}, /* US 3 right of M */
   {XK_space, ALLEGRO_KEY_SPACE},
   {XK_Insert, ALLEGRO_KEY_INSERT},
   {XK_Delete, ALLEGRO_KEY_DELETE},
   {XK_Home, ALLEGRO_KEY_HOME},
   {XK_End, ALLEGRO_KEY_END},
   {XK_Page_Up, ALLEGRO_KEY_PGUP},
   {XK_Page_Down, ALLEGRO_KEY_PGDN},
   {XK_Left, ALLEGRO_KEY_LEFT},
   {XK_Right, ALLEGRO_KEY_RIGHT},
   {XK_Up, ALLEGRO_KEY_UP},
   {XK_Down, ALLEGRO_KEY_DOWN},
   {XK_KP_Divide, ALLEGRO_KEY_PAD_SLASH},
   {XK_KP_Multiply, ALLEGRO_KEY_PAD_ASTERISK},
   {XK_KP_Subtract, ALLEGRO_KEY_PAD_MINUS},
   {XK_KP_Add, ALLEGRO_KEY_PAD_PLUS},
   {XK_KP_Enter, ALLEGRO_KEY_PAD_ENTER},
   {XK_Print, ALLEGRO_KEY_PRINTSCREEN},

   //{, ALLEGRO_KEY_ABNT_C1},
   //{, ALLEGRO_KEY_YEN},
   //{, ALLEGRO_KEY_KANA},
   //{, ALLEGRO_KEY_CONVERT},
   //{, ALLEGRO_KEY_NOCONVERT},
   //{, ALLEGRO_KEY_AT},
   //{, ALLEGRO_KEY_CIRCUMFLEX},
   //{, ALLEGRO_KEY_COLON2},
   //{, ALLEGRO_KEY_KANJI},
   {XK_KP_Equal, ALLEGRO_KEY_EQUALS_PAD},  /* MacOS X */
   //{, ALLEGRO_KEY_BACKQUOTE},  /* MacOS X */
   //{, ALLEGRO_KEY_SEMICOLON},  /* MacOS X */
   //{, ALLEGRO_KEY_COMMAND},  /* MacOS X */

   {XK_Shift_L, ALLEGRO_KEY_LSHIFT},
   {XK_Shift_R, ALLEGRO_KEY_RSHIFT},
   {XK_Control_L, ALLEGRO_KEY_LCTRL},
   {XK_Control_R, ALLEGRO_KEY_RCTRL},
   {XK_Alt_L, ALLEGRO_KEY_ALT},

   /* Double mappings. This is a bit of a problem, since you can configure
    * X11 differently to what report for those keys.
    */
   {XK_Alt_R, ALLEGRO_KEY_ALTGR},
   {XK_ISO_Level3_Shift, ALLEGRO_KEY_ALTGR},
   {XK_Meta_L, ALLEGRO_KEY_LWIN},
   {XK_Super_L, ALLEGRO_KEY_LWIN},
   {XK_Meta_R, ALLEGRO_KEY_RWIN},
   {XK_Super_R, ALLEGRO_KEY_RWIN},

   {XK_Menu, ALLEGRO_KEY_MENU},
   {XK_Scroll_Lock, ALLEGRO_KEY_SCROLLLOCK},
   {XK_Num_Lock, ALLEGRO_KEY_NUMLOCK},
   {XK_Caps_Lock, ALLEGRO_KEY_CAPSLOCK}
};

/* Table of: Allegro's modifier flag, associated X11 flag, toggle method. */
static int modifier_flags[8][3] = {
   {ALLEGRO_KEYMOD_SHIFT, ShiftMask, 0},
   {ALLEGRO_KEYMOD_CAPSLOCK, LockMask, 1},
   {ALLEGRO_KEYMOD_CTRL, ControlMask, 0},
   {ALLEGRO_KEYMOD_ALT, Mod1Mask, 0},
   {ALLEGRO_KEYMOD_NUMLOCK, Mod2Mask, 1},
   {ALLEGRO_KEYMOD_SCROLLLOCK, Mod3Mask, 1},
   {ALLEGRO_KEYMOD_LWIN | ALLEGRO_KEYMOD_RWIN, Mod4Mask, 0}, /* Should we use only one? */
   {ALLEGRO_KEYMOD_MENU, Mod5Mask, 0} /* AltGr */
};

/* Table of key names. */
static char AL_CONST *key_names[1 + ALLEGRO_KEY_MAX];



/* update_shifts
 *  Update Allegro's key_shifts variable, directly from the corresponding
 *  X11 modifiers state.
 */
static void update_shifts(XKeyEvent *event)
{
   int mask = 0;
   int i;

   for (i = 0; i < 8; i++) {
      int j;
      /* This is the state of the modifiers just before the key
       * press/release.
       */
      if (event->state & modifier_flags[i][1])
         mask |= modifier_flags[i][0];

      /* In case a modifier key itself was pressed, we now need to update
       * the above state for Allegro, which wants the state after the
       * press, not before as reported by X.
       */
      for (j = 0; j < xmodmap->max_keypermod; j++) {
         if (event->keycode && event->keycode ==
            xmodmap->modifiermap[i * xmodmap->max_keypermod + j]) {
            if (event->type == KeyPress) {
               /* Modifier key pressed - toggle or set flag. */
               if (modifier_flags[i][2])
                  mask ^= modifier_flags[i][0];
               else
                  mask |= modifier_flags[i][0];
            }
            else if (event->type == KeyRelease) {
               /* Modifier key of non-toggle key released - remove flag. */
               if (!modifier_flags[i][2])
                  mask &= ~modifier_flags[i][0];
            }
         }
      }
   }
   _key_shifts = mask;
}



/* dga2_update_shifts
 *  DGA2 doesn't seem to have a reliable state field. Therefore Allegro must
 *  take care of modifier keys itself.
 */
static void dga2_update_shifts(XKeyEvent *event)
{
   int i, j;

   for (i = 0; i < 8; i++) {
      for (j = 0; j < xmodmap->max_keypermod; j++) {
         if (event->keycode && event->keycode ==
            xmodmap->modifiermap[i * xmodmap->max_keypermod + j]) {
            if (event->type == KeyPress) {
               if (modifier_flags[i][2])
                  _key_shifts ^= modifier_flags[i][0];
               else
                  _key_shifts |= modifier_flags[i][0];
            }
            else if (event->type == KeyRelease) {
               if (!modifier_flags[i][2])
                  _key_shifts &= ~modifier_flags[i][0];
            }
         }
      }
      /* Hack: DGA keys seem to get reported wrong otherwise. */
      if (_key_shifts & modifier_flags[i][0])
         event->state |= modifier_flags[i][1];
   }
}



/* find_unknown_key_assignment
 *  In some cases, X11 doesn't report any KeySym for a key - so the earliest
 *  time we can map it to an Allegro key is when it is first pressed.
 */
static int find_unknown_key_assignment(int i)
{
   int j;

   for (j = 1; j < ALLEGRO_KEY_MAX; j++) {
      if (!used[j]) {
         AL_CONST char *str;
         keycode_to_scancode[i] = j;
         str = XKeysymToString(keysyms[sym_per_key * (i - min_keycode)]);
         if (str)
            key_names[j] = str;
         else {
            key_names[j] = _al_keyboard_common_names[j];
         }
         used[j] = 1;
         break;
      }
   }

   if (j == ALLEGRO_KEY_MAX) {
      TRACE (PREFIX_E "You have more keys reported by X than Allegro's "
             "maximum of %i keys. Please send a bug report.\n", ALLEGRO_KEY_MAX);
      keycode_to_scancode[i] = 0;
   }

   TRACE(PREFIX_I "Key %i missing:", i);
   for (j = 0; j < sym_per_key; j++) {
      char *sym_str = XKeysymToString(keysyms[sym_per_key * (i - min_keycode) + j]);
      TRACE(" %s", sym_str ? sym_str : "NULL");
   }
   TRACE(" - assigned to %i.\n", keycode_to_scancode[i]);

   return keycode_to_scancode[i];
}



/* _al_xwin_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void _al_xwin_keyboard_handler(XKeyEvent *event, bool dga2_hack,
    ALLEGRO_DISPLAY *display)
{
   int keycode;

   if (!xkeyboard_installed)
      return;

   keycode = keycode_to_scancode[event->keycode];
   if (keycode == -1)
      keycode = find_unknown_key_assignment(event->keycode);

   if (dga2_hack)
      dga2_update_shifts(event);
   else
      update_shifts(event);

   /* Special case the pause key. */
   if (keycode == ALLEGRO_KEY_PAUSE) {
      /* Allegro ignore's releasing of the pause key. */
      if (event->type == KeyRelease)
         return;
      if (pause_key) {
         event->type = KeyRelease;
         pause_key = 0;
      }
      else {
         pause_key = 1;
      }
   }

   if (event->type == KeyPress) { /* Key pressed.  */
      int len;
      char buffer[16];
      char buffer2[16];
      int unicode = 0, r = 0;

#if defined (ALLEGRO_XWINDOWS_WITH_XIM) && defined(X_HAVE_UTF8_STRING)
      if (xic) {
         len = Xutf8LookupString(xic, event, buffer, sizeof buffer, NULL, NULL);
      }
      else
#endif
      {
         /* XLookupString is supposed to only use ASCII. */
         len = XLookupString(event, buffer, sizeof buffer, NULL, NULL);
      }
      buffer[len] = '\0';
      uconvert(buffer, U_UTF8, buffer2, U_UNICODE, sizeof buffer2);
      unicode = *(unsigned short *)buffer2;

#ifdef ALLEGRO_XWINDOWS_WITH_XIM
      r = XFilterEvent((XEvent *)event, display->window);
#endif
      if (keycode || unicode) {
         /* If we have a keycode, we want it to go to Allegro immediately, so the
          * key[] array is updated, and the user callbacks are called. OTOH, a key
          * should not be added to the keyboard buffer (parameter -1) if it was
          * filtered out as a compose key, or if it is a modifier key.
          */
         if (r || keycode >= ALLEGRO_KEY_MODIFIERS)
            unicode = -1;
         else {
            /* Historically, Allegro expects to get only the scancode when Alt is
             * held down.
             */
            if (_key_shifts & ALLEGRO_KEYMOD_ALT)
               unicode = 0;
         }

         handle_key_press(keycode, unicode, _key_shifts, display);
      }
   }
   else { /* Key release. */
      handle_key_release(keycode, display);
   }
}



/* _al_xwin_keyboard_focus_handler:
 *  Handles switching of X keyboard focus.
 */
void _al_xwin_keyboard_focus_handler(XFocusChangeEvent *event)
{
   /* TODO */

#if 0
   /* Simulate release of all keys on focus out. */
   if (event->type == FocusOut) {
      int i;

      for (i = 0; i < ALLEGRO_KEY_MAX; i++) {
         if (key[i])
            handle_key_release(i, display);
      }
   }
#endif
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



/* scancode_to_name:
 *  Converts the given scancode to a description of the key.
 */
static AL_CONST char *x_scancode_to_name(int scancode)
{
   ASSERT (scancode >= 0 && scancode < ALLEGRO_KEY_MAX);

   return key_names[scancode];
}



/* al_xwin_get_keyboard_mapping:
 *  Generate a mapping from X11 keycodes to Allegro ALLEGRO_KEY_* codes. We have
 *  two goals: Every keypress should be mapped to a distinct Allegro ALLEGRO_KEY_*
 *  code. And we want the ALLEGRO_KEY_* codes to match the pressed
 *  key to some extent. To do the latter, the X11 KeySyms produced by a key
 *  are examined. If a match is found in the table above, the mapping is
 *  added to the mapping table. If no known KeySym is found for a key (or
 *  the same KeySym is found for more keys) - the remaining keys are
 *  distributed arbitrarily to the remaining ALLEGRO_KEY_* codes.
 *
 *  In a future version, this could be simplified by mapping *all* the X11
 *  KeySyms to ALLEGRO_KEY_* codes.
 */

static void _al_xwin_get_keyboard_mapping(void)
{
   int i;
   int count;
   int missing = 0;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();

   memset(used, 0, sizeof used);
   memset(keycode_to_scancode, 0, sizeof keycode_to_scancode);

   XDisplayKeycodes(system->x11display, &min_keycode, &max_keycode);
   count = 1 + max_keycode - min_keycode;

   if (keysyms) {
      XFree(keysyms);
   }
   keysyms = XGetKeyboardMapping(system->x11display, min_keycode,
      count, &sym_per_key);

   TRACE (PREFIX_I "%i keys, %i symbols per key.\n", count, sym_per_key);

   missing = 0;

   for (i = min_keycode; i <= max_keycode; i++) {
      KeySym sym = keysyms[sym_per_key * (i - min_keycode)];
      KeySym sym2 =  keysyms[sym_per_key * (i - min_keycode) + 1];
      char *sym_str, *sym2_str;
      int allegro_key = 0;

      sym_str = XKeysymToString(sym);
      sym2_str = XKeysymToString(sym2);

      TRACE (PREFIX_I "key [%i: %s %s]", i, sym_str ? sym_str : "NULL", sym2_str ?
         sym2_str : "NULL");

      /* Hack for French keyboards, to correctly map ALLEGRO_KEY_0 to ALLEGRO_KEY_9. */
      if (sym2 >= XK_0 && sym2 <= XK_9) {
         allegro_key = find_allegro_key(sym2);
      }

      if (!allegro_key) {
         if (sym != NoSymbol) {
            allegro_key = find_allegro_key(sym);

            if (allegro_key == 0) {
               missing++;
               TRACE (" defering.\n");
            }
         }
         else {
            /* No KeySym for this key - ignore it. */
            keycode_to_scancode[i] = -1;
            TRACE (" not assigned.\n");
         }
      }

      if (allegro_key) {
         if (used[allegro_key]) {
            TRACE(" *double*");
         }
         keycode_to_scancode[i] = allegro_key;
         key_names[allegro_key] =
            XKeysymToString(keysyms[sym_per_key * (i - min_keycode)]);
         used[allegro_key] = 1;
         TRACE(" assigned to %i.\n", allegro_key);
      }
   }

   if (missing) {
      /* The keys still not assigned are just assigned arbitrarily now. */
      for (i = min_keycode; i <= max_keycode; i++) {
         if (keycode_to_scancode[i] == 0) {
            find_unknown_key_assignment(i);
         }
      }
   }

   if (xmodmap)
      XFreeModifiermap(xmodmap);
   xmodmap = XGetModifierMapping(system->x11display);
   for (i = 0; i < 8; i++) {
      int j;

      TRACE (PREFIX_I "Modifier %d:", i + 1);
      for (j = 0; j < xmodmap->max_keypermod; j++) {
         KeySym sym = XKeycodeToKeysym(system->x11display,
            xmodmap->modifiermap[i * xmodmap->max_keypermod + j], 0);
         char *sym_str = XKeysymToString(sym);
         TRACE(" %s", sym_str ? sym_str : "NULL");
      }
      TRACE("\n");
   }

   /* The [xkeymap] section can be useful, e.g. if trying to play a
    * game which has X and Y hardcoded as ALLEGRO_KEY_X and ALLEGRO_KEY_Y to mean
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
         if (scancode > 0) {
            keycode_to_scancode[i] = scancode;
            TRACE(PREFIX_I "User override: KeySym %i assigned to %i.\n", i, scancode);
         }
      }
   }
}



/* x_set_leds:
 *  Update the keyboard LEDs.
 */
static void x_set_leds(int leds)
{
   XKeyboardControl values;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();

   ASSERT(xkeyboard_installed);

   _al_mutex_lock(&system->lock);

   values.led = 1;
   values.led_mode = leds & ALLEGRO_KEYMOD_NUMLOCK ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(system->x11display, KBLed | KBLedMode, &values);

   values.led = 2;
   values.led_mode = leds & ALLEGRO_KEYMOD_CAPSLOCK ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(system->x11display, KBLed | KBLedMode, &values);

   values.led = 3;
   values.led_mode = leds & ALLEGRO_KEYMOD_SCROLLLOCK ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(system->x11display, KBLed | KBLedMode, &values);

   _al_mutex_unlock(&system->lock);
}



/* x_keyboard_init
 *  Initialise the X11 keyboard driver.
 */
static int x_keyboard_init(void)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XIM
   XIMStyles *xim_styles;
   XIMStyle xim_style = 0;
   char *imvalret;
   int i;
#endif
   ALLEGRO_SYSTEM_XGLX *s = (void *)al_system_driver();

   if (xkeyboard_installed)
      return 0;

   main_pid = getpid();

   memcpy(key_names, _al_keyboard_common_names, sizeof key_names);

   _al_mutex_lock(&s->lock);

#ifdef ALLEGRO_XWINDOWS_WITH_XIM
/* TODO: is this needed?
   if (setlocale(LC_ALL,"") == NULL) {
      TRACE(PREFIX_W "Could not set default locale.\n");
   }

   modifiers = XSetLocaleModifiers("@im=none");
   if (modifiers == NULL) {
      TRACE(PREFIX_W "XSetLocaleModifiers failed.\n");
   }
*/
   xim = XOpenIM (s->x11display, NULL, NULL, NULL);
   if (xim == NULL) {
      TRACE(PREFIX_W "XOpenIM failed.\n");
   }

   if (xim) {
      imvalret = XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL);
      if (imvalret != NULL || xim_styles == NULL) {
         TRACE(PREFIX_W "Input method doesn't support any styles.\n");
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
            TRACE (PREFIX_W "Input method doesn't support the style we support.\n");
         }
         XFree(xim_styles);
      }
   }

   if (xim && xim_style) {
      xic = XCreateIC(xim,
                       XNInputStyle, xim_style,
                       XNClientWindow, s->xwindow,
                       XNFocusWindow, s->xwindow,
                       NULL);

      if (xic == NULL) {
         TRACE (PREFIX_W "XCreateIC failed.\n");
      }
   }
#endif

   _al_xwin_get_keyboard_mapping();

   _al_mutex_unlock(&s->lock);

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
   
   ALLEGRO_SYSTEM_XGLX *s = (void *)al_system_driver();

   _al_mutex_lock(&s->lock);

#ifdef ALLEGRO_XWINDOWS_WITH_XIM

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
      XFree(keysyms);
      keysyms = NULL;
   }

   _al_mutex_unlock(&s->lock);
}



/*----------------------------------------------------------------------
 *
 * Supports for new API.  For now this code is kept separate from the
 * above to ease merging changes made in the trunk.
 *
 */


typedef struct ALLEGRO_KEYBOARD_XWIN
{
   ALLEGRO_KEYBOARD parent;
   ALLEGRO_KBDSTATE state;
} ALLEGRO_KEYBOARD_XWIN;



/* the one and only keyboard object */
static ALLEGRO_KEYBOARD_XWIN the_keyboard;



/* forward declarations */
static bool xkeybd_init_keyboard(void);
static void xkeybd_exit_keyboard(void);
static ALLEGRO_KEYBOARD *xkeybd_get_keyboard(void);
static bool xkeybd_set_keyboard_leds(int leds);
static AL_CONST char *xkeybd_keycode_to_name(int keycode);
static void xkeybd_get_keyboard_state(ALLEGRO_KBDSTATE *ret_state);



/* the driver vtable */
#define KEYDRV_XWIN  AL_ID('X','W','I','N')

static ALLEGRO_KEYBOARD_DRIVER keydrv_xwin =
{
   KEYDRV_XWIN,
   empty_string,
   empty_string,
   "X11 keyboard",
   xkeybd_init_keyboard,
   xkeybd_exit_keyboard,
   xkeybd_get_keyboard,
   xkeybd_set_keyboard_leds,
   xkeybd_keycode_to_name,
   xkeybd_get_keyboard_state
};



/* list the available drivers */
_DRIVER_INFO _al_xwin_keyboard_driver_list[] =
{
   {  KEYDRV_XWIN, &keydrv_xwin, TRUE  },
   {  0,           NULL,         0     }
};



/* xkeybd_init_keyboard:
 *  Initialise the driver.
 */
static bool xkeybd_init_keyboard(void)
{
   if (x_keyboard_init() != 0)
      return false;

   memset(&the_keyboard, 0, sizeof the_keyboard);

   _al_event_source_init(&the_keyboard.parent.es);

   //_xwin_keydrv_set_leds(_key_shifts);
   
   /* Get the pid, which we use for the three finger salute */
   main_pid = getpid();

   return true;
}



/* xkeybd_exit_keyboard:
 *  Shut down the keyboard driver.
 */
static void xkeybd_exit_keyboard(void)
{
   x_keyboard_exit();

   _al_event_source_free(&the_keyboard.parent.es);
}



/* xkeybd_get_keyboard:
 *  Returns the address of a ALLEGRO_KEYBOARD structure representing the keyboard.
 */
static ALLEGRO_KEYBOARD *xkeybd_get_keyboard(void)
{
   return (ALLEGRO_KEYBOARD *)&the_keyboard;
}



/* xkeybd_set_keyboard_leds:
 *  Updates the LED state.
 */
static bool xkeybd_set_keyboard_leds(int leds)
{
   x_set_leds(leds);
   return true;
}



/* xkeybd_keycode_to_name:
 *  Converts the given keycode to a description of the key.
 */
static AL_CONST char *xkeybd_keycode_to_name(int keycode)
{
   return x_scancode_to_name(keycode);
}



/* xkeybd_get_keyboard_state:
 *  Copy the current keyboard state into RET_STATE, with any necessary locking.
 */
static void xkeybd_get_keyboard_state(ALLEGRO_KBDSTATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.parent.es);
   {
      *ret_state = the_keyboard.state;
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}



/* handle_key_press: [bgman thread]
 *  Hook for the X event dispatcher to handle key presses.
 *  The caller must lock the X-display.
 */
static int last_press_code = -1;

static void handle_key_press(int mycode, int unichar, unsigned int modifiers,
    ALLEGRO_DISPLAY *display)
{
   bool is_repeat;
   ALLEGRO_EVENT *event;
   ALLEGRO_EVENT_TYPE type;

   is_repeat = (last_press_code == mycode);
   last_press_code = mycode;

   _al_event_source_lock(&the_keyboard.parent.es);
   {
      /* Update the key_down array.  */
      _AL_KBDSTATE_SET_KEY_DOWN(the_keyboard.state, mycode);

      /* Generate the press event if necessary. */
      type = is_repeat ? ALLEGRO_EVENT_KEY_REPEAT : ALLEGRO_EVENT_KEY_DOWN;
      if ((_al_event_source_needs_to_generate_event(&the_keyboard.parent.es)) &&
          (event = _al_event_source_get_unused_event(&the_keyboard.parent.es)))
      {
         event->keyboard.type = type;
         event->keyboard.timestamp = al_current_time();
         event->keyboard.display = display;
         event->keyboard.keycode   = mycode;
         event->keyboard.unichar   = unichar;
         event->keyboard.modifiers = modifiers;
         _al_event_source_emit_event(&the_keyboard.parent.es, event);
      }
   }
   _al_event_source_unlock(&the_keyboard.parent.es);

   /* Exit by Ctrl-Alt-End.  */
   if ((_al_three_finger_flag)
       && ((mycode == ALLEGRO_KEY_DELETE) || (mycode == ALLEGRO_KEY_END))
       && (modifiers & ALLEGRO_KEYMOD_CTRL)
       && (modifiers & (ALLEGRO_KEYMOD_ALT | ALLEGRO_KEYMOD_ALTGR)))
   {
      TRACE(PREFIX_W "Three finger combo detected. SIGTERMing "
            "pid %d\n", main_pid);
      kill(main_pid, SIGTERM);
   }
}



/* handle_key_release: [bgman thread]
 *  Hook for the X event dispatcher to handle key releases.
 *  The caller must lock the X-display.
 */
static void handle_key_release(int mycode, ALLEGRO_DISPLAY *display)
{
   ALLEGRO_EVENT *event;

   if (last_press_code == mycode)
      last_press_code = -1;

   _al_event_source_lock(&the_keyboard.parent.es);
   {
      /* Update the key_down array.  */
      _AL_KBDSTATE_CLEAR_KEY_DOWN(the_keyboard.state, mycode);

      /* Generate the release event if necessary. */
      if ((_al_event_source_needs_to_generate_event(&the_keyboard.parent.es)) &&
          (event = _al_event_source_get_unused_event(&the_keyboard.parent.es)))
      {
         event->keyboard.type = ALLEGRO_EVENT_KEY_UP;
         event->keyboard.timestamp = al_current_time();
         event->keyboard.display = display;
         event->keyboard.keycode = mycode;
         event->keyboard.unichar = 0;
         event->keyboard.modifiers = 0;
         _al_event_source_emit_event(&the_keyboard.parent.es, event);
      }
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}
