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
 *      Linux console keyboard driver.
 *
 *      By Marek Habersack, modified by George Foot.
 *
 *      Modified for the new keyboard API by Peter Wang.
 *
 *      See readme.txt for copyright information.
 *
 *---
 *      Recommended reading:
 *
 *      - "Kernel Korner: The Linux keyboard driver" by Andries E. Brouwer
 *        http://www.linuxjournal.com/article.php?sid=1080
 *
 *      - Console Programming HOWTO
 *
 *      TODO: diacriticals
 */


#define ALLEGRO_NO_COMPATIBILITY

#include <fcntl.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <linux/vt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_events.h"
#include "allegro/internal/aintern_keyboard.h"
#include "allegro/platform/aintlnx.h"


#define PREFIX_I                "al-ckey INFO: "
#define PREFIX_W                "al-ckey WARNING: "
#define PREFIX_E                "al-ckey ERROR: "



typedef struct AL_KEYBOARD_LINUX
{
   AL_KEYBOARD parent;
   int fd;
   struct termios startup_termio;
   struct termios work_termio;
   int startup_kbmode;
   AL_KBDSTATE state;
   unsigned int modifiers;
} AL_KEYBOARD_LINUX;



/* the one and only keyboard object */
static AL_KEYBOARD_LINUX the_keyboard;

/* the pid to kill when three finger saluting */
static pid_t main_pid;



/* forward declarations */
static bool lkeybd_init_keyboard(void);
static void lkeybd_exit_keyboard(void);
static AL_KEYBOARD *lkeybd_get_keyboard(void);
static bool lkeybd_set_keyboard_leds(int leds);
static void lkeybd_get_keyboard_state(AL_KBDSTATE *ret_state);

static void process_new_data(void *unused);
static void process_character(unsigned char ch);
static void handle_key_press(int mycode, unsigned int ascii);
static void handle_key_release(int mycode);



/* the driver vtable */
#define KEYDRV_LINUX    AL_ID('L','N','X','C')

static AL_KEYBOARD_DRIVER keydrv_linux =
{
   KEYDRV_LINUX,
   empty_string,
   empty_string,
   "Linux console keyboard",
   lkeybd_init_keyboard,
   lkeybd_exit_keyboard,
   lkeybd_get_keyboard,
   lkeybd_set_keyboard_leds,
   NULL, /* const char *keycode_to_name(int keycode) */
   lkeybd_get_keyboard_state
};



/* list the available drivers */
_DRIVER_INFO _al_linux_keyboard_driver_list[] =
{
   {  KEYDRV_LINUX, &keydrv_linux, TRUE  },
   {  0,            NULL,          0     }
};



/*
 * Various ugly things.
 */


#define KB_MODIFIERS    (AL_KEYMOD_SHIFT | AL_KEYMOD_CTRL | AL_KEYMOD_ALT | \
                         AL_KEYMOD_ALTGR | AL_KEYMOD_LWIN | AL_KEYMOD_RWIN | \
                         AL_KEYMOD_MENU)

#define KB_LED_FLAGS    (AL_KEYMOD_SCROLLLOCK | AL_KEYMOD_NUMLOCK | \
                         AL_KEYMOD_CAPSLOCK)


