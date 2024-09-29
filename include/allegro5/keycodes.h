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
 *      Keycode constants.
 *
 *      See readme.txt for copyright information.
 */


#ifndef __al_included_allegro5_keycodes_h
#define __al_included_allegro5_keycodes_h



/* Note these values are deliberately the same as in Allegro 4.1.x */
enum
{
   A5O_KEY_A		= 1,
   A5O_KEY_B		= 2,
   A5O_KEY_C		= 3,
   A5O_KEY_D		= 4,
   A5O_KEY_E		= 5,
   A5O_KEY_F		= 6,
   A5O_KEY_G		= 7,
   A5O_KEY_H		= 8,
   A5O_KEY_I		= 9,
   A5O_KEY_J		= 10,
   A5O_KEY_K		= 11,
   A5O_KEY_L		= 12,
   A5O_KEY_M		= 13,
   A5O_KEY_N		= 14,
   A5O_KEY_O		= 15,
   A5O_KEY_P		= 16,
   A5O_KEY_Q		= 17,
   A5O_KEY_R		= 18,
   A5O_KEY_S		= 19,
   A5O_KEY_T		= 20,
   A5O_KEY_U		= 21,
   A5O_KEY_V		= 22,
   A5O_KEY_W		= 23,
   A5O_KEY_X		= 24,
   A5O_KEY_Y		= 25,
   A5O_KEY_Z		= 26,

   A5O_KEY_0		= 27,
   A5O_KEY_1		= 28,
   A5O_KEY_2		= 29,
   A5O_KEY_3		= 30,
   A5O_KEY_4		= 31,
   A5O_KEY_5		= 32,
   A5O_KEY_6		= 33,
   A5O_KEY_7		= 34,
   A5O_KEY_8		= 35,
   A5O_KEY_9		= 36,

   A5O_KEY_PAD_0		= 37,
   A5O_KEY_PAD_1		= 38,
   A5O_KEY_PAD_2		= 39,
   A5O_KEY_PAD_3		= 40,
   A5O_KEY_PAD_4		= 41,
   A5O_KEY_PAD_5		= 42,
   A5O_KEY_PAD_6		= 43,
   A5O_KEY_PAD_7		= 44,
   A5O_KEY_PAD_8		= 45,
   A5O_KEY_PAD_9		= 46,

   A5O_KEY_F1		= 47,
   A5O_KEY_F2		= 48,
   A5O_KEY_F3		= 49,
   A5O_KEY_F4		= 50,
   A5O_KEY_F5		= 51,
   A5O_KEY_F6		= 52,
   A5O_KEY_F7		= 53,
   A5O_KEY_F8		= 54,
   A5O_KEY_F9		= 55,
   A5O_KEY_F10		= 56,
   A5O_KEY_F11		= 57,
   A5O_KEY_F12		= 58,

   A5O_KEY_ESCAPE	= 59,
   A5O_KEY_TILDE		= 60,
   A5O_KEY_MINUS		= 61,
   A5O_KEY_EQUALS	= 62,
   A5O_KEY_BACKSPACE	= 63,
   A5O_KEY_TAB		= 64,
   A5O_KEY_OPENBRACE	= 65,
   A5O_KEY_CLOSEBRACE	= 66,
   A5O_KEY_ENTER		= 67,
   A5O_KEY_SEMICOLON	= 68,
   A5O_KEY_QUOTE		= 69,
   A5O_KEY_BACKSLASH	= 70,
   A5O_KEY_BACKSLASH2	= 71, /* DirectInput calls this DIK_OEM_102: "< > | on UK/Germany keyboards" */
   A5O_KEY_COMMA		= 72,
   A5O_KEY_FULLSTOP	= 73,
   A5O_KEY_SLASH		= 74,
   A5O_KEY_SPACE		= 75,

   A5O_KEY_INSERT	= 76,
   A5O_KEY_DELETE	= 77,
   A5O_KEY_HOME		= 78,
   A5O_KEY_END		= 79,
   A5O_KEY_PGUP		= 80,
   A5O_KEY_PGDN		= 81,
   A5O_KEY_LEFT		= 82,
   A5O_KEY_RIGHT		= 83,
   A5O_KEY_UP		= 84,
   A5O_KEY_DOWN		= 85,

