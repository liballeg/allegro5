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
 *      X Window keyboard driver.
 *
 *      By Michael Bukin.
 *
 *      Modified for the new keyboard API by Peter Wang.
 *
 *      See readme.txt for copyright information.
 *
 *      TODO: move all the keyboard-related stuff into this file and
 *      rearchitect the X-event loop.
 *
 *      TODO: non-Latin1 character input
 */


#define ALLEGRO_NO_COMPATIBILITY

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern2.h"
#include "xwin.h"



typedef struct AL_KEYBOARD_XWIN
{
   AL_KEYBOARD parent;
   AL_KBDSTATE state;
} AL_KEYBOARD_XWIN;



/* the one and only keyboard object */
static AL_KEYBOARD_XWIN the_keyboard;

/* the pid to kill when three finger saluting */
static pid_t main_pid;


/* forward declarations */
static bool xkeybd_init(void);
static void xkeybd_exit(void);
static AL_KEYBOARD *xkeybd_get_keyboard(void);
static bool xkeybd_set_leds(int leds);
static void xkeybd_get_state(AL_KBDSTATE *ret_state);

/* for hooking onto xwin.c event loop */
static void handle_key_press(XKeyEvent *ke, bool state_field_reliable);
static void handle_key_release(XKeyEvent *ke, bool state_field_reliable);
static void handle_focus_change(bool focused);



/* the driver vtable */
#define KEYDRV_XWIN  AL_ID('X','W','I','N')

static AL_KEYBOARD_DRIVER keydrv_xwin =
{
   KEYDRV_XWIN,
   empty_string,
   empty_string,
   "X-Windows keyboard",
   xkeybd_init,
   xkeybd_exit,
   xkeybd_get_keyboard,
   xkeybd_set_leds,
   xkeybd_get_state
};



/* list the available drivers */
_DRIVER_INFO _al_xwin_keyboard_driver_list[] =
{
   {  KEYDRV_XWIN, &keydrv_xwin, TRUE  },
   {  0,           NULL,         0     }
};



/* mapping of X keycodes to AL_KEY_* codes, initialised by
 * init_keyboard_tables()
 */
#define XKEYCODE_TO_MYCODE_SIZE 256
static unsigned char xkeycode_to_mycode[XKEYCODE_TO_MYCODE_SIZE];
static void init_keyboard_tables(void);




/* xkeybd_init:
 *  Initialise the driver.
 */
static bool xkeybd_init(void)
{
   XLOCK();
   init_keyboard_tables();
   XUNLOCK();

   memset(&the_keyboard, 0, sizeof the_keyboard);

   _al_event_source_init(&the_keyboard.parent.es, _AL_ALL_KEYBOARD_EVENTS, sizeof(AL_KEYBOARD_EVENT));

   //_xwin_keydrv_set_leds(_key_shifts);

   XLOCK();
   _al_xwin_key_press_handler = handle_key_press;
   _al_xwin_key_release_handler = handle_key_release;
   _al_xwin_focus_change_handler = handle_focus_change;
   XUNLOCK();
   
   /* Get the pid, which we use for the three finger salute */
   main_pid = getpid();

   return true;
}



/* xkeybd_exit:
 *  Shut down the keyboard driver.
 */
static void xkeybd_exit(void)
{
   XLOCK();
   {
      _al_xwin_key_press_handler = 0;
      _al_xwin_key_release_handler = 0;
      _al_xwin_focus_change_handler = 0;      
   }
   XUNLOCK();

   _al_event_source_free(&the_keyboard.parent.es);
}



/* xkeybd_get_keyboard:
 *  Returns the address of a AL_KEYBOARD structure representing the keyboard.
 */
static AL_KEYBOARD *xkeybd_get_keyboard(void)
{
   return (AL_KEYBOARD *)&the_keyboard;
}



/* xkeybd_set_leds:
 *  Updates the LED state.
 */
static bool xkeybd_set_leds(int leds)
{
   _xwin_change_keyboard_control(1, leds & AL_KEYMOD_NUMLOCK);
   _xwin_change_keyboard_control(2, leds & AL_KEYMOD_CAPSLOCK);
   _xwin_change_keyboard_control(3, leds & AL_KEYMOD_SCROLLLOCK);
   return true;
}



/* xkeybd_get_state:
 *  Copy the current keyboard state into RET_STATE, with any necessary locking.
 */
static void xkeybd_get_state(AL_KBDSTATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.parent.es);
   {
      *ret_state = the_keyboard.state;
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}



/* get_latin1_char: [bgman thread]
 *  Helper to return the latin-1 character corresponding to the
 *  XKeyEvent given, or 0 if there is no corresponding character.
 */
