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
 *      Unix keyboard module.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"


static int _xwin_keydrv_init(void);
static void _xwin_keydrv_exit(void);
static void _xwin_keydrv_set_leds(int leds);
static void _xwin_keydrv_set_rate(int delay, int rate);


static KEYBOARD_DRIVER keyboard_xwin =
{
   KEYBOARD_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows keyboard",
   TRUE,
   _xwin_keydrv_init,
   _xwin_keydrv_exit,
   NULL,
   _xwin_keydrv_set_leds,
   _xwin_keydrv_set_rate,
   NULL, NULL,
   _pckey_scancode_to_ascii
};



/* list the available drivers */
_DRIVER_INFO _xwin_keyboard_driver_list[] =
{
   {  KEYBOARD_XWINDOWS, &keyboard_xwin, TRUE  },
   {  0,                 NULL,           0     }
};



/* the pid to kill when three finger saluting */
static int main_pid;



/* _xwin_keydrv_set_leds:
 *  Updates the LED state.
 */
static void _xwin_keydrv_set_leds(int leds)
{
   _xwin_change_keyboard_control(1, leds & KB_NUMLOCK_FLAG);
   _xwin_change_keyboard_control(2, leds & KB_CAPSLOCK_FLAG);
   _xwin_change_keyboard_control(3, leds & KB_SCROLOCK_FLAG);
}



/* _xwin_keydrv_set_rate:
 *  Sets the key repeat rate.
 */
static void _xwin_keydrv_set_rate(int delay, int rate)
{
}



/* _xwin_keydrv_handler:
 *  Keyboard "interrupt" handler.
 */
static void _xwin_keydrv_handler(int pressed, int code)
{
   int scancode;

   /* Handle special scancodes.  */
   if (code & 0x100) {
      switch (code & 0x7F) {
	 case 0:
	    /* Pause.  */
	    if (pressed) {
	       /* Only keypress is interesting.  */
	       _handle_pckey(0xE1); _handle_pckey(0x1D); _handle_pckey(0x52);
	       _handle_pckey(0xE1); _handle_pckey(0x9D); _handle_pckey(0xD2);
	    }
	    break;
      }
      return;
   }

   /* Handle extended flag.  */
   if (code & 0x80)
      _handle_pckey(0xE0);

   /* Handle scancode.  */
   scancode = (code & 0x7F) | (pressed ? 0x00 : 0x80);
   _handle_pckey(scancode);

   /* Exit by Ctrl-Alt-End.  */
   if (((scancode == 0x4F) || (scancode == 0x53)) && three_finger_flag
       && (_key_shifts & KB_CTRL_FLAG) && (_key_shifts & KB_ALT_FLAG)) {
#ifndef HAVE_LIBPTHREAD
      if (_unix_bg_man == &_bg_man_sigalrm)
         _sigalrm_request_abort();
      else
#endif
         kill(main_pid, SIGTERM);
   }
}



/* _xwin_keydrv_focused:
 *  Keyboard focus change handler.
 */
static void _xwin_keydrv_focused(int focused, int state)
{
   int i, mask;

   if (focused) {
      mask = KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG;
      _key_shifts = (_key_shifts & ~mask) | (state & mask);
   }
   else {
      for (i=0; i<KEY_MAX; i++) {
	 if (key[i])
	    _handle_key_release(i);
      }
   }
}



/* _xwin_keydrv_init:
 *  Installs the keyboard handler.
 */
static int _xwin_keydrv_init(void)
{
   _pckeys_init();

   _xwin_init_keyboard_tables();
   _xwin_keydrv_set_leds(_key_shifts);

   XLOCK();

   _xwin_keyboard_interrupt = _xwin_keydrv_handler;
   _xwin_keyboard_focused = _xwin_keydrv_focused;

   XUNLOCK();

   main_pid = getpid();

   return 0;
}



/* _xwin_keydrv_exit:
 *  Removes the keyboard handler.
 */
static void _xwin_keydrv_exit(void)
{
   XLOCK();

   _xwin_keyboard_interrupt = 0;
   _xwin_keyboard_focused = 0;

   XUNLOCK();
}

