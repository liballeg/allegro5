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
 *      Linux console keyboard module.
 *
 *      By Marek Habersack, modified by George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"


/* list the available drivers */
_DRIVER_INFO _linux_keyboard_driver_list[] =
{
   {  KEYDRV_LINUX,   &keydrv_linux_console, TRUE  },
   {  0,               NULL,                 0     }
};


static int  linux_key_init (void);
static void linux_key_exit (void);
static void linux_set_leds (int leds);
static void linux_set_rate (int delay, int rate);
static int  linux_scancode_to_ascii (int scancode);

KEYBOARD_DRIVER keydrv_linux_console =
{
   KEYDRV_LINUX,
   empty_string,
   empty_string,
   "Linux console keyboard",
   FALSE,
   linux_key_init,
   linux_key_exit,
   NULL,
   linux_set_leds,
   linux_set_rate,
   NULL, NULL,
   linux_scancode_to_ascii
};

static int update_keyboard (void);
static void resume_keyboard (void);
static void suspend_keyboard (void);

static STD_DRIVER std_keyboard =
{
   STD_KBD,
   update_keyboard,
   resume_keyboard,
   suspend_keyboard,
   -1  /* fd -- filled in later */
};


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <linux/vt.h>


static int startup_kbmode;
static int resume_count;
static int main_pid;


#define KB_MODIFIERS    (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG | KB_LWIN_FLAG | KB_RWIN_FLAG | KB_MENU_FLAG)
#define KB_LED_FLAGS    (KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG)


/* lookup table for converting kernel scancodes into Allegro format */
static unsigned char kernel_to_mycode[128] =
{
   /* 0x00 */  0,              KEY_ESC,        KEY_1,          KEY_2,
   /* 0x04 */  KEY_3,          KEY_4,          KEY_5,          KEY_6,
   /* 0x08 */  KEY_7,          KEY_8,          KEY_9,          KEY_0,
   /* 0x0C */  KEY_MINUS,      KEY_EQUALS,     KEY_BACKSPACE,  KEY_TAB,
   /* 0x10 */  KEY_Q,          KEY_W,          KEY_E,          KEY_R,
   /* 0x14 */  KEY_T,          KEY_Y,          KEY_U,          KEY_I,
   /* 0x18 */  KEY_O,          KEY_P,          KEY_OPENBRACE,  KEY_CLOSEBRACE,
   /* 0x1C */  KEY_ENTER,      KEY_LCONTROL,   KEY_A,          KEY_S,
   /* 0x20 */  KEY_D,          KEY_F,          KEY_G,          KEY_H,
   /* 0x24 */  KEY_J,          KEY_K,          KEY_L,          KEY_COLON,
   /* 0x28 */  KEY_QUOTE,      KEY_TILDE,      KEY_LSHIFT,     KEY_BACKSLASH,
   /* 0x2C */  KEY_Z,          KEY_X,          KEY_C,          KEY_V,
   /* 0x30 */  KEY_B,          KEY_N,          KEY_M,          KEY_COMMA,
   /* 0x34 */  KEY_STOP,       KEY_SLASH,      KEY_RSHIFT,     KEY_ASTERISK,
   /* 0x38 */  KEY_ALT,        KEY_SPACE,      KEY_CAPSLOCK,   KEY_F1,
   /* 0x3C */  KEY_F2,         KEY_F3,         KEY_F4,         KEY_F5,
   /* 0x40 */  KEY_F6,         KEY_F7,         KEY_F8,         KEY_F9,
   /* 0x44 */  KEY_F10,        KEY_NUMLOCK,    KEY_SCRLOCK,    KEY_7_PAD,
   /* 0x48 */  KEY_8_PAD,      KEY_9_PAD,      KEY_MINUS_PAD,  KEY_4_PAD,
   /* 0x4C */  KEY_5_PAD,      KEY_6_PAD,      KEY_PLUS_PAD,   KEY_1_PAD,
   /* 0x50 */  KEY_2_PAD,      KEY_3_PAD,      KEY_0_PAD,      KEY_DEL_PAD,
   /* 0x54 */  0,              0,              KEY_BACKSLASH2, KEY_F11,
   /* 0x58 */  KEY_F12,        0,              0,              0,
   /* 0x5C */  0,              0,              0,              0,
   /* 0x60 */  KEY_ENTER_PAD,  KEY_RCONTROL,   KEY_SLASH_PAD,  KEY_PRTSCR,
   /* 0x64 */  KEY_ALTGR,      0,              KEY_HOME,       KEY_UP,
   /* 0x68 */  KEY_PGUP,       KEY_LEFT,       KEY_RIGHT,      KEY_END,
   /* 0x6C */  KEY_DOWN,       KEY_PGDN,       KEY_INSERT,     KEY_DEL,
   /* 0x70 */  0,              0,              0,              0,
   /* 0x74 */  0,              0,              0,              KEY_PAUSE,
   /* 0x78 */  0,              0,              0,              0,
   /* 0x7C */  0,              KEY_LWIN,       KEY_RWIN,       KEY_MENU
};