static unsigned char get_latin1_char(XKeyEvent *ke)
{
   unsigned char buf[5];

   if (XLookupString(ke, buf, sizeof buf, NULL, NULL) == 1)
      return buf[0];

   return 0;
}



/* get_modifiers: [bgman thread]
 *  Helper to convert the modifier state in the given XKeyEvent to
 *  Allegro modifier bitflags.
 */
static unsigned int get_modifiers(XKeyEvent *ke)
{
   unsigned int state = ke->state;
   unsigned int mods = 0;
   if (state & ShiftMask)   mods |= AL_KEYMOD_SHIFT;
   if (state & ControlMask) mods |= AL_KEYMOD_CTRL;
   if (state & Mod1Mask)    mods |= AL_KEYMOD_ALT;
   if (state & Mod5Mask)    mods |= AL_KEYMOD_SCROLLLOCK;
   if (state & Mod2Mask)    mods |= AL_KEYMOD_NUMLOCK;
   if (state & LockMask)    mods |= AL_KEYMOD_CAPSLOCK;
   return mods;
}



/* handle_key_press: [bgman thread]
 *  Hook for the X event dispatcher to handle key presses.
 *  The caller must lock the X-display.
 */
static unsigned int key_state;
static int last_press_code = -1;

static void handle_key_press(XKeyEvent *ke, bool state_field_reliable)
{
   int mycode;
   bool is_repeat;
   AL_EVENT *event;
   unsigned int type;

   if (ke->keycode >= XKEYCODE_TO_MYCODE_SIZE)
      return;

   mycode = xkeycode_to_mycode[ke->keycode];
   if (mycode < 0)
      return;

   is_repeat = (last_press_code == mycode);
   last_press_code = mycode;

   /* TODO: I don't like this code being here */
   if (!state_field_reliable) {
      switch (mycode) {
         case AL_KEY_LSHIFT:
         case AL_KEY_RSHIFT:     key_state |= ShiftMask; break;
         case AL_KEY_LCTRL:
         case AL_KEY_RCTRL:      key_state |= ControlMask; break;
         case AL_KEY_ALT:
         case AL_KEY_ALTGR:      key_state |= Mod1Mask; break;
         case AL_KEY_SCROLLLOCK: key_state ^= Mod5Mask; break;
         case AL_KEY_NUMLOCK:    key_state ^= Mod2Mask; break;
         case AL_KEY_CAPSLOCK:   key_state ^= LockMask; break;
      }
      ke->state = key_state;
   }

   _al_event_source_lock(&the_keyboard.parent.es);
   {
      /* Update the key_down array.  */
      _AL_KBDSTATE_SET_KEY_DOWN(the_keyboard.state, mycode);

      /* Generate the press event if necessary. */
      type = is_repeat ? AL_EVENT_KEY_REPEAT : AL_EVENT_KEY_DOWN;
      if ((_al_event_source_needs_to_generate_event(&the_keyboard.parent.es, type)) &&
          (event = _al_event_source_get_unused_event(&the_keyboard.parent.es)))
      {
         event->keyboard.type = type;
         event->keyboard.timestamp = al_current_time();
         event->keyboard.__display__dont_use_yet__ = NULL; /* TODO */
         event->keyboard.keycode   = mycode;
         event->keyboard.unichar   = get_latin1_char(ke);
         event->keyboard.modifiers = get_modifiers(ke);
         _al_event_source_emit_event(&the_keyboard.parent.es, event);
      }
   }
   _al_event_source_unlock(&the_keyboard.parent.es);

   /* Exit by Ctrl-Alt-End.  */
   if ((_al_three_finger_flag)
       && ((mycode == AL_KEY_DELETE) || (mycode == AL_KEY_END))
       && (ke->state & ControlMask)
       && (ke->state & Mod1Mask))
      kill(main_pid, SIGTERM);
}



/* handle_key_release: [bgman thread]
 *  Hook for the X event dispatcher to handle key releases.
 *  The caller must lock the X-display.
 */
