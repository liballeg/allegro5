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

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintlnx.h"
#include "allegro5/platform/aintunix.h"


#define PREFIX_I                "al-ckey INFO: "
#define PREFIX_W                "al-ckey WARNING: "
#define PREFIX_E                "al-ckey ERROR: "



typedef struct ALLEGRO_KEYBOARD_LINUX
{
   ALLEGRO_KEYBOARD parent;
   int fd;
   struct termios startup_termio;
   struct termios work_termio;
   int startup_kbmode;
   ALLEGRO_KEYBOARD_STATE state;
   unsigned int modifiers;
} ALLEGRO_KEYBOARD_LINUX;



/* the one and only keyboard object */
static ALLEGRO_KEYBOARD_LINUX the_keyboard;

/* the pid to kill when three finger saluting */
static pid_t main_pid;



/* forward declarations */
static bool lkeybd_init_keyboard(void);
static void lkeybd_exit_keyboard(void);
static ALLEGRO_KEYBOARD *lkeybd_get_keyboard(void);
static bool lkeybd_set_keyboard_leds(int leds);
static void lkeybd_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state);

static void process_new_data(void *unused);
static void process_character(unsigned char ch);
static void handle_key_press(int mycode, unsigned int ascii);
static void handle_key_release(int mycode);



/* the driver vtable */
#define KEYDRV_LINUX    AL_ID('L','N','X','C')

static ALLEGRO_KEYBOARD_DRIVER keydrv_linux =
{
   KEYDRV_LINUX,
   "",
   "",
   "Linux console keyboard",
   lkeybd_init_keyboard,
   lkeybd_exit_keyboard,
   lkeybd_get_keyboard,
   lkeybd_set_keyboard_leds,
   NULL, /* const char *keycode_to_name(int keycode) */
   lkeybd_get_keyboard_state
};



/* list the available drivers */
_AL_DRIVER_INFO _al_linux_keyboard_driver_list[] =
{
   {  KEYDRV_LINUX, &keydrv_linux, true  },
   {  0,            NULL,          0     }
};



/*
 * Various ugly things.
 */


#define KB_MODIFIERS    (ALLEGRO_KEYMOD_SHIFT | ALLEGRO_KEYMOD_CTRL | ALLEGRO_KEYMOD_ALT | \
                         ALLEGRO_KEYMOD_ALTGR | ALLEGRO_KEYMOD_LWIN | ALLEGRO_KEYMOD_RWIN | \
                         ALLEGRO_KEYMOD_MENU)

#define KB_LED_FLAGS    (ALLEGRO_KEYMOD_SCROLLLOCK | ALLEGRO_KEYMOD_NUMLOCK | \
                         ALLEGRO_KEYMOD_CAPSLOCK)


