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
 *      Mac system, keyboard, timer and mouse drivers.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "macalleg.h"
#include "allegro/aintern.h"
#include "allegro/aintmac.h"

#define TRACE_MAC_KBRD 0
#define TRACE_MAC_MOUSE 0
#define TRACE_MAC_SYSTEM 0


/*Our TimerManager task entry queue*/
TMTask   tm_entry;      
/*Our TM task is running*/
short tm_running=0;
short mac_timer_installed=FALSE;
short mac_mouse_installed=FALSE;
short mac_keyboard_installed=FALSE;
/*Used for our keyboard driver*/
volatile KeyMap KeyNow;
volatile KeyMap KeyOld;
/*Our window title ???*/
char macwindowtitle[256];
/*char buffer for Trace*/
char macsecuremsg[256];

static pascal void tm_interrupt(TMTaskPtr tmTaskPtr);
static int mac_timer_init(void);
static void mac_timer_exit(void);
static int mouse_mac_init(void);
static void mouse_mac_exit(void);
static int key_mac_init(void);
static void key_mac_exit(void);
static int mac_init(void);
static void mac_exit(void);
static void mac_get_executable_name(char *output, int size);
static void mac_set_window_title(const char *name);
static void mac_message(const char *msg);
static void mac_assert(const char *msg);
static int mac_desktop_color_depth(void);
static void mac_yield_timeslice(void);
static int mac_trace_handler(const char *msg);


const int k_apple_caps=0x3E;
const int k_apple_shift=0x3F;
const int k_apple_win=0x30; /* mouse right button emulation */
/*
The LookUp table to translate AppleKeyboard scan codes to Allegro KEY constants
*/

const unsigned char key_apple_to_allegro[128]=
{
/*00*/   KEY_X,      KEY_Z,      KEY_G,      KEY_H, 
/*04*/   KEY_F,      KEY_D,      KEY_S,      KEY_A,   
/*08*/   KEY_R,      KEY_E,      KEY_W,      KEY_Q, 
/*0C*/   KEY_B,      KEY_TILDE,  KEY_V,      KEY_C, 
/*10*/   KEY_5,      KEY_6,      KEY_4,      KEY_3, 
/*14*/   KEY_2,      KEY_1,      KEY_T,      KEY_Y, 
/*18*/   KEY_O,      KEY_CLOSEBRACE, KEY_0,   KEY_8, 
/*1C*/   KEY_MINUS,  KEY_7,      KEY_9,      KEY_EQUALS, 
/*20*/   KEY_QUOTE,  KEY_J,      KEY_L,      KEY_ENTER, 
/*24*/   KEY_P,      KEY_I,      KEY_OPENBRACE, KEY_U, 
/*28*/   KEY_STOP,   KEY_M,      KEY_N,      KEY_SLASH, 
/*2C*/   KEY_COMMA,  KEY_BACKSLASH, KEY_COLON, KEY_K, 
/*30*/   /*KEY_LWIN used for right button of mouse*/ 
         0,          0,          KEY_ESC,    0,   
/*34*/   KEY_BACKSPACE, KEY_BACKSLASH2, KEY_SPACE, KEY_TAB, 
/*38*/   0,          0,          0,          0,   
/*3C*/   KEY_LCONTROL, KEY_ALT,   KEY_CAPSLOCK, KEY_LSHIFT, 
/*40*/   KEY_NUMLOCK,0,          KEY_PLUS_PAD, 0,   
/*44*/   KEY_ASTERISK, 0,        KEY_DEL_PAD, 0,   
/*48*/   0,          KEY_MINUS_PAD, 0,        KEY_ENTER_PAD,   
/*4C*/   KEY_SLASH_PAD, 0,        0,          0,   
/*50*/   KEY_5_PAD,  KEY_4_PAD,  KEY_3_PAD,   KEY_2_PAD, 
/*54*/   KEY_1_PAD,  KEY_0_PAD,  0,          0,   
/*58*/   0,          0,          0,          KEY_9_PAD, 
/*5C*/   KEY_8_PAD,  0,          KEY_7_PAD,  KEY_6_PAD,   
/*60*/   KEY_F11,    0,          KEY_F9,     KEY_F8, 
/*64*/   KEY_F3,     KEY_F7,     KEY_F6,     KEY_F5, 
/*68*/   KEY_F12,    0,          KEY_F10,    0,   
/*6C*/   KEY_SCRLOCK, 0,         KEY_PRTSCR, 0,   
/*70*/   KEY_END,    KEY_F4,     KEY_DEL,    KEY_PGUP, 
/*74*/   KEY_HOME,   KEY_INSERT, KEY_PAUSE,          0,   
/*78*/   0,          KEY_UP,     KEY_DOWN,   KEY_RIGHT, 
/*7C*/   KEY_LEFT,   KEY_F1,     KEY_PGDN,   KEY_F2, 
};