/* convert Allegro format scancodes into key_shifts flag bits */
static unsigned short modifier_table[KEY_MAX - KEY_MODIFIERS] =
{
   KB_SHIFT_FLAG,    KB_SHIFT_FLAG,    KB_CTRL_FLAG,
   KB_CTRL_FLAG,     KB_ALT_FLAG,      KB_ALT_FLAG,
   KB_LWIN_FLAG,     KB_RWIN_FLAG,     KB_MENU_FLAG,
   KB_SCROLOCK_FLAG, KB_NUMLOCK_FLAG,  KB_CAPSLOCK_FLAG
};

#define NUM_PAD_KEYS 17
static int pad_arrow_codes[NUM_PAD_KEYS] = {
        KEY_INSERT, KEY_END, KEY_DOWN, KEY_PGDN,
        KEY_LEFT, KEY_5_PAD, KEY_RIGHT, KEY_HOME,
        KEY_UP, KEY_PGUP, KEY_PLUS_PAD, KEY_MINUS_PAD,
	KEY_ASTERISK, KEY_SLASH_PAD, KEY_ENTER_PAD, 0/*KEY_COMMA_PAD*/,
	KEY_DEL
};
static char pad_asciis[NUM_PAD_KEYS] = "0123456789+-*/\r,.";
static char pad_asciis_no_numlock[NUM_PAD_KEYS] = { 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	'+', '-', '*', '/', '\r', -1, -1
};


