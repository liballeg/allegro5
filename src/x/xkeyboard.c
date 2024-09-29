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

#define A5O_NO_COMPATIBILITY

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
#include <X11/XKBlib.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xkeyboard.h"
#include "allegro5/internal/aintern_xsystem.h"

#ifdef A5O_RASPBERRYPI
#include "allegro5/internal/aintern_raspberrypi.h"
#define A5O_SYSTEM_XGLX A5O_SYSTEM_RASPBERRYPI
#endif

A5O_DEBUG_CHANNEL("keyboard")

/*----------------------------------------------------------------------*/
static void handle_key_press(int mycode, int unichar, int filtered,
   unsigned int modifiers, A5O_DISPLAY *display);
static void handle_key_release(int mycode, unsigned int modifiers, A5O_DISPLAY *display);
static int _key_shifts;
/*----------------------------------------------------------------------*/


typedef struct A5O_KEYBOARD_XWIN
{
   A5O_KEYBOARD parent;
   A5O_KEYBOARD_STATE state;
   // Quit if Ctrl-Alt-Del is pressed.
   bool three_finger_flag;
} A5O_KEYBOARD_XWIN;


typedef struct A5O_KEY_REPEAT_DATA {
   XKeyEvent *event;
   bool found;
} A5O_KEY_REPEAT_DATA;


/* the one and only keyboard object */
static A5O_KEYBOARD_XWIN the_keyboard;

static int last_press_code = -1;

#ifdef A5O_XWINDOWS_WITH_XIM
static XIM xim = NULL;
static XIC xic = NULL;
#endif
static XModifierKeymap *xmodmap = NULL;
static int xkeyboard_installed = 0;
static int used[A5O_KEY_MAX];
static int sym_per_key;
static int min_keycode, max_keycode;
static KeySym *keysyms = NULL;
static int main_pid; /* The pid to kill with ctrl-alt-del. */
static int pause_key = 0; /* Allegro's special pause key state. */
static int keycode_to_scancode[256];

/* This table can be ammended to provide more reasonable defaults for
 * mappings other than US/UK. They are used to map X11 KeySyms as found in
 * X11/keysym.h to Allegro's A5O_KEY_* codes. This will only work well on US/UK
 * keyboards since Allegro simply doesn't have A5O_KEY_* codes for non US/UK
 * keyboards. So with other mappings, the unmapped keys will be distributed
 * arbitrarily to the remaining A5O_KEY_* codes.
 *
 * Double mappings should be avoided, else they can lead to different keys
 * producing the same A5O_KEY_* code on some mappings.
 *
 * In cases where there is no other way to detect a key, since we have no
 * ASCII applied to it, like A5O_KEY_LEFT or A5O_KEY_PAUSE, multiple mappings should
 * be ok though. This table will never be able to be 100% perfect, so just
 * try to make it work for as many as possible, using additional hacks in
 * some cases. There is also still the possibility to override keys with
 * the [xkeyboard] config section, so users can always re-map keys. (E.g.
 * to play an Allegro game which hard-coded A5O_KEY_Y and A5O_KEY_X for left and
 * right.)
 */
