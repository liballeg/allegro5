/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to access the keyboard. The
 *    first part shows the basic use of readkey(). The second part
 *    shows how to extract the ASCII value. Next come the scan codes.
 *    The fourth test detects modifier keys like alt or shift. The
 *    fifth test requires some focus to be passed. The final step
 *    shows how to use the global key array to read simultaneous
 *    key presses.
 *    The last method to detect key presses are keyboard callbacks.
 *    This is demonstrated by installing a keyboard callback,
 *    which marks all pressed keys by drawing to a grid.
 */


#include <stdio.h>
#include <string.h>

#include <allegro.h>



char *key_names[] =
{
   "(none)",         "KEY_A",          "KEY_B",          "KEY_C",
   "KEY_D",          "KEY_E",          "KEY_F",          "KEY_G",
   "KEY_H",          "KEY_I",          "KEY_J",          "KEY_K",
   "KEY_L",          "KEY_M",          "KEY_N",          "KEY_O",
   "KEY_P",          "KEY_Q",          "KEY_R",          "KEY_S",
   "KEY_T",          "KEY_U",          "KEY_V",          "KEY_W",
   "KEY_X",          "KEY_Y",          "KEY_Z",          "KEY_0",
   "KEY_1",          "KEY_2",          "KEY_3",          "KEY_4",
   "KEY_5",          "KEY_6",          "KEY_7",          "KEY_8",
   "KEY_9",          "KEY_0_PAD",      "KEY_1_PAD",      "KEY_2_PAD",
   "KEY_3_PAD",      "KEY_4_PAD",      "KEY_5_PAD",      "KEY_6_PAD",
   "KEY_7_PAD",      "KEY_8_PAD",      "KEY_9_PAD",      "KEY_F1",
   "KEY_F2",         "KEY_F3",         "KEY_F4",         "KEY_F5",
   "KEY_F6",         "KEY_F7",         "KEY_F8",         "KEY_F9",
   "KEY_F10",        "KEY_F11",        "KEY_F12",        "KEY_ESC",
   "KEY_TILDE",      "KEY_MINUS",      "KEY_EQUALS",     "KEY_BACKSPACE",
   "KEY_TAB",        "KEY_OPENBRACE",  "KEY_CLOSEBRACE", "KEY_ENTER",
   "KEY_COLON",      "KEY_QUOTE",      "KEY_BACKSLASH",  "KEY_BACKSLASH2",
   "KEY_COMMA",      "KEY_STOP",       "KEY_SLASH",      "KEY_SPACE",
   "KEY_INSERT",     "KEY_DEL",        "KEY_HOME",       "KEY_END",
   "KEY_PGUP",       "KEY_PGDN",       "KEY_LEFT",       "KEY_RIGHT",
   "KEY_UP",         "KEY_DOWN",       "KEY_SLASH_PAD",  "KEY_ASTERISK",
   "KEY_MINUS_PAD",  "KEY_PLUS_PAD",   "KEY_DEL_PAD",    "KEY_ENTER_PAD",
   "KEY_PRTSCR",     "KEY_PAUSE",      "KEY_ABNT_C1",    "KEY_YEN",
   "KEY_KANA",       "KEY_CONVERT",    "KEY_NOCONVERT",  "KEY_AT",
   "KEY_CIRCUMFLEX", "KEY_COLON2",     "KEY_KANJI",      "KEY_EQUALS_PAD",
   "KEY_BACKQUOTE",  "KEY_SEMICOLON",  "KEY_COMMAND",    "KEY_UNKNOWN1",
   "KEY_UNKNOWN2",   "KEY_UNKNOWN3",   "KEY_UNKNOWN4",   "KEY_UNKNOWN5",
   "KEY_UNKNOWN6",   "KEY_UNKNOWN7",   "KEY_UNKNOWN8",   "KEY_LSHIFT",
   "KEY_RSHIFT",     "KEY_LCONTROL",   "KEY_RCONTROL",   "KEY_ALT",
   "KEY_ALTGR",      "KEY_LWIN",       "KEY_RWIN",       "KEY_MENU",
   "KEY_SCRLOCK",    "KEY_NUMLOCK",    "KEY_CAPSLOCK",   "KEY_MAX"
};



/* Keyboard callback. We are very evil and draw to the screen from within
 * the callback. Don't do this in your own programs ;)
 */
void keypress_handler(int scancode)
{
   int i, x, y, color;
   char str[64];

   i = scancode & 0x7f;
   x = SCREEN_W - 100 * 3 + (i % 3) * 100;
   y = SCREEN_H / 2 + (i / 3 - 21) * 10;
   color = scancode & 0x80 ? makecol(255, 255, 0) : makecol(128, 0, 0);
   rectfill(screen, x, y, x + 95, y + 8, color);
   ustrzncpy(str, sizeof(str), scancode_to_name(i), 12);
   textout_ex(screen, font, str, x + 1, y + 1, makecol(0, 0, 0), -1);
}
END_OF_FUNCTION(keypress_handler)



/* helper function for making more room on the screen */
void scroll(void)
{
   blit(screen, screen, 0, 32, 0, 24, SCREEN_W / 2, SCREEN_H-32);
   rectfill(screen, 0, SCREEN_H-16, SCREEN_W / 2, SCREEN_H-1, makecol(255, 255, 255));
}