/* lookup table for converting kernel keycodes into Allegro format */
static unsigned char kernel_to_mycode[128] =
{
   /* 0x00 */ 0,                AL_KEY_ESCAPE,   AL_KEY_1,           AL_KEY_2,
   /* 0x04 */ AL_KEY_3,         AL_KEY_4,        AL_KEY_5,           AL_KEY_6,
   /* 0x08 */ AL_KEY_7,         AL_KEY_8,        AL_KEY_9,           AL_KEY_0,
   /* 0x0C */ AL_KEY_MINUS,     AL_KEY_EQUALS,   AL_KEY_BACKSPACE,   AL_KEY_TAB,
   /* 0x10 */ AL_KEY_Q,         AL_KEY_W,        AL_KEY_E,           AL_KEY_R,
   /* 0x14 */ AL_KEY_T,         AL_KEY_Y,        AL_KEY_U,           AL_KEY_I,
   /* 0x18 */ AL_KEY_O,         AL_KEY_P,        AL_KEY_OPENBRACE,   AL_KEY_CLOSEBRACE,
   /* 0x1C */ AL_KEY_ENTER,     AL_KEY_LCTRL,    AL_KEY_A,           AL_KEY_S,
   /* 0x20 */ AL_KEY_D,         AL_KEY_F,        AL_KEY_G,           AL_KEY_H,
   /* 0x24 */ AL_KEY_J,         AL_KEY_K,        AL_KEY_L,           AL_KEY_SEMICOLON,
   /* 0x28 */ AL_KEY_QUOTE,     AL_KEY_TILDE,    AL_KEY_LSHIFT,      AL_KEY_BACKSLASH,
   /* 0x2C */ AL_KEY_Z,         AL_KEY_X,        AL_KEY_C,           AL_KEY_V,
   /* 0x30 */ AL_KEY_B,         AL_KEY_N,        AL_KEY_M,           AL_KEY_COMMA,
   /* 0x34 */ AL_KEY_FULLSTOP,  AL_KEY_SLASH,    AL_KEY_RSHIFT,      AL_KEY_PAD_ASTERISK,
   /* 0x38 */ AL_KEY_ALT,       AL_KEY_SPACE,    AL_KEY_CAPSLOCK,    AL_KEY_F1,
   /* 0x3C */ AL_KEY_F2,        AL_KEY_F3,       AL_KEY_F4,          AL_KEY_F5,
   /* 0x40 */ AL_KEY_F6,        AL_KEY_F7,       AL_KEY_F8,          AL_KEY_F9,
   /* 0x44 */ AL_KEY_F10,       AL_KEY_NUMLOCK,  AL_KEY_SCROLLLOCK,  AL_KEY_PAD_7,
   /* 0x48 */ AL_KEY_PAD_8,     AL_KEY_PAD_9,    AL_KEY_PAD_MINUS,   AL_KEY_PAD_4,
   /* 0x4C */ AL_KEY_PAD_5,     AL_KEY_PAD_6,    AL_KEY_PAD_PLUS,    AL_KEY_PAD_1,
   /* 0x50 */ AL_KEY_PAD_2,     AL_KEY_PAD_3,    AL_KEY_PAD_0,       AL_KEY_PAD_DELETE,
   /* 0x54 */ AL_KEY_PRINTSCREEN, 0,             AL_KEY_BACKSLASH2,  AL_KEY_F11,
   /* 0x58 */ AL_KEY_F12,       0,               0,                  0,
   /* 0x5C */ 0,                0,               0,                  0,
   /* 0x60 */ AL_KEY_PAD_ENTER, AL_KEY_RCTRL,    AL_KEY_PAD_SLASH,   AL_KEY_PRINTSCREEN,
   /* 0x64 */ AL_KEY_ALTGR,     AL_KEY_PAUSE,    AL_KEY_HOME,        AL_KEY_UP,
   /* 0x68 */ AL_KEY_PGUP,      AL_KEY_LEFT,     AL_KEY_RIGHT,       AL_KEY_END,
   /* 0x6C */ AL_KEY_DOWN,      AL_KEY_PGDN,     AL_KEY_INSERT,      AL_KEY_DELETE,
   /* 0x70 */ 0,                0,               0,                  0,
   /* 0x74 */ 0,                0,               0,                  AL_KEY_PAUSE,
   /* 0x78 */ 0,                0,               0,                  0,
   /* 0x7C */ 0,                AL_KEY_LWIN,     AL_KEY_RWIN,        AL_KEY_MENU

   /* "Two keys are unusual in the sense that their keycode is not constant,
    *  but depends on modifiers. The PrintScrn key will yield keycode 84 when
    *  combined with either Alt key, but keycode 99 otherwise. The Pause key
    *  will yield keycode 101 when combined with either Ctrl key, but keycode
    *  119 otherwise. (This has historic reasons, but might change, to free
    *  keycodes 99 and 119 for other purposes.)"
    */
};


/* convert kernel keycodes for numpad keys into ASCII characters */
#define NUM_PAD_KEYS 17
static const char pad_asciis[NUM_PAD_KEYS] =
{
   '0','1','2','3','4','5','6','7','8','9',
   '+','-','*','/','\r',',','.'
};
static const char pad_asciis_no_numlock[NUM_PAD_KEYS] =
{
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   '+', '-', '*', '/', '\r', 0, 0
};