static struct {
   KeySym keysym;
   int allegro_key;
}
translation_table[] = {
   {XK_a, A5O_KEY_A},
   {XK_b, A5O_KEY_B},
   {XK_c, A5O_KEY_C},
   {XK_d, A5O_KEY_D},
   {XK_e, A5O_KEY_E},
   {XK_f, A5O_KEY_F},
   {XK_g, A5O_KEY_G},
   {XK_h, A5O_KEY_H},
   {XK_i, A5O_KEY_I},
   {XK_j, A5O_KEY_J},
   {XK_k, A5O_KEY_K},
   {XK_l, A5O_KEY_L},
   {XK_m, A5O_KEY_M},
   {XK_n, A5O_KEY_N},
   {XK_o, A5O_KEY_O},
   {XK_p, A5O_KEY_P},
   {XK_q, A5O_KEY_Q},
   {XK_r, A5O_KEY_R},
   {XK_s, A5O_KEY_S},
   {XK_t, A5O_KEY_T},
   {XK_u, A5O_KEY_U},
   {XK_v, A5O_KEY_V},
   {XK_w, A5O_KEY_W},
   {XK_x, A5O_KEY_X},
   {XK_y, A5O_KEY_Y},
   {XK_z, A5O_KEY_Z},
   {XK_0, A5O_KEY_0},
   {XK_1, A5O_KEY_1},
   {XK_2, A5O_KEY_2},
   {XK_3, A5O_KEY_3},
   {XK_4, A5O_KEY_4},
   {XK_5, A5O_KEY_5},
   {XK_6, A5O_KEY_6},
   {XK_7, A5O_KEY_7},
   {XK_8, A5O_KEY_8},
   {XK_9, A5O_KEY_9},

   /* Double mappings for numeric keyboard.
    * If an X server actually uses both at the same time, Allegro will
    * detect them as the same. But normally, an X server just reports it as
    * either of them, and therefore we always get the keys as A5O_KEY_PAD_*.
    */
   {XK_KP_0, A5O_KEY_PAD_0},
   {XK_KP_Insert, A5O_KEY_PAD_0},
   {XK_KP_1, A5O_KEY_PAD_1},
   {XK_KP_End, A5O_KEY_PAD_1},
   {XK_KP_2, A5O_KEY_PAD_2},
   {XK_KP_Down, A5O_KEY_PAD_2},
   {XK_KP_3, A5O_KEY_PAD_3},
   {XK_KP_Page_Down, A5O_KEY_PAD_3},
   {XK_KP_4, A5O_KEY_PAD_4},
   {XK_KP_Left, A5O_KEY_PAD_4},
   {XK_KP_5, A5O_KEY_PAD_5},
   {XK_KP_Begin, A5O_KEY_PAD_5},
   {XK_KP_6, A5O_KEY_PAD_6},
   {XK_KP_Right, A5O_KEY_PAD_6},
   {XK_KP_7, A5O_KEY_PAD_7},
   {XK_KP_Home, A5O_KEY_PAD_7},
   {XK_KP_8, A5O_KEY_PAD_8},
   {XK_KP_Up, A5O_KEY_PAD_8},
   {XK_KP_9, A5O_KEY_PAD_9},
   {XK_KP_Page_Up, A5O_KEY_PAD_9},
   {XK_KP_Delete, A5O_KEY_PAD_DELETE},
   {XK_KP_Decimal, A5O_KEY_PAD_DELETE},

   /* Double mapping!
    * Same as above - but normally, the X server just reports one or the
    * other for the Pause key, and the other is not reported for any key.
    */
   {XK_Pause, A5O_KEY_PAUSE},
   {XK_Break, A5O_KEY_PAUSE},

   {XK_F1, A5O_KEY_F1},
   {XK_F2, A5O_KEY_F2},
   {XK_F3, A5O_KEY_F3},
   {XK_F4, A5O_KEY_F4},
   {XK_F5, A5O_KEY_F5},
   {XK_F6, A5O_KEY_F6},
   {XK_F7, A5O_KEY_F7},
   {XK_F8, A5O_KEY_F8},
   {XK_F9, A5O_KEY_F9},
   {XK_F10, A5O_KEY_F10},
   {XK_F11, A5O_KEY_F11},
   {XK_F12, A5O_KEY_F12},
   {XK_Escape, A5O_KEY_ESCAPE},
   {XK_grave, A5O_KEY_TILDE}, /* US left of 1 */
   {XK_minus, A5O_KEY_MINUS}, /* US right of 0 */
   {XK_equal, A5O_KEY_EQUALS}, /* US 2 right of 0 */
   {XK_BackSpace, A5O_KEY_BACKSPACE},
   {XK_Tab, A5O_KEY_TAB},
   {XK_bracketleft, A5O_KEY_OPENBRACE}, /* US right of P */
   {XK_bracketright, A5O_KEY_CLOSEBRACE}, /* US 2 right of P */
   {XK_Return, A5O_KEY_ENTER},
   {XK_semicolon, A5O_KEY_SEMICOLON}, /* US right of L */
   {XK_apostrophe, A5O_KEY_QUOTE}, /* US 2 right of L */
   {XK_backslash, A5O_KEY_BACKSLASH}, /* US 3 right of L */
   {XK_less, A5O_KEY_BACKSLASH2}, /* US left of Y */
   {XK_comma, A5O_KEY_COMMA}, /* US right of M */
   {XK_period, A5O_KEY_FULLSTOP}, /* US 2 right of M */
   {XK_slash, A5O_KEY_SLASH}, /* US 3 right of M */
   {XK_space, A5O_KEY_SPACE},
   {XK_Insert, A5O_KEY_INSERT},
   {XK_Delete, A5O_KEY_DELETE},
   {XK_Home, A5O_KEY_HOME},
   {XK_End, A5O_KEY_END},
   {XK_Page_Up, A5O_KEY_PGUP},
   {XK_Page_Down, A5O_KEY_PGDN},
   {XK_Left, A5O_KEY_LEFT},
   {XK_Right, A5O_KEY_RIGHT},
   {XK_Up, A5O_KEY_UP},
   {XK_Down, A5O_KEY_DOWN},
   {XK_KP_Divide, A5O_KEY_PAD_SLASH},
   {XK_KP_Multiply, A5O_KEY_PAD_ASTERISK},
   {XK_KP_Subtract, A5O_KEY_PAD_MINUS},
   {XK_KP_Add, A5O_KEY_PAD_PLUS},
   {XK_KP_Enter, A5O_KEY_PAD_ENTER},
   {XK_Print, A5O_KEY_PRINTSCREEN},

   //{, A5O_KEY_ABNT_C1},
   //{, A5O_KEY_YEN},
   //{, A5O_KEY_KANA},
   //{, A5O_KEY_CONVERT},
   //{, A5O_KEY_NOCONVERT},
   //{, A5O_KEY_AT},
   //{, A5O_KEY_CIRCUMFLEX},
   //{, A5O_KEY_COLON2},
   //{, A5O_KEY_KANJI},
   {XK_KP_Equal, A5O_KEY_PAD_EQUALS},  /* MacOS X */
   //{, A5O_KEY_BACKQUOTE},  /* MacOS X */
   //{, A5O_KEY_SEMICOLON},  /* MacOS X */
   //{, A5O_KEY_COMMAND},  /* MacOS X */

   {XK_Shift_L, A5O_KEY_LSHIFT},
   {XK_Shift_R, A5O_KEY_RSHIFT},
   {XK_Control_L, A5O_KEY_LCTRL},
   {XK_Control_R, A5O_KEY_RCTRL},
   {XK_Alt_L, A5O_KEY_ALT},

   /* Double mappings. This is a bit of a problem, since you can configure
    * X11 differently to what report for those keys.
    */
   {XK_Alt_R, A5O_KEY_ALTGR},
   {XK_ISO_Level3_Shift, A5O_KEY_ALTGR},
   {XK_Meta_L, A5O_KEY_LWIN},
   {XK_Super_L, A5O_KEY_LWIN},
   {XK_Meta_R, A5O_KEY_RWIN},
   {XK_Super_R, A5O_KEY_RWIN},

   {XK_Menu, A5O_KEY_MENU},
   {XK_Scroll_Lock, A5O_KEY_SCROLLLOCK},
   {XK_Num_Lock, A5O_KEY_NUMLOCK},
   {XK_Caps_Lock, A5O_KEY_CAPSLOCK}
};