static void handle_key_release(XKeyEvent *ke, bool state_field_reliable)
{
   int mycode;
   AL_EVENT *event;
   
   if (ke->keycode >= XKEYCODE_TO_MYCODE_SIZE)
      return;

   mycode = xkeycode_to_mycode[ke->keycode];
   if (mycode < 0)
      return;

   if (last_press_code == mycode)
      last_press_code = -1;

   /* TODO: I don't like this code being here */
   if (!state_field_reliable) {
      switch (mycode) {
         case AL_KEY_LSHIFT:
         case AL_KEY_RSHIFT: key_state &=~ ShiftMask; break;
         case AL_KEY_LCTRL:
         case AL_KEY_RCTRL:  key_state &=~ ControlMask; break;
         case AL_KEY_ALT:
         case AL_KEY_ALTGR:  key_state &=~ Mod1Mask; break;
      }
   }

   _al_event_source_lock(&the_keyboard.parent.es);
   {
      /* Update the key_down array.  */
      _AL_KBDSTATE_CLEAR_KEY_DOWN(the_keyboard.state, mycode);

      /* Generate the release event if necessary. */
      if ((_al_event_source_needs_to_generate_event(&the_keyboard.parent.es, AL_EVENT_KEY_UP)) &&
          (event = _al_event_source_get_unused_event(&the_keyboard.parent.es)))
      {
         event->keyboard.type = AL_EVENT_KEY_UP;
         event->keyboard.timestamp = al_current_time();
         event->keyboard.__display__dont_use_yet__ = NULL; /* TODO */
         event->keyboard.keycode = mycode;
         event->keyboard.unichar = 0;
         event->keyboard.modifiers = 0;
         _al_event_source_emit_event(&the_keyboard.parent.es, event);
      }
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}



/* handle_focus_change: [bgman thread]
 *  Hook for the X event dispatcher to handle focus changes.
 *  The caller must lock the X-display.
 */
static void handle_focus_change(bool focused)
{
   /* TODO */
   
#if 0
   if (focused) {
      int mask = AL_KEYMOD_SCROLLLOCK | AL_KEYMOD_NUMLOCK | AL_KEYMOD_CAPSLOCK;
      _key_shifts = (_key_shifts & ~mask) | (state & mask);
   }
#endif
}



/*
 * Mappings between KeySym and Allegro keycodes.
 */

static struct {
   KeySym keysym;
   int mycode;
} keysym_to_mycode[] = {
   { XK_Escape,         AL_KEY_ESCAPE },
   { XK_F1,             AL_KEY_F1 },
   { XK_F2,             AL_KEY_F2 },
   { XK_F3,             AL_KEY_F3 },
   { XK_F4,             AL_KEY_F4 },
   { XK_F5,             AL_KEY_F5 },
   { XK_F6,             AL_KEY_F6 },
   { XK_F7,             AL_KEY_F7 },
   { XK_F8,             AL_KEY_F8 },
   { XK_F9,             AL_KEY_F9 },
   { XK_F10,            AL_KEY_F10 },
   { XK_F11,            AL_KEY_F11 },
   { XK_F12,            AL_KEY_F12 },

   { XK_Print,          AL_KEY_PRINTSCREEN },
   { XK_Scroll_Lock,    AL_KEY_SCROLLLOCK },
   { XK_Pause,          AL_KEY_PAUSE },

   { XK_grave,          AL_KEY_TILDE },
   { XK_quoteleft,      AL_KEY_TILDE },
   { XK_asciitilde,     AL_KEY_TILDE },
   { XK_1,              AL_KEY_1 },
   { XK_2,              AL_KEY_2 },
   { XK_3,              AL_KEY_3 },
   { XK_4,              AL_KEY_4 },
   { XK_5,              AL_KEY_5 },
   { XK_6,              AL_KEY_6 },
   { XK_7,              AL_KEY_7 },
   { XK_8,              AL_KEY_8 },
   { XK_9,              AL_KEY_9 },
   { XK_0,              AL_KEY_0 },
   { XK_minus,          AL_KEY_MINUS },
   { XK_equal,          AL_KEY_EQUALS },
   { XK_backslash,      AL_KEY_BACKSLASH },
   { XK_BackSpace,      AL_KEY_BACKSPACE },

   { XK_Tab,            AL_KEY_TAB },
   { XK_q,              AL_KEY_Q },
   { XK_w,              AL_KEY_W },
   { XK_e,              AL_KEY_E },
   { XK_r,              AL_KEY_R },
   { XK_t,              AL_KEY_T },
   { XK_y,              AL_KEY_Y },
   { XK_u,              AL_KEY_U },
   { XK_i,              AL_KEY_I },
   { XK_o,              AL_KEY_O },
   { XK_p,              AL_KEY_P },
   { XK_bracketleft,    AL_KEY_OPENBRACE },
   { XK_bracketright,   AL_KEY_CLOSEBRACE },
   { XK_Return,         AL_KEY_ENTER },

   { XK_Caps_Lock,      AL_KEY_CAPSLOCK },
   { XK_a,              AL_KEY_A },
   { XK_s,              AL_KEY_S },
   { XK_d,              AL_KEY_D },
   { XK_f,              AL_KEY_F },
   { XK_g,              AL_KEY_G },
   { XK_h,              AL_KEY_H },
   { XK_j,              AL_KEY_J },
   { XK_k,              AL_KEY_K },
   { XK_l,              AL_KEY_L },
   { XK_semicolon,      AL_KEY_SEMICOLON },
   { XK_apostrophe,     AL_KEY_QUOTE },

   { XK_Shift_L,        AL_KEY_LSHIFT },
   { XK_z,              AL_KEY_Z },
   { XK_x,              AL_KEY_X },
   { XK_c,              AL_KEY_C },
   { XK_v,              AL_KEY_V },
   { XK_b,              AL_KEY_B },
   { XK_n,              AL_KEY_N },
   { XK_m,              AL_KEY_M },
   { XK_comma,          AL_KEY_COMMA },
   { XK_period,         AL_KEY_FULLSTOP },
   { XK_slash,          AL_KEY_SLASH },
   { XK_Shift_R,        AL_KEY_RSHIFT },

   { XK_Control_L,      AL_KEY_LCTRL },
   { XK_Meta_L,         AL_KEY_ALT },
   { XK_Alt_L,          AL_KEY_ALT },
   { XK_space,          AL_KEY_SPACE },
   { XK_Alt_R,          AL_KEY_ALTGR },
   { XK_Meta_R,         AL_KEY_ALTGR },
   { XK_Menu,           AL_KEY_MENU },
   { XK_Control_R,      AL_KEY_RCTRL },

   { XK_Insert,         AL_KEY_INSERT },
   { XK_Home,           AL_KEY_HOME },
   { XK_Prior,          AL_KEY_PGUP },
   { XK_Delete,         AL_KEY_DELETE },
   { XK_End,            AL_KEY_END },
   { XK_Next,           AL_KEY_PGDN },

   { XK_Up,             AL_KEY_UP },
   { XK_Left,           AL_KEY_LEFT },
   { XK_Down,           AL_KEY_DOWN },
   { XK_Right,          AL_KEY_RIGHT },

   { XK_Num_Lock,       AL_KEY_NUMLOCK },
   { XK_KP_Divide,      AL_KEY_PAD_SLASH },
   { XK_KP_Multiply,    AL_KEY_PAD_ASTERISK },
   { XK_KP_Subtract,    AL_KEY_PAD_MINUS },
   { XK_KP_Home,        AL_KEY_PAD_7 },
   { XK_KP_Up,          AL_KEY_PAD_8 },
   { XK_KP_Prior,       AL_KEY_PAD_9 },
   { XK_KP_Add,         AL_KEY_PAD_PLUS },
   { XK_KP_Left,        AL_KEY_PAD_4 },
   { XK_KP_Begin,       AL_KEY_PAD_5 },
   { XK_KP_Right,       AL_KEY_PAD_6 },
   { XK_KP_End,         AL_KEY_PAD_1 },
   { XK_KP_Down,        AL_KEY_PAD_2 },
   { XK_KP_Next,        AL_KEY_PAD_3 },
   { XK_KP_Enter,       AL_KEY_PAD_ENTER },
   { XK_KP_Insert,      AL_KEY_PAD_0 },
   { XK_KP_Delete,      AL_KEY_PAD_DELETE },

   { NoSymbol,          0 },
};

/* init_keyboard_tables: [primary thread] & [bgman thread]
 *
 *  This function maps X server keycodes to Allegro keycodes.  It looks
 *  up the keysym for each X keycode, then looks up the keysym in the
 *  big table above to find an Allegro keycode.
 *
 *  This is actually wrong.  Allegro's keycodes are supposed to
 *  represent physical keys, but keysyms vary with keymaps.  That's all
 *  we can do with X without asking users to manually configure things.
 *  XXX: Let users customise.
 *
 *  The X-display must be locked for the duration of this function.
 */
static void init_keyboard_tables(void)
{
   int min_keycode;
   int max_keycode;
   int i, j;

   /* Clear mappings.  */
   for (i = 0; i < XKEYCODE_TO_MYCODE_SIZE; i++)
      xkeycode_to_mycode[i] = -1;

   /* Get the number of keycodes.  */
   XDisplayKeycodes(_xwin.display, &min_keycode, &max_keycode);
   min_keycode = MAX(0, min_keycode);
   max_keycode = MIN(max_keycode, XKEYCODE_TO_MYCODE_SIZE-1);

   /* Setup initial X keycode to Allegro keycode mappings.  */
   for (i = min_keycode; i <= max_keycode; i++) {
      KeySym keysym = XKeycodeToKeysym(_xwin.display, i, 0);
      if (keysym != NoSymbol) {
         for (j = 0; keysym_to_mycode[j].keysym != NoSymbol; j++) {
            if (keysym_to_mycode[j].keysym == keysym) {
               xkeycode_to_mycode[i] = keysym_to_mycode[j].mycode;
               break;
            }
         }
      }
   }
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