/* convert Allegro format keycodes into AL_KEYMOD_* */
static const unsigned int modifier_table[AL_KEY_MAX - AL_KEY_MODIFIERS] =
{
   AL_KEYMOD_SHIFT,      AL_KEYMOD_SHIFT,   AL_KEYMOD_CTRL,
   AL_KEYMOD_CTRL,       AL_KEYMOD_ALT,     AL_KEYMOD_ALTGR,
   AL_KEYMOD_LWIN,       AL_KEYMOD_RWIN,    AL_KEYMOD_MENU,
   AL_KEYMOD_SCROLLLOCK, AL_KEYMOD_NUMLOCK, AL_KEYMOD_CAPSLOCK
};



/* keycode_to_char: [fdwatch thread]
 *  Helper function.
 *  KEYCODE is a Linux kernel keycode, not an Allegro keycode.
 *
 *  In the process of doing the translation, we may find that the user
 *  has pressed a key that for VT switching. In that case a negative
 *  number is returned, the absolute value of which is the VT to
 *  switch to. Yes, ugly.
 */
static int keycode_to_char(int keycode)
{
   const unsigned int modifiers = the_keyboard.modifiers;
   struct kbentry kbe;
   int keymap;
   int ascii;

   /* build kernel keymap number */
   keymap = 0;
   if (modifiers & AL_KEYMOD_SHIFT) keymap |= 1;
   if (modifiers & AL_KEYMOD_ALTGR) keymap |= 2;
   if (modifiers & AL_KEYMOD_CTRL)  keymap |= 4;
   if (modifiers & AL_KEYMOD_ALT)   keymap |= 8;

   /* map keycode to type and value */
   kbe.kb_table = keymap;
   kbe.kb_index = keycode;
   ioctl(the_keyboard.fd, KDGKBENT, &kbe);

   if (keycode == KEY_BACKSPACE)
      ascii = 8;
   else {
      if (kbe.kb_value == K_NOSUCHMAP) {
         /* invalid keymaps */
         /* FIXME: Maybe we should just redo the */
         /*        ioctl with keymap 0? */
         ascii = 0;
      }
      else {
         /* most keys come here */
         ascii = KVAL(kbe.kb_value);
      }
   }

   /* finally do some things based on key type */
   switch (KTYP(kbe.kb_value)) {

      case KT_CONS:
         /* VT switch key -- return a negative number */
         return -( KVAL(kbe.kb_value)+1 );

      case KT_LETTER:
         /* apply capslock translation. */
         if (modifiers & AL_KEYMOD_CAPSLOCK)
            return ascii ^ 0x20;
         else
            return ascii;

      case KT_LATIN:
      case KT_ASCII:
         return ascii;

      case KT_PAD:
      {
         int val = KVAL(kbe.kb_value);
         if (modifiers & AL_KEYMOD_NUMLOCK) {
            if ((val >= 0) && (val < NUM_PAD_KEYS))
               ascii = pad_asciis[val];
         } else {
            if ((val >= 0) && (val < NUM_PAD_KEYS))
               ascii = pad_asciis_no_numlock[val];
         }
         return ascii;
      }

      case KT_SPEC:
         if (keycode == KEY_ENTER)
            return '\r';
         else
            return 0;

      default:
         /* dunno */
         return 0;
   }
}



/* lkeybd_init_keyboard: [primary thread]
 *  Initialise the keyboard driver.
 */