/* Table of: Allegro's modifier flag, associated X11 flag, toggle method. */
static int modifier_flags[8][3] = {
   {A5O_KEYMOD_SHIFT, ShiftMask, 0},
   {A5O_KEYMOD_CAPSLOCK, LockMask, 1},
   {A5O_KEYMOD_CTRL, ControlMask, 0},
   {A5O_KEYMOD_ALT, Mod1Mask, 0},
   {A5O_KEYMOD_NUMLOCK, Mod2Mask, 1},
   {A5O_KEYMOD_SCROLLLOCK, Mod3Mask, 1},
   {A5O_KEYMOD_LWIN | A5O_KEYMOD_RWIN, Mod4Mask, 0}, /* Should we use only one? */
   {A5O_KEYMOD_ALTGR, Mod5Mask, 0} /* AltGr */
};

/* Table of key names. */
static char const *key_names[A5O_KEY_MAX];



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



/* find_unknown_key_assignment
 *  In some cases, X11 doesn't report any KeySym for a key - so the earliest
 *  time we can map it to an Allegro key is when it is first pressed.
 */
static int find_unknown_key_assignment(int i)
{
   int j;

   for (j = 1; j < A5O_KEY_MAX; j++) {
      if (!used[j]) {
         const char *str;
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

   if (j == A5O_KEY_MAX) {
      A5O_ERROR("You have more keys reported by X than Allegro's "
             "maximum of %i keys. Please send a bug report.\n", A5O_KEY_MAX);
      keycode_to_scancode[i] = 0;
   }

   char str[1024];
   sprintf(str, "Key %i missing:", i);
   for (j = 0; j < sym_per_key; j++) {
      char *sym_str = XKeysymToString(keysyms[sym_per_key * (i - min_keycode) + j]);
      sprintf(str + strlen(str), " %s", sym_str ? sym_str : "NULL");
   }
   A5O_DEBUG("%s assigned to %i.\n", str, keycode_to_scancode[i]);

   return keycode_to_scancode[i];
}


/* XCheckIfAny predicate that checks if this event may be a key repeat event */
static Bool check_for_repeat(Display *display, XEvent *event, XPointer arg)
{
   A5O_KEY_REPEAT_DATA *d = (A5O_KEY_REPEAT_DATA *)arg;

   (void)display;

   if (event->type == KeyPress &&
      event->xkey.keycode == d->event->keycode &&
      (event->xkey.time - d->event->time) < 4) {
      d->found = true;
   }

   return False;
}


/* _al_xwin_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void _al_xwin_keyboard_handler(XKeyEvent *event, A5O_DISPLAY *display)
{
   int keycode;

   if (!xkeyboard_installed)
      return;

   keycode = keycode_to_scancode[event->keycode];
   if (keycode == -1)
      keycode = find_unknown_key_assignment(event->keycode);

   update_shifts(event);

   /* Special case the pause key. */
   if (keycode == A5O_KEY_PAUSE) {
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
      int unicode = 0;
      int filtered = 0;

#if defined (A5O_XWINDOWS_WITH_XIM) && defined(X_HAVE_UTF8_STRING)
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
      A5O_USTR_INFO info;
      const A5O_USTR *ustr = al_ref_cstr(&info, buffer);
      unicode = al_ustr_get(ustr, 0);
      if (unicode < 0)
         unicode = 0;

#ifdef A5O_XWINDOWS_WITH_XIM
      A5O_DISPLAY_XGLX *glx = (void *)display;
      filtered = XFilterEvent((XEvent *)event, glx->window);
#endif
      if (keycode || unicode) {
         handle_key_press(keycode, unicode, filtered, _key_shifts, display);
      }
   }
   else { /* Key release. */
     /* HACK:
      * Detect key repeat by looking forward to see if this release is
      * followed by a press event, in which case we assume that this release
      * event was generated in response to that key press event. Events are
      * simultaneous if they are separated by less than 4 ms (a value that
      * worked well on one machine where this hack was needed).
      *
      * This is unnecessary on systems where XkbSetDetectableAutorepeat works.
      */
      if (XPending(event->display) > 0) {
         A5O_KEY_REPEAT_DATA d;
         XEvent dummy;
         d.event = event;
         d.found = false;
         XCheckIfEvent(event->display, &dummy, check_for_repeat, (XPointer)&d);
         if (d.found) {
            return;
         }
      }
      handle_key_release(keycode, _key_shifts, display);
   }
}



/* _al_xwin_keyboard_switch_handler:
 *  Handle a focus switch event.
 */
void _al_xwin_keyboard_switch_handler(A5O_DISPLAY *display, bool focus_in)
{
   _al_event_source_lock(&the_keyboard.parent.es);

   if (focus_in) {
      the_keyboard.state.display = display;
#ifdef A5O_XWINDOWS_WITH_XIM
      if (xic) {
         A5O_DISPLAY_XGLX *display_glx = (void *)display;
         XSetICValues(xic, XNClientWindow, display_glx->window, NULL);
      }
#endif
   }
   else {
      the_keyboard.state.display = NULL;
   }

   _al_event_source_unlock(&the_keyboard.parent.es);
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
static const char *x_scancode_to_name(int scancode)
{
   ASSERT (scancode >= 0 && scancode < A5O_KEY_MAX);

   return key_names[scancode];
}



/* _al_xwin_get_keyboard_mapping:
 *  Generate a mapping from X11 keycodes to Allegro A5O_KEY_* codes. We have
 *  two goals: Every keypress should be mapped to a distinct Allegro A5O_KEY_*
 *  code. And we want the A5O_KEY_* codes to match the pressed
 *  key to some extent. To do the latter, the X11 KeySyms produced by a key
 *  are examined. If a match is found in the table above, the mapping is
 *  added to the mapping table. If no known KeySym is found for a key (or
 *  the same KeySym is found for more keys) - the remaining keys are
 *  distributed arbitrarily to the remaining A5O_KEY_* codes.
 *
 *  In a future version, this could be simplified by mapping *all* the X11
 *  KeySyms to A5O_KEY_* codes.
 */

static bool _al_xwin_get_keyboard_mapping(void)
{
   int i;
   int count;
   int missing = 0;

   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   memset(used, 0, sizeof used);
   memset(keycode_to_scancode, 0, sizeof keycode_to_scancode);

   XDisplayKeycodes(system->x11display, &min_keycode, &max_keycode);
   count = 1 + max_keycode - min_keycode;

   if (keysyms) {
      XFree(keysyms);
   }
   keysyms = XGetKeyboardMapping(system->x11display, min_keycode,
      count, &sym_per_key);

   A5O_INFO("%i keys, %i symbols per key.\n", count, sym_per_key);

   if (sym_per_key <= 0) {
      return false;
   }

   missing = 0;

   for (i = min_keycode; i <= max_keycode; i++) {
      KeySym sym = keysyms[sym_per_key * (i - min_keycode)];
      KeySym sym2 =  keysyms[sym_per_key * (i - min_keycode) + 1];
      char *sym_str, *sym2_str;
      int allegro_key = 0;
      char str[1024];

      sym_str = XKeysymToString(sym);
      sym2_str = XKeysymToString(sym2);

      snprintf(str, sizeof str, "key [%i: %s %s]", i,
         sym_str ? sym_str : "NULL", sym2_str ? sym2_str : "NULL");

      /* Hack for French keyboards, to correctly map A5O_KEY_0 to A5O_KEY_9. */
      if (sym2 >= XK_0 && sym2 <= XK_9) {
         allegro_key = find_allegro_key(sym2);
      }

      if (!allegro_key) {
         if (sym != NoSymbol) {
            allegro_key = find_allegro_key(sym);

            if (allegro_key == 0) {
               missing++;
               A5O_DEBUG("%s defering.\n", str);
            }
         }
         else {
            /* No KeySym for this key - ignore it. */
            keycode_to_scancode[i] = -1;
            A5O_DEBUG("%s not assigned.\n", str);
         }
      }

      if (allegro_key) {
         bool is_double = false;
         if (used[allegro_key]) {
            is_double = true;
         }
         keycode_to_scancode[i] = allegro_key;
         key_names[allegro_key] =
            XKeysymToString(keysyms[sym_per_key * (i - min_keycode)]);
         used[allegro_key] = 1;
         A5O_DEBUG("%s%s assigned to %i.\n", str,
            is_double ? " *double*" : "", allegro_key);
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
      char str[1024];
      sprintf(str, "Modifier %d:", i + 1);
      for (j = 0; j < xmodmap->max_keypermod; j++) {
         KeyCode keycode = xmodmap->modifiermap[i * xmodmap->max_keypermod + j];
         // XKeycodeToKeysym is deprecated
         //KeySym sym = XKeycodeToKeysym(system->x11display, keycode, 0);
         KeySym sym = XkbKeycodeToKeysym(system->x11display, keycode, 0, 0);

         char *sym_str = XKeysymToString(sym);
         sprintf(str + strlen(str), " %s", sym_str ? sym_str : "NULL");
      }
      A5O_DEBUG("%s\n", str);
   }

   /* The [xkeymap] section can be useful, e.g. if trying to play a
    * game which has X and Y hardcoded as A5O_KEY_X and A5O_KEY_Y to mean
    * left/right movement, but on the X11 keyboard X and Y are far apart.
    * For normal use, a user never should have to touch [xkeymap] anymore
    * though, and proper written programs will not hardcode such mappings.
    */
   A5O_CONFIG *c = al_get_system_config();

   char const *key;
   A5O_CONFIG_ENTRY *it;
   key = al_get_first_config_entry(c, "xkeymap", &it);
   while (key) {
      char const *val;
      val = al_get_config_value(c, "xkeymap", key);
      int keycode = strtol(key, NULL, 10);
      int scancode = strtol(val, NULL, 10);
      if (keycode > 0 && scancode > 0) {
         keycode_to_scancode[keycode] = scancode;
         A5O_WARN("User override: KeySym %i assigned to %i.\n",
            keycode, scancode);
      }
      key = al_get_next_config_entry(&it);
   }

   return true;
}



/* x_set_leds:
 *  Update the keyboard LEDs.
 */
static void x_set_leds(int leds)
{
   XKeyboardControl values;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   ASSERT(xkeyboard_installed);

   _al_mutex_lock(&system->lock);

   values.led = 1;
   values.led_mode = leds & A5O_KEYMOD_NUMLOCK ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(system->x11display, KBLed | KBLedMode, &values);

   values.led = 2;
   values.led_mode = leds & A5O_KEYMOD_CAPSLOCK ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(system->x11display, KBLed | KBLedMode, &values);

   values.led = 3;
   values.led_mode = leds & A5O_KEYMOD_SCROLLLOCK ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(system->x11display, KBLed | KBLedMode, &values);

   _al_mutex_unlock(&system->lock);
}



/* x_keyboard_init
 *  Initialise the X11 keyboard driver.
 */
static int x_keyboard_init(void)
{
#ifdef A5O_XWINDOWS_WITH_XIM
   char *old_locale;
   XIMStyles *xim_styles;
   XIMStyle xim_style = 0;
   char *imvalret;
   int i;
#endif
   A5O_SYSTEM_XGLX *s = (void *)al_get_system_driver();

   if (xkeyboard_installed)
      return 0;

   if (s->x11display == NULL)
      return 0;

   main_pid = getpid();

   memcpy(key_names, _al_keyboard_common_names, sizeof key_names);

   _al_mutex_lock(&s->lock);

   /* HACK: XkbSetDetectableAutoRepeat is broken in some versions of X.Org */
   Bool supported;
   XkbSetDetectableAutoRepeat(s->x11display, True, &supported);
   if (!supported) {
      A5O_WARN("XkbSetDetectableAutoRepeat failed.\n");
   }

#ifdef A5O_XWINDOWS_WITH_XIM
   A5O_INFO("Using X Input Method.\n");

   old_locale = setlocale(LC_CTYPE, NULL);
   A5O_DEBUG("Old locale: %s\n", old_locale ? old_locale : "(null)");
   if (old_locale) {
      /* The return value of setlocale() may be clobbered by the next call
       * to setlocale() so we must copy it.
       */
      old_locale = strdup(old_locale);
   }

   /* Otherwise we are restricted to ISO-8859-1 characters. */
   if (setlocale(LC_CTYPE, "") == NULL) {
      A5O_WARN("Could not set default locale.\n");
   }

   /* By default never use an input method as we are not prepared to
    * handle any of them. This is still enough to get composed keys.
    */
   char const *modifiers = XSetLocaleModifiers("@im=none");
   if (modifiers == NULL) {
      A5O_WARN("XSetLocaleModifiers failed.\n");
   }

   xim = XOpenIM(s->x11display, NULL, NULL, NULL);
   if (xim == NULL) {
      A5O_WARN("XOpenIM failed.\n");
   }

   if (old_locale) {
      A5O_DEBUG("Restoring old locale: %s\n", old_locale);
      setlocale(LC_CTYPE, old_locale);
      free(old_locale);
   }

   if (xim) {
      imvalret = XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL);
      if (imvalret != NULL || xim_styles == NULL) {
         A5O_WARN("Input method doesn't support any styles.\n");
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
            A5O_WARN("Input method doesn't support the style we support.\n");
         }
         else {
            A5O_INFO("Input method style = %ld\n", xim_style);
         }
         XFree(xim_styles);
      }
   }

   if (xim && xim_style) {
      xic = XCreateIC(xim,
         XNInputStyle, xim_style,
         NULL);
      if (xic == NULL) {
         A5O_WARN("XCreateIC failed.\n");
      }
      else {
         A5O_INFO("XCreateIC succeeded.\n");
      }

      /* In case al_install_keyboard() is called when there already is
       * a display, we set it as client window.
       */
      A5O_DISPLAY *display = al_get_current_display();
      A5O_DISPLAY_XGLX *display_glx = (void *)display;
      if (display_glx && xic)
         XSetICValues(xic, XNClientWindow, display_glx->window, NULL);
   }
#endif

   if (!_al_xwin_get_keyboard_mapping()) {
      return 1;
   }

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

   A5O_SYSTEM_XGLX *s = (void *)al_get_system_driver();

   _al_mutex_lock(&s->lock);

#ifdef A5O_XWINDOWS_WITH_XIM

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


/* forward declarations */
static bool xkeybd_init_keyboard(void);
static void xkeybd_exit_keyboard(void);
static A5O_KEYBOARD *xkeybd_get_keyboard(void);
static bool xkeybd_set_keyboard_leds(int leds);
static const char *xkeybd_keycode_to_name(int keycode);
static void xkeybd_get_keyboard_state(A5O_KEYBOARD_STATE *ret_state);
static void xkeybd_clear_keyboard_state(void);



/* the driver vtable */
#define KEYDRV_XWIN  AL_ID('X','W','I','N')

static A5O_KEYBOARD_DRIVER keydrv_xwin =
{
   KEYDRV_XWIN,
   "",
   "",
   "X11 keyboard",
   xkeybd_init_keyboard,
   xkeybd_exit_keyboard,
   xkeybd_get_keyboard,
   xkeybd_set_keyboard_leds,
   xkeybd_keycode_to_name,
   xkeybd_get_keyboard_state,
   xkeybd_clear_keyboard_state
};



A5O_KEYBOARD_DRIVER *_al_xwin_keyboard_driver(void)
{
   return &keydrv_xwin;
}



/* xkeybd_init_keyboard:
 *  Initialise the driver.
 */
static bool xkeybd_init_keyboard(void)
{
   if (x_keyboard_init() != 0)
      return false;

   memset(&the_keyboard, 0, sizeof the_keyboard);

   _al_event_source_init(&the_keyboard.parent.es);

   the_keyboard.three_finger_flag = true;

   const char *value = al_get_config_value(al_get_system_config(),
         "keyboard", "enable_three_finger_exit");
   if (value) {
      the_keyboard.three_finger_flag = !strncmp(value, "true", 4);
   }
   A5O_DEBUG("Three finger flag enabled: %s\n",
      the_keyboard.three_finger_flag ? "true" : "false");

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
 *  Returns the address of a A5O_KEYBOARD structure representing the keyboard.
 */
static A5O_KEYBOARD *xkeybd_get_keyboard(void)
{
   return (A5O_KEYBOARD *)&the_keyboard;
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
static const char *xkeybd_keycode_to_name(int keycode)
{
   return x_scancode_to_name(keycode);
}



/* xkeybd_get_keyboard_state:
 *  Copy the current keyboard state into RET_STATE, with any necessary locking.
 */
static void xkeybd_get_keyboard_state(A5O_KEYBOARD_STATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.parent.es);
   {
      *ret_state = the_keyboard.state;
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}



/* xkeybd_get_keyboard_state:
 *  Clear the current keyboard state, with any necessary locking.
 */
static void xkeybd_clear_keyboard_state(void)
{
   _al_event_source_lock(&the_keyboard.parent.es);
   {
      last_press_code = -1;
      memset(&the_keyboard.state, 0, sizeof(the_keyboard.state));
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}



/* handle_key_press: [bgman thread]
 *  Hook for the X event dispatcher to handle key presses.
 *  The caller must lock the X-display.
 */
static void handle_key_press(int mycode, int unichar, int filtered,
   unsigned int modifiers, A5O_DISPLAY *display)
{
   bool is_repeat;

   is_repeat = (last_press_code == mycode);
   if (mycode > 0)
      last_press_code = mycode;

   _al_event_source_lock(&the_keyboard.parent.es);
   {
      /* Update the key_down array.  */
      _AL_KEYBOARD_STATE_SET_KEY_DOWN(the_keyboard.state, mycode);

      /* Generate the events if necessary. */
      if (_al_event_source_needs_to_generate_event(&the_keyboard.parent.es)) {
         A5O_EVENT event;

         event.keyboard.type = A5O_EVENT_KEY_DOWN;
         event.keyboard.timestamp = al_get_time();
         event.keyboard.display = display;
         event.keyboard.keycode = last_press_code;
         event.keyboard.unichar = 0;
         event.keyboard.modifiers = modifiers;
         event.keyboard.repeat = false;

         /* Don't send KEY_DOWN for non-physical key events. */
         if (mycode > 0 && !is_repeat) {
            _al_event_source_emit_event(&the_keyboard.parent.es, &event);
         }

         /* Don't send KEY_CHAR for events filtered by an input method,
          * nor modifier keys.
          */
         if (!filtered && mycode < A5O_KEY_MODIFIERS) {
            event.keyboard.type = A5O_EVENT_KEY_CHAR;
            event.keyboard.unichar = unichar;
            event.keyboard.modifiers = modifiers;
            event.keyboard.repeat = is_repeat;
            _al_event_source_emit_event(&the_keyboard.parent.es, &event);
         }
      }
   }
   _al_event_source_unlock(&the_keyboard.parent.es);

// FIXME?
#ifndef A5O_RASPBERRYPI
   /* Toggle mouse grab key.  The system driver should not be locked here. */
   if (last_press_code && !is_repeat) {
      A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
      if (system->toggle_mouse_grab_keycode &&
          system->toggle_mouse_grab_keycode == mycode &&
          (modifiers & system->toggle_mouse_grab_modifiers)
            == system->toggle_mouse_grab_modifiers)
      {
         if (system->mouse_grab_display == display)
            al_ungrab_mouse();
         else
            al_grab_mouse(display);
      }
   }
#endif

   /* Exit by Ctrl-Alt-End.  */
   if ((the_keyboard.three_finger_flag)
       && ((mycode == A5O_KEY_DELETE) || (mycode == A5O_KEY_END))
       && (modifiers & A5O_KEYMOD_CTRL)
       && (modifiers & (A5O_KEYMOD_ALT | A5O_KEYMOD_ALTGR)))
   {
      A5O_WARN("Three finger combo detected. SIGTERMing "
         "pid %d\n", main_pid);
      kill(main_pid, SIGTERM);
   }
}



/* handle_key_release: [bgman thread]
 *  Hook for the X event dispatcher to handle key releases.
 *  The caller must lock the X-display.
 */
static void handle_key_release(int mycode, unsigned int modifiers, A5O_DISPLAY *display)
{
   if (last_press_code == mycode)
      last_press_code = -1;

   _al_event_source_lock(&the_keyboard.parent.es);
   {
      /* Update the key_down array.  */
      _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(the_keyboard.state, mycode);

      /* Generate the release event if necessary. */
      if (_al_event_source_needs_to_generate_event(&the_keyboard.parent.es)) {
         A5O_EVENT event;
         event.keyboard.type = A5O_EVENT_KEY_UP;
         event.keyboard.timestamp = al_get_time();
         event.keyboard.display = display;
         event.keyboard.keycode = mycode;
         event.keyboard.unichar = 0;
         event.keyboard.modifiers = modifiers;
         _al_event_source_emit_event(&the_keyboard.parent.es, &event);
      }
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}


/* vim: set sts=3 sw=3 et: */