   A5O_KEY_PAD_SLASH	= 86,
   A5O_KEY_PAD_ASTERISK	= 87,
   A5O_KEY_PAD_MINUS	= 88,
   A5O_KEY_PAD_PLUS	= 89,
   A5O_KEY_PAD_DELETE	= 90,
   A5O_KEY_PAD_ENTER	= 91,

   A5O_KEY_PRINTSCREEN	= 92,
   A5O_KEY_PAUSE		= 93,

   A5O_KEY_ABNT_C1	= 94,
   A5O_KEY_YEN		= 95,
   A5O_KEY_KANA		= 96,
   A5O_KEY_CONVERT	= 97,
   A5O_KEY_NOCONVERT	= 98,
   A5O_KEY_AT		= 99,
   A5O_KEY_CIRCUMFLEX	= 100,
   A5O_KEY_COLON2	= 101,
   A5O_KEY_KANJI		= 102,

   A5O_KEY_PAD_EQUALS	= 103,	/* MacOS X */
   A5O_KEY_BACKQUOTE	= 104,	/* MacOS X */
   A5O_KEY_SEMICOLON2	= 105,	/* MacOS X -- TODO: ask lillo what this should be */
   A5O_KEY_COMMAND	= 106,	/* MacOS X */
   
   A5O_KEY_BACK = 107,        /* Android back key */
   A5O_KEY_VOLUME_UP = 108,
   A5O_KEY_VOLUME_DOWN = 109,

   /* Android game keys */
   A5O_KEY_SEARCH       = 110,
   A5O_KEY_DPAD_CENTER  = 111,
   A5O_KEY_BUTTON_X     = 112,
   A5O_KEY_BUTTON_Y     = 113,
   A5O_KEY_DPAD_UP      = 114,
   A5O_KEY_DPAD_DOWN    = 115,
   A5O_KEY_DPAD_LEFT    = 116,
   A5O_KEY_DPAD_RIGHT   = 117,
   A5O_KEY_SELECT       = 118,
   A5O_KEY_START        = 119,
   A5O_KEY_BUTTON_L1    = 120,
   A5O_KEY_BUTTON_R1    = 121,
   A5O_KEY_BUTTON_L2    = 122,
   A5O_KEY_BUTTON_R2    = 123,
   A5O_KEY_BUTTON_A     = 124,
   A5O_KEY_BUTTON_B     = 125,
   A5O_KEY_THUMBL       = 126,
   A5O_KEY_THUMBR       = 127,
   
   A5O_KEY_UNKNOWN      = 128,

   /* All codes up to before A5O_KEY_MODIFIERS can be freely
    * assignedas additional unknown keys, like various multimedia
    * and application keys keyboards may have.
    */

   A5O_KEY_MODIFIERS	= 215,

   A5O_KEY_LSHIFT	= 215,
   A5O_KEY_RSHIFT	= 216,
   A5O_KEY_LCTRL	= 217,
   A5O_KEY_RCTRL	= 218,
   A5O_KEY_ALT		= 219,
   A5O_KEY_ALTGR	= 220,
   A5O_KEY_LWIN		= 221,
   A5O_KEY_RWIN		= 222,
   A5O_KEY_MENU		= 223,
   A5O_KEY_SCROLLLOCK = 224,
   A5O_KEY_NUMLOCK	= 225,
   A5O_KEY_CAPSLOCK	= 226,

   A5O_KEY_MAX
};



enum
{
   A5O_KEYMOD_SHIFT       = 0x00001,
   A5O_KEYMOD_CTRL        = 0x00002,
   A5O_KEYMOD_ALT         = 0x00004,
   A5O_KEYMOD_LWIN        = 0x00008,
   A5O_KEYMOD_RWIN        = 0x00010,
   A5O_KEYMOD_MENU        = 0x00020,
   A5O_KEYMOD_ALTGR       = 0x00040,
   A5O_KEYMOD_COMMAND     = 0x00080,
   A5O_KEYMOD_SCROLLLOCK  = 0x00100,
   A5O_KEYMOD_NUMLOCK     = 0x00200,
   A5O_KEYMOD_CAPSLOCK    = 0x00400,
   A5O_KEYMOD_INALTSEQ	 = 0x00800,
   A5O_KEYMOD_ACCENT1     = 0x01000,
   A5O_KEYMOD_ACCENT2     = 0x02000,
   A5O_KEYMOD_ACCENT3     = 0x04000,
   A5O_KEYMOD_ACCENT4     = 0x08000
};



#endif