static bool lkeybd_init_keyboard(void)
{
   bool can_restore_termio_and_kbmode = false;

   memset(&the_keyboard, 0, sizeof the_keyboard);

   if (__al_linux_use_console()) return false;

   the_keyboard.fd = dup(__al_linux_console_fd);

   /* Save the current terminal attributes, which we will restore when
    * we close up shop.
    */
   if (tcgetattr(the_keyboard.fd, &the_keyboard.startup_termio) != 0)
      goto Error;

   /* Save previous keyboard mode (probably XLATE). */
   if (ioctl(the_keyboard.fd, KDGKBMODE, &the_keyboard.startup_kbmode) != 0)
      goto Error;

   can_restore_termio_and_kbmode = true;

   /* Set terminal attributes we need.
    *
    * Input modes (c_iflag): We want to disable:
    *  - stripping bytes to 7 bits
    *  - ignoring of carriage returns
    *  - translating of carriage returns to newlines, and vice versa
    *  - start/stop control on input and output
    *
    * Control modes (c_cflag): We want 8 bits per byte.
    *
    * Local modes (c_lflag: We want to disable:
    *  - canonical (line by line) input
    *  - echoing input back to the display
    *  - interpretation of signal characters
    *
    * The c_iflag, c_lflag settings come from svgalib. Allegro 4
    * simply set them to 0, which is a bit crude.
    */
   the_keyboard.work_termio = the_keyboard.startup_termio;
   the_keyboard.work_termio.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
   the_keyboard.work_termio.c_cflag &= ~CSIZE;
   the_keyboard.work_termio.c_cflag |= CS8;
   the_keyboard.work_termio.c_lflag &= ~(ICANON | ECHO | ISIG);

   if (tcsetattr(the_keyboard.fd, TCSANOW, &the_keyboard.work_termio) != 0)
      goto Error;

   /* Set the keyboard mode to mediumraw. */
   if (ioctl(the_keyboard.fd, KDSKBMODE, K_MEDIUMRAW) != 0)
      goto Error;

   /* Initialise the keyboard object for use as an event source. */
   _al_event_source_init(&the_keyboard.parent.es);

   /* Start watching for data on the fd. */
   _al_unix_start_watching_fd(the_keyboard.fd, process_new_data, NULL);

   /* Get the pid, which we use for the three finger salute */
   main_pid = getpid();

   return true;

  Error:

   if (can_restore_termio_and_kbmode) {
      tcsetattr(the_keyboard.fd, TCSANOW, &the_keyboard.startup_termio);
      ioctl(the_keyboard.fd, KDSKBMODE, the_keyboard.startup_kbmode);
   }

   close(the_keyboard.fd);

   __al_linux_leave_console();

   return false;
}



/* lkeybd_exit_keyboard: [primary thread]
 *  Shut down the keyboard driver.
 */
static void lkeybd_exit_keyboard(void)
{
   _al_unix_stop_watching_fd(the_keyboard.fd);

   _al_event_source_free(&the_keyboard.parent.es);

   /* Restore terminal attrs, keyboard mode, and reset the LED mode. */
   tcsetattr(the_keyboard.fd, TCSANOW, &the_keyboard.startup_termio);
   ioctl(the_keyboard.fd, KDSKBMODE, the_keyboard.startup_kbmode);
   ioctl(the_keyboard.fd, KDSETLED, 8);

   close(the_keyboard.fd);

   __al_linux_leave_console();

   /* This may help catch bugs in the user program, since the pointer
    * we return to the user is always the same.
    */
   memset(&the_keyboard, 0, sizeof the_keyboard);
}



/* lkeybd_get_keyboard:
 *  Returns the address of a AL_KEYBOARD structure representing the keyboard.
 */
static AL_KEYBOARD *lkeybd_get_keyboard(void)
{
   return (AL_KEYBOARD *)&the_keyboard;
}



/* lkeybd_set_keyboard_leds:
 *  Updates the LED state.
 */
static bool lkeybd_set_keyboard_leds(int leds)
{
   int val = 0;

   if (leds & AL_KEYMOD_SCROLLLOCK) val |= LED_SCR;
   if (leds & AL_KEYMOD_NUMLOCK)    val |= LED_NUM;
   if (leds & AL_KEYMOD_CAPSLOCK)   val |= LED_CAP;

   return (ioctl(the_keyboard.fd, KDSETLED, val) == 0) ? true : false;
}



/* lkeybd_get_keyboard_state: [primary thread]
 *  Copy the current keyboard state into RET_STATE, with any necessary locking.
 */
static void lkeybd_get_keyboard_state(AL_KBDSTATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.parent.es);
   {
      *ret_state = the_keyboard.state;
   }
   _al_event_source_unlock(&the_keyboard.parent.es);
}



/* process_new_data: [fdwatch thread]
 *  Process new data arriving in the keyboard's fd.
 */
static void process_new_data(void *unused)
{
   _al_event_source_lock(&the_keyboard.parent.es);
   {
      unsigned char buf[128];
      size_t bytes_read;
      unsigned int ch;

      bytes_read = read(the_keyboard.fd, &buf, sizeof(buf));
      for (ch = 0; ch < bytes_read; ch++)
         process_character(buf[ch]);
   }
   _al_event_source_unlock(&the_keyboard.parent.es);

   (void)unused;
}



/* process_character: [fdwatch thread]
 *  This is where most of the work gets done. CH is a byte read in
 *  from the keyboard's fd. This function finds the Allegro equivalent
 *  of that key code, figures out which ASCII character that key
 *  generates (if any), and then calls the handle_key_press() and
 *  handle_key_release() functions with that information. It also does
 *  some things with modifier keys.
 */