int main(void)
{
   char buf[128];
   int k;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);

   clear_to_color(screen, makecol(255, 255, 255));

   /* Draw the initial keys grid by simulating release of every key. */
   for (k = 0; k < KEY_MAX; k++)
      keypress_handler (k + 0x80);

   /* Install our keyboard callback. */
   LOCK_FUNCTION(keypress_handler);
   keyboard_lowlevel_callback = keypress_handler;

   acquire_screen();
   textprintf_centre_ex(screen, font, SCREEN_W/2, 8, makecol(0, 0, 0), makecol(255, 255, 255),
			"Driver: %s", keyboard_driver->name);

   /* keyboard input can be accessed with the readkey() function */
   textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
	         "Press some keys (ESC to finish)");
   scroll();

   do {
      release_screen();
      k = readkey();
      acquire_screen();
      scroll();
      textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		    "readkey() returned %-6d (0x%04X)", k, k);
   } while ((k & 0xFF) != 27);

   /* the ASCII code is in the low byte of the return value */
   scroll(); scroll(); scroll();
   textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		 "Press some more keys (ESC to finish)");
   scroll();

   do {
      release_screen();
      k = readkey();
      acquire_screen();
      scroll();
      textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		    "ASCII code is %d", k&0xFF);
   } while ((k&0xFF) != 27);

   /* the hardware scan code is in the high byte of the return value */
   scroll(); scroll(); scroll();
   textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		 "Press some more keys (ESC to finish)");
   scroll();

   do {
      release_screen();
      k = readkey();
      acquire_screen();
      scroll();
      textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		    "Scan code is %d (%s)", k>>8, key_names[k>>8]);
   } while ((k&0xFF) != 27);

   /* key qualifiers are stored in the key_shifts variable. Note that this
    * version of the code uses ureadkey() instead of readkey(): that is
    * necessary if you want to access Unicode characters from outside
    * the normal ASCII range, for example to support Russian or Chinese.
    */
   scroll(); scroll(); scroll();
   textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		 "Press some more keys (ESC to finish)");
   scroll();

   do {
      release_screen();
      k = ureadkey(NULL);
      acquire_screen();
      buf[0] = 0;
      if (key_shifts & KB_SHIFT_FLAG)   strcat(buf, "shift ");
      if (key_shifts & KB_CTRL_FLAG)    strcat(buf, "ctrl ");
      if (key_shifts & KB_ALT_FLAG)     strcat(buf, "alt ");
      if (key_shifts & KB_LWIN_FLAG)    strcat(buf, "lwin ");
      if (key_shifts & KB_RWIN_FLAG)    strcat(buf, "rwin ");
      if (key_shifts & KB_MENU_FLAG)    strcat(buf, "menu ");
      if (key_shifts & KB_COMMAND_FLAG) strcat(buf, "command ");
      usprintf(buf+strlen(buf), "'%c' [0x%02x]", k ? k : ' ', k);
      if (key_shifts & KB_CAPSLOCK_FLAG) strcat(buf, " caps");
      if (key_shifts & KB_NUMLOCK_FLAG)  strcat(buf, " num");
      if (key_shifts & KB_SCROLOCK_FLAG) strcat(buf, " scrl");
      scroll();
      textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255), "%s", buf);
   } while (k != 27);

   /* various scan codes are defined in allegro.h as KEY_* constants */
   scroll(); scroll(); scroll();
   textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255), "Press F6");
   scroll();

   release_screen();
   k = readkey();
   acquire_screen();

   while ((k>>8) != KEY_F6 && (k>>8) != KEY_ESC) {
      scroll();
      textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		    "You pressed the wrong key. Press F6 instead.");
      release_screen();
      k = readkey();
      acquire_screen();
   }

   /* for detecting multiple simultaneous key presses, use the key[] array */
   scroll(); scroll(); scroll();
   textprintf_ex(screen, font, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255),
		 "Press a combination of numbers");
   scroll(); scroll();

   release_screen();

   do {
      if (key[KEY_0]) buf[0] = '0'; else buf[0] = ' ';
      if (key[KEY_1]) buf[1] = '1'; else buf[1] = ' ';
      if (key[KEY_2]) buf[2] = '2'; else buf[2] = ' ';
      if (key[KEY_3]) buf[3] = '3'; else buf[3] = ' ';
      if (key[KEY_4]) buf[4] = '4'; else buf[4] = ' ';
      if (key[KEY_5]) buf[5] = '5'; else buf[5] = ' ';
      if (key[KEY_6]) buf[6] = '6'; else buf[6] = ' ';
      if (key[KEY_7]) buf[7] = '7'; else buf[7] = ' ';
      if (key[KEY_8]) buf[8] = '8'; else buf[8] = ' ';
      if (key[KEY_9]) buf[9] = '9'; else buf[9] = ' ';
      buf[10] = 0;
      textout_ex(screen, font, buf, 8, SCREEN_H-16, makecol(0, 0, 0), makecol(255, 255, 255));
      rest(1);
   } while (!keypressed() || (readkey() >> 8) != KEY_ESC);

   clear_keybuf();
   keyboard_lowlevel_callback = NULL;
   return 0;
}

END_OF_MAIN()
