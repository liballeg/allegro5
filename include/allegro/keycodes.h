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
 *      Keyboard routines.
 *
 *      See readme.txt for copyright information.
 */


/* Title: Keyboard constants
 */

#ifndef _al_included_keycodes_h
#define _al_included_keycodes_h



/* Type: Key codes
 *
 * > AL_KEY_A ... AL_KEY_Z,
 * > AL_KEY_0 ... AL_KEY_9,
 * > AL_KEY_PAD_0 ... AL_KEY_PAD_9,
 * > AL_KEY_F1 ... AL_KEY_F12,
 * > AL_KEY_ESCAPE, AL_KEY_TILDE, AL_KEY_MINUS, AL_KEY_EQUALS,
 * > AL_KEY_BACKSPACE, AL_KEY_TAB, AL_KEY_OPENBRACE, AL_KEY_CLOSEBRACE,
 * > AL_KEY_ENTER, AL_KEY_SEMICOLON, AL_KEY_QUOTE, AL_KEY_BACKSLASH,
 * > AL_KEY_BACKSLASH2, AL_KEY_COMMA, AL_KEY_FULLSTOP, AL_KEY_SLASH,
 * > AL_KEY_SPACE,
 * > AL_KEY_INSERT, AL_KEY_DELETE, AL_KEY_HOME, AL_KEY_END, AL_KEY_PGUP,
 * > AL_KEY_PGDN, AL_KEY_LEFT, AL_KEY_RIGHT, AL_KEY_UP, AL_KEY_DOWN,
 * > AL_KEY_PAD_SLASH, AL_KEY_PAD_ASTERISK, AL_KEY_PAD_MINUS,
 * > AL_KEY_PAD_PLUS, AL_KEY_PAD_DELETE, AL_KEY_PAD_ENTER,
 * > AL_KEY_PRINTSCREEN, AL_KEY_PAUSE,
 * > AL_KEY_ABNT_C1, AL_KEY_YEN, AL_KEY_KANA, AL_KEY_CONVERT, AL_KEY_NOCONVERT,
 * > AL_KEY_AT, AL_KEY_CIRCUMFLEX, AL_KEY_COLON2, AL_KEY_KANJI,
 * > AL_KEY_LSHIFT, AL_KEY_RSHIFT,
 * > AL_KEY_LCTRL, AL_KEY_RCTRL,
 * > AL_KEY_ALT, AL_KEY_ALTGR,
 * > AL_KEY_LWIN, AL_KEY_RWIN, AL_KEY_MENU,
 * > AL_KEY_SCROLLLOCK, AL_KEY_NUMLOCK, AL_KEY_CAPSLOCK
 * > AL_KEY_EQUALS_PAD, AL_KEY_BACKQUOTE, AL_KEY_SEMICOLON2, AL_KEY_COMMAND
 */
/* Note these values are deliberately the same as in Allegro 4.1.x */
enum
{
   AL_KEY_A		= 1,
   AL_KEY_B		= 2,
   AL_KEY_C		= 3,
   AL_KEY_D		= 4,
   AL_KEY_E		= 5,
   AL_KEY_F		= 6,
   AL_KEY_G		= 7,
   AL_KEY_H		= 8,
   AL_KEY_I		= 9,
   AL_KEY_J		= 10,
   AL_KEY_K		= 11,
   AL_KEY_L		= 12,
   AL_KEY_M		= 13,
   AL_KEY_N		= 14,
   AL_KEY_O		= 15,
   AL_KEY_P		= 16,
   AL_KEY_Q		= 17,
   AL_KEY_R		= 18,
   AL_KEY_S		= 19,
   AL_KEY_T		= 20,
   AL_KEY_U		= 21,
   AL_KEY_V		= 22,
   AL_KEY_W		= 23,
   AL_KEY_X		= 24,
   AL_KEY_Y		= 25,
   AL_KEY_Z		= 26,

   AL_KEY_0		= 27,
   AL_KEY_1		= 28,
   AL_KEY_2		= 29,
   AL_KEY_3		= 30,
   AL_KEY_4		= 31,
   AL_KEY_5		= 32,
   AL_KEY_6		= 33,
   AL_KEY_7		= 34,
   AL_KEY_8		= 35,
   AL_KEY_9		= 36,

   AL_KEY_PAD_0		= 37,
   AL_KEY_PAD_1		= 38,
   AL_KEY_PAD_2		= 39,
   AL_KEY_PAD_3		= 40,
   AL_KEY_PAD_4		= 41,
   AL_KEY_PAD_5		= 42,
   AL_KEY_PAD_6		= 43,
   AL_KEY_PAD_7		= 44,
   AL_KEY_PAD_8		= 45,
   AL_KEY_PAD_9		= 46,

   AL_KEY_F1		= 47,
   AL_KEY_F2		= 48,
   AL_KEY_F3		= 49,
   AL_KEY_F4		= 50,
   AL_KEY_F5		= 51,
   AL_KEY_F6		= 52,
   AL_KEY_F7		= 53,
   AL_KEY_F8		= 54,
   AL_KEY_F9		= 55,
   AL_KEY_F10		= 56,
   AL_KEY_F11		= 57,
   AL_KEY_F12		= 58,