static void process_keyboard_data (unsigned char *buf, size_t bytes_read) 
{
        unsigned int ch;
        unsigned int code, mycode, press, ascii;
        int map;
        struct kbentry kbe;

        /* Process all keys read from left to right */
        for (ch = 0; ch < bytes_read; ch++) {
                /* Read kernel's fake scancode */
                code = buf[ch] & 0x7f;
                press = !(buf[ch] & 0x80);

                /* Convert to Allegro's fake scancode */
                mycode = kernel_to_mycode[code];

                /* Process modifiers */
                if (mycode >= KEY_MODIFIERS) {
                        int flag = modifier_table[mycode - KEY_MODIFIERS];
                        if (press) {
                                if (flag & KB_MODIFIERS)
                                        _key_shifts |= flag;
                                else if ((flag & KB_LED_FLAGS) && key_led_flag)
                                        _key_shifts ^= flag;
                                _handle_key_press (-1, mycode);
                        } else {
                                if (flag & KB_MODIFIERS)
                                        _key_shifts &= ~flag;
                                _handle_key_release (mycode);
                        }
                        continue;
                }

                /* Releases only need the scancode */
                if (!press) {
                        _handle_key_release (mycode);
                        continue;
                }

                /* Build kernel keymap number */
                map = 0;
                if (key[KEY_LSHIFT] || key[KEY_RSHIFT]) map |= 1;
                if (key[KEY_ALTGR]) map |= 2;
                if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) map |= 4;
                if (key[KEY_ALT]) map |= 8;

                /* Map scancode to type and value */
                kbe.kb_table = map;
                kbe.kb_index = code;
                ioctl (std_keyboard.fd, KDGKBENT, &kbe);

		/* debugging */
		TRACE("keypress: code=%3d mycode=%3d map=%2d kb_value=%04x\n", code, mycode, map, kbe.kb_value);

                /* Allegro wants Alt+key to return ASCII code zero */
                if (key[KEY_ALT])
                        ascii = 0;
		else
		/* Otherwise fill it in according to the key pressed */
		     switch (mycode) {
			case KEY_BACKSPACE: ascii = 8; break;
			default:
				/* Most keys come here */
				ascii = KVAL(kbe.kb_value);
                		/* Handle invalid keymaps */
		                /* FIXME: Maybe we should just redo the
				 *        ioctl with keymap 0? */
				if (kbe.kb_value == K_NOSUCHMAP)
					ascii = 0;
		}

                /* Finally do some things based on key type */
                switch (KTYP(kbe.kb_value)) {
                        case KT_CONS:
                                if (!ioctl (__al_linux_console_fd, VT_ACTIVATE, KVAL(kbe.kb_value) + 1)) {
                                        _handle_key_press (-1, mycode);
                                        continue;
                                }
                                ascii = 0;
                                break;
                        case KT_LETTER:
                                /* Apply capslock translation */
                                if (_key_shifts & KB_CAPSLOCK_FLAG)
                                        ascii ^= 0x20;
                                break;
                        case KT_LATIN:
                        case KT_ASCII:
                                break;
                        case KT_PAD:
                                ascii = -1;
                                if (_key_shifts & KB_NUMLOCK_FLAG) {
                                        int val = KVAL(kbe.kb_value);
                                        if ((val >= 0) && (val < NUM_PAD_KEYS))
                                                ascii = pad_asciis[val];
                                } else {
					int val = KVAL(kbe.kb_value);
					if ((val >= 0) && (val < NUM_PAD_KEYS) && pad_arrow_codes[val]) {
					    	ascii = pad_asciis_no_numlock[val];
						mycode = pad_arrow_codes[val];
					}
                                }
                                break;
                        case KT_SPEC:
                                if (mycode == KEY_ENTER) {
                                        ascii = '\r';
                                        break;
                                }
                        case KT_FN:
                        case KT_CUR:
                                ascii = -1;
                                break;
                        default:
                                /* Don't put anything in the buffer */
                                _handle_key_press (-1, mycode);
                }

                /* Replace -1 with the modifier state */
                if (ascii == -1)
                        ascii = _key_shifts & KB_MODIFIERS;

                /* three-finger salute for killing the program */
                if (((mycode == KEY_DEL) || (mycode == KEY_END)) && 
                    (three_finger_flag) &&
                    (_key_shifts & KB_CTRL_FLAG) && 
                    (_key_shifts & KB_ALT_FLAG))
                        kill(main_pid, SIGTERM);

                _handle_key_press (ascii, mycode);
        }
}

/* update_keyboard:
 *  Read keys from keyboard, if there are any, and update all relevant
 *  variables.
 *
 *  We take full advantage of the kernel keyboard driver while still working
 *  in the MEDIUM_RAW mode.  This allows us to both keep track of the up/down
 *  status of any key and return character codes from the kernel keymaps.
 */
static int update_keyboard (void)
{
   unsigned char buf[128];
   int bytes_read;
   fd_set set;
   struct timeval tv = { 0, 0 };

   if (resume_count <= 0)
      return 0;

   FD_ZERO(&set);
   FD_SET(std_keyboard.fd, &set);
   if (select (FD_SETSIZE, &set, NULL, NULL, &tv) <= 0)
      return 0;

   bytes_read = read(std_keyboard.fd, &buf, sizeof(buf));
   if (bytes_read < 1)
      return 0;

   process_keyboard_data(buf, bytes_read);

   return 1;
}