static void process_character(unsigned char ch)
{
   /* read kernel's keycode and convert to Allegro's keycode */
   int keycode = ch & 0x7f;
   bool press = !(ch & 0x80);
   int mycode = kernel_to_mycode[keycode];

   /* if the key was something weird we don't understand, skip it */
   if (mycode == 0)
      return;

   /* process modifiers */
   if (mycode >= AL_KEY_MODIFIERS) {
      int flag = modifier_table[mycode - AL_KEY_MODIFIERS];
      if (press) {
         if (flag & KB_MODIFIERS)
            the_keyboard.modifiers |= flag;
         else if ((flag & KB_LED_FLAGS) && _al_key_led_flag)
            the_keyboard.modifiers ^= flag;
      }
      else {
         /* XXX: if the user presses LCTRL, then RCTRL, then releases
          * LCTRL, the AL_KEYMOD_CTRL modifier should still be on.
          */
         if (flag & KB_MODIFIERS)
            the_keyboard.modifiers &= ~flag;
      }
   }

   /* call the handlers */
   if (press) {
      int ascii = keycode_to_char(keycode);

      /* switch VT if the user requested so */
      if (ascii < 0) {
         int console = -ascii;
         int last_console;

         ioctl(the_keyboard.fd, VT_OPENQRY, &last_console);
         if (console < last_console)
            if (ioctl(the_keyboard.fd, VT_ACTIVATE, console) == 0)
               return;
      }

      handle_key_press(mycode, ascii);
   }
   else {
      handle_key_release(mycode);
   }
   
   /* three-finger salute for killing the program */
   if ((_al_three_finger_flag)
       && ((mycode == AL_KEY_DELETE) || (mycode == AL_KEY_END))
       && (the_keyboard.modifiers & AL_KEYMOD_CTRL)
       && (the_keyboard.modifiers & AL_KEYMOD_ALT))
   {
      TRACE(PREFIX_W "Three finger combo detected. SIGTERMing "
	 "pid %d\n", main_pid);
      kill(main_pid, SIGTERM);
   }
}



/* handle_key_press: [fdwatch thread]
 *  Helper: stuff to do when a key is pressed.
 */
static void handle_key_press(int mycode, unsigned int ascii)
{
   AL_EVENT_TYPE event_type;
   AL_EVENT *event;

   event_type = (_AL_KBDSTATE_KEY_DOWN(the_keyboard.state, mycode)
                 ? AL_EVENT_KEY_REPEAT
                 : AL_EVENT_KEY_DOWN);
   
   /* Maintain the key_down array. */
   _AL_KBDSTATE_SET_KEY_DOWN(the_keyboard.state, mycode);

   /* Generate key press/repeat events if necessary. */   
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.parent.es))
      return;

   event = _al_event_source_get_unused_event(&the_keyboard.parent.es);
   if (!event)
      return;

   event->keyboard.type = event_type;
   event->keyboard.timestamp = al_current_time();
   event->keyboard.__display__dont_use_yet__ = NULL;
   event->keyboard.keycode = mycode;
   event->keyboard.unichar = ascii;
   event->keyboard.modifiers = the_keyboard.modifiers;

   _al_event_source_emit_event(&the_keyboard.parent.es, event);
}



/* handle_key_release: [fdwatch thread]
 *  Helper: stuff to do when a key is released.
 */
static void handle_key_release(int mycode)
{
   AL_EVENT *event;

   /* This can happen, e.g. when we are switching back into a VT with
    * ALT+Fn, we only get the release event of the function key.
    */
   if (!_AL_KBDSTATE_KEY_DOWN(the_keyboard.state, mycode))
      return;

   /* Maintain the key_down array. */
   _AL_KBDSTATE_CLEAR_KEY_DOWN(the_keyboard.state, mycode);

   /* Generate key release events if necessary. */
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.parent.es))
      return;

   event = _al_event_source_get_unused_event(&the_keyboard.parent.es);
   if (!event)
      return;

   event->keyboard.type = AL_EVENT_KEY_UP;
   event->keyboard.timestamp = al_current_time();
   event->keyboard.__display__dont_use_yet__ = NULL;
   event->keyboard.keycode = mycode;
   event->keyboard.unichar = 0;
   event->keyboard.modifiers = 0;

   _al_event_source_emit_event(&the_keyboard.parent.es, event);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