/*
The LookUp table to translate AppleKeyboard scan codes to lowcase ASCII chars
*/

const unsigned char key_apple_to_char[128]=
{
/*00*/   'x',      'z',      'g',      'h', 
/*04*/   'f',      'd',      's',      'a',   
/*08*/   'r',      'e',      'w',      'q', 
/*0C*/   'b',      '~',      'v',      'c', 
/*10*/   '5',      '6',      '4',      '3', 
/*14*/   '2',      '1',      't',      'y', 
/*18*/   'o',      ']',      '0',      '8', 
/*1C*/   '-',      '7',      '9',      '=', 
/*20*/   ' ',      'j',      'l',      0xD, 
/*24*/   'p',      'i',      '[',      'u', 
/*28*/   '.',      'm',      'n',      '/', 
/*2C*/   ', ',      '\\',      ':',      'k', 
/*30*/   0,         0,        27,         0,   
/*34*/   8,         '\\',     ' ',       9, 
/*38*/   0,         0,         0,         0,   
/*3C*/   0,         0,         0,         0, 
/*40*/   0,         0,         '+',       0,   
/*44*/   '*',      0,          0,         0,   
/*48*/   0,         '-',       0,       0xD,   
/*4C*/   '/',      0,          0,         0,   
/*50*/   '5',      '4',       '3',       '2', 
/*54*/   '1',      '0',        0,         0,   
/*58*/   0,         0,         0,        '9', 
/*5C*/   '8',       0,         '7',      '6',   
/*60*/   0,         0,         0,         0, 
/*64*/   0,         0,         0,         0, 
/*68*/   0,         0,         0,         0,   
/*6C*/   0,         0,         0,         0,   
/*70*/   0,         0,         127,       0, 
/*74*/   0,         0,         0,         0,   
/*78*/   0,         0,         0,         0, 
/*7C*/   0,         0,         0,         0, 
};


#pragma mark todo
/*
Todo LookUp table to translate AppleKeyboard scan codes to upcase ASCII chars
   and diacrilic chars
*/


SYSTEM_DRIVER system_macos ={
   SYSTEM_MACOS, 
   empty_string, 
   empty_string, 
   "MacOs", 
   mac_init, 
   mac_exit, 
   mac_get_executable_name, 
   NULL, 
   mac_set_window_title, 
   mac_message, 
   mac_assert, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   mac_desktop_color_depth, 
   mac_yield_timeslice, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
   NULL, 
};

MOUSE_DRIVER mouse_macos ={
      MOUSE_MACOS, 
      empty_string, 
      empty_string, 
      "MacOs Mouse", 
      mouse_mac_init, 
      mouse_mac_exit, 
      NULL, 
      NULL, 
      NULL, 
      NULL, 
      NULL, 
      NULL, 
      NULL
};
KEYBOARD_DRIVER keyboard_macos ={
   KEYBOARD_MACOS, 
   empty_string, 
   empty_string, 
   "MacOs Key", 
   0, 
   key_mac_init, 
   key_mac_exit, 
   NULL, NULL, NULL, 
   NULL, 
   NULL, 
   NULL
};
TIMER_DRIVER timer_macos ={
      TIMER_MACOS, 
      empty_string, 
      empty_string, 
      "MacOs Timer", 
      mac_timer_init, 
      mac_timer_exit, 
      NULL, 
      NULL, 
      NULL, 
      NULL, 
      NULL, 
      NULL, 
      NULL, 
};



/*
 *
 */
static int mac_init()
{
   os_type = 'MAC ';
   register_trace_handler(mac_trace_handler);
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_init()::OK\n");
   fflush(stdout);
#endif
   _rgb_r_shift_15 = 10;         /*Ours truecolor pixel format */
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;
   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;
   return 0;
}



/*
 *
 */
static void mac_exit()
{
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_exit()\n");
   fflush(stdout);
#endif
}



/*
 *
 */ 
void _mac_get_executable_name(char *output, int size) {
   Str255 appName;
   OSErr e;
   ProcessInfoRec info;
   ProcessSerialNumber curPSN;   

   e = GetCurrentProcess(&curPSN);

   info.processInfoLength = sizeof(ProcessInfoRec);
   info.processName = appName;
   info.processAppSpec = NULL;

   e = GetProcessInformation(&curPSN, &info);

   size=MIN(size, (unsigned char)appName[0]);
   strncpy(output, (char const *)appName+1, size);
   output[size]=0;
}



/*
 *
 */
static void mac_get_executable_name(char *output, int size) {
   _mac_get_executable_name(output, size);
}



/*
 *
 */
static void mac_set_window_title(const char *name)
{
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_set_window_title(%s)\n", name);
   fflush(stdout);
#endif
   memcpy(macwindowtitle, name, 254);
   macwindowtitle[254]=0;
}



/*
 *
 */