/* activate_keyboard:
 *  Put keyboard into the mode we want.
 */
static void activate_keyboard (void)
{
        tcsetattr (std_keyboard.fd, TCSANOW, &__al_linux_work_termio);
        ioctl (std_keyboard.fd, KDSKBMODE, K_MEDIUMRAW);
}

/* deactivate_keyboard:
 *  Restore the original keyboard settings.
 */
static void deactivate_keyboard (void)
{
        tcsetattr (std_keyboard.fd, TCSANOW, &__al_linux_startup_termio);
        ioctl (std_keyboard.fd, KDSKBMODE, startup_kbmode);
}


/* release_all_keys:
 *  Sends release messages for all keys that are marked as pressed in the
 *  key array.  Maybe this should be a global function in `src/keyboard.c';
 *  it could be used by other platforms, and could work better with the
 *  polled driver emulation system.
 */
static void release_all_keys (void)
{
        int i;
        for (i = 0; i < KEY_MAX; i++)
                if (key[i]) _handle_key_release (i);
}

/* suspend_keyboard:
 *  Suspend our keyboard handling, restoring the original OS values, so that
 *  the application can take advantage of Linux keyboard handling.
 */
static void suspend_keyboard (void)
{
        resume_count--;
        if (resume_count == 0)
                deactivate_keyboard();
        release_all_keys();
}

/* resume_keyboard:
 *  Start/resume Allegro keyboard handling.
 */
static void resume_keyboard()
{
        if (resume_count == 0)
                activate_keyboard();
        resume_count++;
}


static int linux_key_init (void)
{
        std_keyboard.fd = dup (__al_linux_console_fd);

        /* Set terminal attributes we need */
        __al_linux_work_termio.c_iflag = 0;
        __al_linux_work_termio.c_cflag = CS8;
        __al_linux_work_termio.c_lflag = 0;

        /* Prevent `read' from blocking */
        __al_linux_work_termio.c_cc[VMIN] = 0;
        __al_linux_work_termio.c_cc[VTIME] = 0;

        /* Save previous keyboard mode (probably XLATE) */
        ioctl (std_keyboard.fd, KDGKBMODE, &startup_kbmode);

        /* Keyboard is disabled */
        resume_count = 0;

        /* Get the pid, which we use for the three finger salute */
        main_pid = getpid();

        /* Register our driver (enables it too) */
        __al_linux_add_standard_driver (&std_keyboard);

        return 0;
}

static void linux_key_exit (void)
{
	/* Reset LED mode before exiting */
	ioctl(std_keyboard.fd, KDSETLED, 8);

        __al_linux_remove_standard_driver (&std_keyboard);
        close (std_keyboard.fd);
}

static void linux_set_leds (int leds)
{
	int val = 0;
	if (leds & KB_SCROLOCK_FLAG) val |= LED_SCR;
	if (leds & KB_NUMLOCK_FLAG) val |= LED_NUM;
	if (leds & KB_CAPSLOCK_FLAG) val |= LED_CAP;
        ioctl(std_keyboard.fd, KDSETLED, val);
}

static void linux_set_rate (int delay, int rate)
{
}

static int linux_scancode_to_ascii (int scancode)
{
        int kernel_code;
        struct kbentry kbe;

        for (kernel_code = 0; kernel_code < 128; kernel_code++)
                if (scancode == kernel_to_mycode[kernel_code]) break;
        if (kernel_code == 128) return 0;

        kbe.kb_table = 0;
        kbe.kb_index = kernel_code;
        ioctl (std_keyboard.fd, KDGKBENT, &kbe);
        
        switch (KTYP(kbe.kb_value)) {
                case KT_LETTER:
                case KT_LATIN:
                case KT_ASCII:
                        return KVAL(kbe.kb_value);
                case KT_SPEC:
                        if (scancode == KEY_ENTER)
                                return '\r';
                        break;
        }
        return 0;
}