/* lookup table for converting kernel keycodes into Allegro format */
static unsigned char kernel_to_mycode[128] =
{
   /* 0x00 */ 0,                ALLEGRO_KEY_ESCAPE,   ALLEGRO_KEY_1,           ALLEGRO_KEY_2,
   /* 0x04 */ ALLEGRO_KEY_3,         ALLEGRO_KEY_4,        ALLEGRO_KEY_5,           ALLEGRO_KEY_6,
   /* 0x08 */ ALLEGRO_KEY_7,         ALLEGRO_KEY_8,        ALLEGRO_KEY_9,           ALLEGRO_KEY_0,
   /* 0x0C */ ALLEGRO_KEY_MINUS,     ALLEGRO_KEY_EQUALS,   ALLEGRO_KEY_BACKSPACE,   ALLEGRO_KEY_TAB,
   /* 0x10 */ ALLEGRO_KEY_Q,         ALLEGRO_KEY_W,        ALLEGRO_KEY_E,           ALLEGRO_KEY_R,
   /* 0x14 */ ALLEGRO_KEY_T,         ALLEGRO_KEY_Y,        ALLEGRO_KEY_U,           ALLEGRO_KEY_I,
   /* 0x18 */ ALLEGRO_KEY_O,         ALLEGRO_KEY_P,        ALLEGRO_KEY_OPENBRACE,   ALLEGRO_KEY_CLOSEBRACE,
   /* 0x1C */ ALLEGRO_KEY_ENTER,     ALLEGRO_KEY_LCTRL,    ALLEGRO_KEY_A,           ALLEGRO_KEY_S,
   /* 0x20 */ ALLEGRO_KEY_D,         ALLEGRO_KEY_F,        ALLEGRO_KEY_G,           ALLEGRO_KEY_H,
   /* 0x24 */ ALLEGRO_KEY_J,         ALLEGRO_KEY_K,        ALLEGRO_KEY_L,           ALLEGRO_KEY_SEMICOLON,
   /* 0x28 */ ALLEGRO_KEY_QUOTE,     ALLEGRO_KEY_TILDE,    ALLEGRO_KEY_LSHIFT,      ALLEGRO_KEY_BACKSLASH,
   /* 0x2C */ ALLEGRO_KEY_Z,         ALLEGRO_KEY_X,        ALLEGRO_KEY_C,           ALLEGRO_KEY_V,
   /* 0x30 */ ALLEGRO_KEY_B,         ALLEGRO_KEY_N,        ALLEGRO_KEY_M,           ALLEGRO_KEY_COMMA,
   /* 0x34 */ ALLEGRO_KEY_FULLSTOP,  ALLEGRO_KEY_SLASH,    ALLEGRO_KEY_RSHIFT,      ALLEGRO_KEY_PAD_ASTERISK,
   /* 0x38 */ ALLEGRO_KEY_ALT,       ALLEGRO_KEY_SPACE,    ALLEGRO_KEY_CAPSLOCK,    ALLEGRO_KEY_F1,
   /* 0x3C */ ALLEGRO_KEY_F2,        ALLEGRO_KEY_F3,       ALLEGRO_KEY_F4,          ALLEGRO_KEY_F5,
   /* 0x40 */ ALLEGRO_KEY_F6,        ALLEGRO_KEY_F7,       ALLEGRO_KEY_F8,          ALLEGRO_KEY_F9,
   /* 0x44 */ ALLEGRO_KEY_F10,       ALLEGRO_KEY_NUMLOCK,  ALLEGRO_KEY_SCROLLLOCK,  ALLEGRO_KEY_PAD_7,
   /* 0x48 */ ALLEGRO_KEY_PAD_8,     ALLEGRO_KEY_PAD_9,    ALLEGRO_KEY_PAD_MINUS,   ALLEGRO_KEY_PAD_4,
   /* 0x4C */ ALLEGRO_KEY_PAD_5,     ALLEGRO_KEY_PAD_6,    ALLEGRO_KEY_PAD_PLUS,    ALLEGRO_KEY_PAD_1,
   /* 0x50 */ ALLEGRO_KEY_PAD_2,     ALLEGRO_KEY_PAD_3,    ALLEGRO_KEY_PAD_0,       ALLEGRO_KEY_PAD_DELETE,
   /* 0x54 */ ALLEGRO_KEY_PRINTSCREEN, 0,             ALLEGRO_KEY_BACKSLASH2,  ALLEGRO_KEY_F11,
   /* 0x58 */ ALLEGRO_KEY_F12,       0,               0,                  0,
   /* 0x5C */ 0,                0,               0,                  0,
   /* 0x60 */ ALLEGRO_KEY_PAD_ENTER, ALLEGRO_KEY_RCTRL,    ALLEGRO_KEY_PAD_SLASH,   ALLEGRO_KEY_PRINTSCREEN,
   /* 0x64 */ ALLEGRO_KEY_ALTGR,     ALLEGRO_KEY_PAUSE,    ALLEGRO_KEY_HOME,        ALLEGRO_KEY_UP,
   /* 0x68 */ ALLEGRO_KEY_PGUP,      ALLEGRO_KEY_LEFT,     ALLEGRO_KEY_RIGHT,       ALLEGRO_KEY_END,
   /* 0x6C */ ALLEGRO_KEY_DOWN,      ALLEGRO_KEY_PGDN,     ALLEGRO_KEY_INSERT,      ALLEGRO_KEY_DELETE,
   /* 0x70 */ 0,                0,               0,                  0,
   /* 0x74 */ 0,                0,               0,                  ALLEGRO_KEY_PAUSE,
   /* 0x78 */ 0,                0,               0,                  0,
   /* 0x7C */ 0,                ALLEGRO_KEY_LWIN,     ALLEGRO_KEY_RWIN,        ALLEGRO_KEY_MENU

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


/* convert Allegro format keycodes into ALLEGRO_KEYMOD_* */
static const unsigned int modifier_table[ALLEGRO_KEY_MAX - ALLEGRO_KEY_MODIFIERS] =
{
   ALLEGRO_KEYMOD_SHIFT,      ALLEGRO_KEYMOD_SHIFT,   ALLEGRO_KEYMOD_CTRL,
   ALLEGRO_KEYMOD_CTRL,       ALLEGRO_KEYMOD_ALT,     ALLEGRO_KEYMOD_ALTGR,
   ALLEGRO_KEYMOD_LWIN,       ALLEGRO_KEYMOD_RWIN,    ALLEGRO_KEYMOD_MENU,
   ALLEGRO_KEYMOD_SCROLLLOCK, ALLEGRO_KEYMOD_NUMLOCK, ALLEGRO_KEYMOD_CAPSLOCK
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
   if (modifiers & ALLEGRO_KEYMOD_SHIFT) keymap |= 1;
   if (modifiers & ALLEGRO_KEYMOD_ALTGR) keymap |= 2;
   if (modifiers & ALLEGRO_KEYMOD_CTRL)  keymap |= 4;
   if (modifiers & ALLEGRO_KEYMOD_ALT)   keymap |= 8;

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
         if (modifiers & ALLEGRO_KEYMOD_CAPSLOCK)
            return ascii ^ 0x20;
         else
            return ascii;

      case KT_LATIN:
      case KT_ASCII:
         return ascii;

      case KT_PAD:
      {
         int val = KVAL(kbe.kb_value);
         if (modifiers & ALLEGRO_KEYMOD_NUMLOCK) {
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

   if (__al_linux_use_console())
      return false;

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
 *  Returns the address of a ALLEGRO_KEYBOARD structure representing the keyboard.
 */
static ALLEGRO_KEYBOARD *lkeybd_get_keyboard(void)
{
   return (ALLEGRO_KEYBOARD *)&the_keyboard;
}



/* lkeybd_set_keyboard_leds:
 *  Updates the LED state.
 */
static bool lkeybd_set_keyboard_leds(int leds)
{
   int val = 0;

   if (leds & ALLEGRO_KEYMOD_SCROLLLOCK) val |= LED_SCR;
   if (leds & ALLEGRO_KEYMOD_NUMLOCK)    val |= LED_NUM;
   if (leds & ALLEGRO_KEYMOD_CAPSLOCK)   val |= LED_CAP;

   return (ioctl(the_keyboard.fd, KDSETLED, val) == 0) ? true : false;
}



/* lkeybd_get_keyboard_state: [primary thread]
 *  Copy the current keyboard state into RET_STATE, with any necessary locking.
 */
static void lkeybd_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state)
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
   if (mycode >= ALLEGRO_KEY_MODIFIERS) {
      int flag = modifier_table[mycode - ALLEGRO_KEY_MODIFIERS];
      if (press) {
         if (flag & KB_MODIFIERS)
            the_keyboard.modifiers |= flag;
         else if ((flag & KB_LED_FLAGS) && _al_key_led_flag)
            the_keyboard.modifiers ^= flag;
      }
      else {
         /* XXX: if the user presses LCTRL, then RCTRL, then releases
          * LCTRL, the ALLEGRO_KEYMOD_CTRL modifier should still be on.
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
       && ((mycode == ALLEGRO_KEY_DELETE) || (mycode == ALLEGRO_KEY_END))
       && (the_keyboard.modifiers & ALLEGRO_KEYMOD_CTRL)
       && (the_keyboard.modifiers & ALLEGRO_KEYMOD_ALT))
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
   ALLEGRO_EVENT_TYPE event_type;
   ALLEGRO_EVENT event;

   event_type = (_AL_KEYBOARD_STATE_KEY_DOWN(the_keyboard.state, mycode)
                 ? ALLEGRO_EVENT_KEY_REPEAT
                 : ALLEGRO_EVENT_KEY_DOWN);
   
   /* Maintain the key_down array. */
   _AL_KEYBOARD_STATE_SET_KEY_DOWN(the_keyboard.state, mycode);

   /* Generate key press/repeat events if necessary. */   
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.parent.es))
      return;

   event.keyboard.type = event_type;
   event.keyboard.timestamp = al_get_time();
   event.keyboard.display = NULL;
   event.keyboard.keycode = mycode;
   event.keyboard.unichar = ascii;
   event.keyboard.modifiers = the_keyboard.modifiers;

   _al_event_source_emit_event(&the_keyboard.parent.es, &event);

   /* The first press should generate a KEY_CHAR also */
   if (event_type == ALLEGRO_EVENT_KEY_DOWN) {
      event.keyboard.type = ALLEGRO_EVENT_KEY_CHAR;
      event.keyboard.timestamp = al_get_time();
      _al_event_source_emit_event(&the_keyboard.parent.es, &event);
   }
}



/* handle_key_release: [fdwatch thread]
 *  Helper: stuff to do when a key is released.
 */
static void handle_key_release(int mycode)
{
   ALLEGRO_EVENT event;

   /* This can happen, e.g. when we are switching back into a VT with
    * ALT+Fn, we only get the release event of the function key.
    */
   if (!_AL_KEYBOARD_STATE_KEY_DOWN(the_keyboard.state, mycode))
      return;

   /* Maintain the key_down array. */
   _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(the_keyboard.state, mycode);

   /* Generate key release events if necessary. */
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.parent.es))
      return;

   event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
   event.keyboard.timestamp = al_get_time();
   event.keyboard.display = NULL;
   event.keyboard.keycode = mycode;
   event.keyboard.unichar = 0;
   event.keyboard.modifiers = 0;

   _al_event_source_emit_event(&the_keyboard.parent.es, &event);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