void _mac_message(const char *msg)
{
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_message(%s)\n", msg);
   fflush(stdout);
#endif
   memcpy(macsecuremsg, msg, 254);
   macsecuremsg[254]=0;
   paramtext(macsecuremsg, "\0", "\0", "\0");
   ShowCursor();
   Alert(rmac_message, nil);
   HideCursor();
}



/*
 *
 */
static void mac_message(const char *msg)
{
   _mac_message(msg);
}



/*
 *
 */
static void mac_assert(const char *msg)
{
   debugstr(msg);
}



/*
 *
 */
static int mac_desktop_color_depth(void)
{
   return 0;
}



/*
 *
 */
static void mac_yield_timeslice(void)
{
   SystemTask();   
}



/*
 *
 */
static int mac_trace_handler(const char *msg)
{
   debugstr(msg);
   return 0;
}



/*
 *
 */
static int mac_timer_init(void)
{
   if (!tm_running)
      _tm_sys_init();
   mac_timer_installed=1;
   return 0;
}


/*
 *
 */
static void mac_timer_exit(void)
{
   mac_timer_installed=0;
}



/*
 *
 */
static int mouse_mac_init(void)
{
#if (TRACE_MAC_MOUSE)
   
   fprintf(stdout, "mouse_mac_init()\n");
   fflush(stdout);
#endif
   if (!tm_running)
      _tm_sys_init();
   mac_mouse_installed=1;
   return 2;/*emulated two button command-click*/
}



/*
 *
 */
static void mouse_mac_exit(void)
{
#if (TRACE_MAC_MOUSE)   
   fprintf(stdout, "mouse_mac_exit()\n");
   fflush(stdout);
#endif
   mac_mouse_installed=0;
}



/*
 *
 */
static int key_mac_init(void)
{
#if (TRACE_MAC_KBRD)
   fprintf(stdout, "key_mac_init()\n");
   fflush(stdout);
#endif
   GetKeys(KeyNow);
   BlockMove(KeyNow, KeyOld, sizeof(KeyMap));
   if (!tm_running)
      _tm_sys_init();
   mac_keyboard_installed=1;
   return 0;
}



/*
 *
 */
static void key_mac_exit(void)
{
#if (TRACE_MAC_KBRD)
   fprintf(stdout, "key_mac_exit()\n");
   fflush(stdout);
#endif
   mac_keyboard_installed=0;
}



/*
 *
 */
static pascal void tm_interrupt(TMTaskPtr tmTaskPtr)
{
   char ascii;
   Boolean  upper;
   Boolean  symbol;
   if (mac_timer_installed)_handle_timer_tick(MSEC_TO_TIMER(50));
   if (mac_mouse_installed||mac_keyboard_installed){
      KeyNow[0]=(*(UInt32*)0x174);
      KeyNow[1]=(*(UInt32*)0x178);
      KeyNow[2]=(*(UInt32*)0x17C);
      KeyNow[3]=(*(UInt32*)0x180);
   }
   if (mac_mouse_installed) {
      Point pt;
      pt=(*((volatile Point *)0x082C));
      _mouse_b = (*((volatile UInt8 *)0x0172))&0x80?0:BitTst(KeyNow, k_apple_win)?2:1;
      _mouse_x = pt.h;
      _mouse_y = pt.v;
      _handle_mouse_input();
   }
   if (mac_keyboard_installed) {
      UInt32 xKeys;
      int a, row, base, keycode;
	  upper=BitTst(KeyNow, k_apple_caps);
	  symbol=BitTst(KeyNow, k_apple_shift);
	  upper^=symbol;
      for(row=0, base=0;row<4;row++, base+=32) {
         xKeys=KeyOld[row]^KeyNow[row];
         if (xKeys) {
            for(a=0;a<32;a++) {
               if (BitTst(&xKeys, a)) {
                  keycode=key_apple_to_allegro[a+base];
                  if (keycode) {
                     if (BitTst( &KeyNow[row], a)) {
					    ascii=key_apple_to_char[a+base];
						if( upper && (keycode<=KEY_Z))
						   ascii-=32;
                        _handle_key_press(ascii, keycode);
                     }
                     else{
                        _handle_key_release(keycode);
                     }
                  }
               }
            }
         }
         KeyOld[row]=KeyNow[row];
      }    
   }
   PrimeTime((QElemPtr)tmTaskPtr, 50);
}



/*
 *
 */
int _tm_sys_init()
{
   if (!tm_running) {
      tm_entry.tmAddr = NewTimerProc(tm_interrupt);
      tm_entry.tmWakeUp = 0;
      tm_entry.tmReserved = 0;
      InsTime((QElemPtr)&tm_entry);
      PrimeTime((QElemPtr)&tm_entry, 40);
      tm_running=TRUE;
   }
   return 0;
}



/*
 *
 */
void _tm_sys_exit()
{
   if (tm_running)
      RmvTime((QElemPtr)&tm_entry);
   tm_running=FALSE;
}