   AL_KEY_ESCAPE	= 59,
   AL_KEY_TILDE		= 60,
   AL_KEY_MINUS		= 61,
   AL_KEY_EQUALS	= 62,
   AL_KEY_BACKSPACE	= 63,
   AL_KEY_TAB		= 64,
   AL_KEY_OPENBRACE	= 65,
   AL_KEY_CLOSEBRACE	= 66,
   AL_KEY_ENTER		= 67,
   AL_KEY_SEMICOLON	= 68,
   AL_KEY_QUOTE		= 69,
   AL_KEY_BACKSLASH	= 70,
   AL_KEY_BACKSLASH2	= 71, /* DirectInput calls this DIK_OEM_102: "< > | on UK/Germany keyboards" */
   AL_KEY_COMMA		= 72,
   AL_KEY_FULLSTOP	= 73,
   AL_KEY_SLASH		= 74,
   AL_KEY_SPACE		= 75,

   AL_KEY_INSERT	= 76,
   AL_KEY_DELETE	= 77,
   AL_KEY_HOME		= 78,
   AL_KEY_END		= 79,
   AL_KEY_PGUP		= 80,
   AL_KEY_PGDN		= 81,
   AL_KEY_LEFT		= 82,
   AL_KEY_RIGHT		= 83,
   AL_KEY_UP		= 84,
   AL_KEY_DOWN		= 85,

   AL_KEY_PAD_SLASH	= 86,
   AL_KEY_PAD_ASTERISK	= 87,
   AL_KEY_PAD_MINUS	= 88,
   AL_KEY_PAD_PLUS	= 89,
   AL_KEY_PAD_DELETE	= 90,
   AL_KEY_PAD_ENTER	= 91,

   AL_KEY_PRINTSCREEN	= 92,
   AL_KEY_PAUSE		= 93,

   AL_KEY_ABNT_C1	= 94,
   AL_KEY_YEN		= 95,
   AL_KEY_KANA		= 96,
   AL_KEY_CONVERT	= 97,
   AL_KEY_NOCONVERT	= 98,
   AL_KEY_AT		= 99,
   AL_KEY_CIRCUMFLEX	= 100,
   AL_KEY_COLON2	= 101,
   AL_KEY_KANJI		= 102,

   AL_KEY_EQUALS_PAD	= 103,	/* MacOS X */
   AL_KEY_BACKQUOTE	= 104,	/* MacOS X */
   AL_KEY_SEMICOLON2	= 105,	/* MacOS X -- TODO: ask lillo what this should be */
   AL_KEY_COMMAND	= 106,	/* MacOS X */
   AL_KEY_UNKNOWN1      = 107,
   AL_KEY_UNKNOWN2      = 108,
   AL_KEY_UNKNOWN3      = 109,
   AL_KEY_UNKNOWN4      = 110,
   AL_KEY_UNKNOWN5      = 111,
   AL_KEY_UNKNOWN6      = 112,
   AL_KEY_UNKNOWN7      = 113,
   AL_KEY_UNKNOWN8      = 114,

   AL_KEY_MODIFIERS	= 115,

   AL_KEY_LSHIFT	= 115,
   AL_KEY_RSHIFT	= 116,
   AL_KEY_LCTRL		= 117,
   AL_KEY_RCTRL		= 118,
   AL_KEY_ALT		= 119,
   AL_KEY_ALTGR		= 120,
   AL_KEY_LWIN		= 121,
   AL_KEY_RWIN		= 122,
   AL_KEY_MENU		= 123,
   AL_KEY_SCROLLLOCK	= 124,
   AL_KEY_NUMLOCK	= 125,
   AL_KEY_CAPSLOCK	= 126,

   AL_KEY_MAX
};



/* Type: Keyboard modifier flags
 *
 * > AL_KEYMOD_SHIFT     
 * > AL_KEYMOD_CTRL      
 * > AL_KEYMOD_ALT       
 * > AL_KEYMOD_LWIN      
 * > AL_KEYMOD_RWIN      
 * > AL_KEYMOD_MENU      
 * > AL_KEYMOD_ALTGR     
 * > AL_KEYMOD_COMMAND   
 * > AL_KEYMOD_SCROLLLOCK
 * > AL_KEYMOD_NUMLOCK   
 * > AL_KEYMOD_CAPSLOCK  
 * > AL_KEYMOD_INALTSEQ  
 * > AL_KEYMOD_ACCENT1   
 * > AL_KEYMOD_ACCENT2   
 * > AL_KEYMOD_ACCENT3   
 * > AL_KEYMOD_ACCENT4   
 */
enum
{
   AL_KEYMOD_SHIFT       = 0x00001,
   AL_KEYMOD_CTRL        = 0x00002,
   AL_KEYMOD_ALT         = 0x00004,
   AL_KEYMOD_LWIN        = 0x00008,
   AL_KEYMOD_RWIN        = 0x00010,
   AL_KEYMOD_MENU        = 0x00020,
   AL_KEYMOD_ALTGR       = 0x00040,
   AL_KEYMOD_COMMAND     = 0x00080,
   AL_KEYMOD_SCROLLLOCK  = 0x00100,
   AL_KEYMOD_NUMLOCK     = 0x00200,
   AL_KEYMOD_CAPSLOCK    = 0x00400,
   AL_KEYMOD_INALTSEQ	 = 0x00800,
   AL_KEYMOD_ACCENT1     = 0x01000,
   AL_KEYMOD_ACCENT2     = 0x02000,
   AL_KEYMOD_ACCENT3     = 0x04000,
   AL_KEYMOD_ACCENT4     = 0x08000
};



#endif
