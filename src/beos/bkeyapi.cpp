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
 *      Stuff for BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#include <stdlib.h>

#include "bealleg.h"
#include "allegro/aintern.h"
#include "allegro/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif                

#define KEY_ID_PC101 0 // the docs say it should be 0x83ab, but they lie

#define KEY_SEMAPHORE_NAME "keyboard driver waiting..."

#define KEY_THREAD_PERIOD   33333             // microseconds, 1/30th of a second
#define KEY_THREAD_NAME     "keyboard driver"
#define KEY_THREAD_PRIORITY 100               // real-time

#define COMBI_OPTION_CAPS_SHIFT (B_OPTION_KEY | B_CAPS_LOCK | B_SHIFT_KEY)
#define COMBI_OPTION_CAPS       (B_OPTION_KEY | B_CAPS_LOCK)
#define COMBI_OPTION_SHIFT      (B_OPTION_KEY | B_SHIFT_KEY)
#define COMBI_CAPS_SHIFT        (B_CAPS_LOCK | B_SHIFT_KEY)



static uint16 keyboard_id = (uint16)(-1);

static key_map *keybd         = NULL;
static char    *characters    = NULL;

static uint32   ch_acute      = 0;
static uint32   ch_grave      = 0;
static uint32   ch_circumflex = 0;
static uint32   ch_dieresis   = 0;
static uint32   ch_tilde      = 0;

static uint32   ch_dead       = 0;
static int32   *accent_table  = NULL;

static volatile int keyboard_thread_running = FALSE;
static thread_id    keyboard_thread_id      = -1;

static sem_id waiting_for_input = -1;

static const int be_to_allegro[128] = {
// Allegro			Be	Key
   -1,			//	0	(not used)
   KEY_ESC,		//	1	ESC
   KEY_F1,		//	2	F1
   KEY_F2,		//	3	F2
   KEY_F3,		//	4	F3
   KEY_F4,		//	5	F4
   KEY_F5,		//	6	F5
   KEY_F6,		//	7	F6
   KEY_F7, 		//	8	F7
   KEY_F8,		//	9	F8
   KEY_F9,		//	10	F9
   KEY_F10,		//	11	F10
   KEY_F11,		//	12	F11
   KEY_F12,		// 	13	F12
   KEY_PRTSCR,		//	14	Print Screen / SysRq
   KEY_SCRLOCK,		//	15	Scroll Lock
   KEY_PAUSE,		//	16	Pause / Break
   KEY_TILDE,		//	17	Tilde
   KEY_1,		//	18	'1'
   KEY_2,		//	19	'2'
   KEY_3,		//	20	'3'
   KEY_4,		//	21	'4'
   KEY_5,		//	22	'5'
   KEY_6,		//	23	'6'
   KEY_7,		//	24	'7'
   KEY_8,		//	25	'8'
   KEY_9,		//	26	'9'
   KEY_0,		//	27	'0'
   KEY_MINUS,		//	28	'-'
   KEY_EQUALS,		//	29	'='
   KEY_BACKSPACE,	//	30	Backspace
   KEY_INSERT,		//	31	Insert
   KEY_HOME,		//	32	Home
   KEY_PGUP,		//	33	Page Up
   KEY_NUMLOCK,		//	34	Num Lock
   KEY_SLASH_PAD,	//	35	Pad '/'
   KEY_ASTERISK,	//	36	Pad '*'
   KEY_MINUS_PAD,	//	37	Pad '-'
   KEY_TAB,		//	38	Tab
   KEY_Q,		//	39	'q'
   KEY_W,		//	40	'w'
   KEY_E,		//	41	'e'
   KEY_R,		//	42	'r'
   KEY_T,		//	43	't'
   KEY_Y,		//	44	'y'
   KEY_U,		//	45	'u'
   KEY_I,		//	46	'i'
   KEY_O,		//	47	'o'
   KEY_P,		//	48	'p'
   KEY_OPENBRACE,	//	49	'['
   KEY_CLOSEBRACE,	//	50	']'
   KEY_SLASH,		//	51	'\'
   KEY_DEL,		//	52	Delete
   KEY_END,		//	53	End
   KEY_PGDN,		//	54	Page Down
   KEY_7_PAD,		//	55	Pad '7'
   KEY_8_PAD,		//	56	Pad '8'
   KEY_9_PAD,		// 	57	Pad '9'
   KEY_PLUS_PAD,	//	58	Pad '+'
   KEY_CAPSLOCK,	//	59	Caps Lock
   KEY_A,		//	60	'a'
   KEY_S,		//	61	's'
   KEY_D,		//	62	'd'
   KEY_F,		//	63	'f'
   KEY_G,		//	64	'g'
   KEY_H,		//	65	'h'
   KEY_J,		//	66	'j'
   KEY_K,		//	67	'k'
   KEY_L,		//	68	'l'
   KEY_COLON,		//	69	';'
   KEY_QUOTE,		//	70	'''
   KEY_ENTER,		//	71	Enter
   KEY_4_PAD,		//	72	Pad '4'
   KEY_5_PAD,		//	73	Pad '5'
   KEY_6_PAD,		//	74	Pad '6'
   KEY_LSHIFT,		//	75	Left Shift
   KEY_Z,		//	76	'z'
   KEY_X,		//	77	'x'
   KEY_C,		//	78	'c'
   KEY_V,		//	79	'v'
   KEY_B,		//	80	'b'
   KEY_N,		//	81	'n'
   KEY_M,		//	82	'm'
   KEY_COMMA,		//	83	','
   KEY_STOP,		//	84	'.'
   KEY_SLASH,		//	85	'/'
   KEY_RSHIFT,		//	86	Right Shift
   KEY_UP,		//	87	Up
   KEY_1_PAD,		//	88	Pad '1'
   KEY_2_PAD,		//	89	Pad '2'
   KEY_3_PAD,		//	90	Pad '3'
   KEY_ENTER_PAD,	//	91	Pad Enter
   KEY_LCONTROL,	//	92	Left Control
   KEY_ALT,		//	93	Left Alt
   KEY_SPACE,		//	94	Space
   KEY_ALTGR,		//	95	Right Alt
   KEY_RCONTROL,	//	96	Right Control
   KEY_LEFT,		//	97	Left
   KEY_DOWN,		//	98	Down
   KEY_RIGHT,		//	99	Right
   KEY_0_PAD,		//	100	Pad '0'
   KEY_DEL_PAD,		//	101	Pad Del
   KEY_LWIN,		//	102	Left Windows Key
   KEY_RWIN,		//	103	Right Windows Key
   KEY_MENU,		//	104	Menu
   -1,			//	105	(not used)
   -1,			//	106	(not used)
   -1,			//	107	(not used)
   -1,			//	108	(not used)
   -1,			//	109	(not used)
   -1,			//	110	(not used)
   -1,			//	111	(not used)
   -1,			//	112	(not used)
   -1,			//	113	(not used)
   -1,			//	114	(not used)
   -1,			//	115	(not used)
   -1,			//	116	(not used)
   -1,			//	117	(not used)
   -1,			//	118	(not used)
   -1,			//	119	(not used)
   -1,			//	120	(not used)
   -1,			//	121	(not used)
   -1,			//	122	(not used)
   -1,			//	123	(not used)
   -1,			//	124	(not used)
   -1,			//	125	(not used)
   -1,			//	126	(not used)
   -1			//	127	(not used)
};

static const uint32 keypad[] = {
   34,  //      Num Lock
   35,	//	Pad '/'
   36,	//	Pad '*'
   37,	//	Pad '-'
   55,	//	Pad '7'
   56,	//	Pad '8'
   57,	//	Pad '9'
   58,	//	Pad '+'
   72,	//	Pad '4'
   73,	//	Pad '5'
   74,  //	Pad '6'
   88,	//	Pad '1'
   89,  //	Pad '2'
   90,  //	Pad '3'
   91,	//	Pad Enter
   100,	//	Pad '0'
   101,	//	Pad Del
   0
};



/* is_member:
 */
static inline bool is_member(uint32 be_key, const uint32 keys[])
{
   int index = 0; 

   while (keys[index] > 0) {
      if (keys[index] == be_key) {
         return true;
      }

      index++;
   }

   return false;
}



/* make_character:
 */
static inline uint32 make_character(int32 offset)
{
   int    len;
   uint32 ch;

   ch  = 0;
   len = characters[offset];

   for (int index = 0; index < len; index++) {
      ch <<= (index * 8);
      ch |=  (characters[offset + index + 1] & 0xFF);
   }

   return ch;
}



/* get_accent_table:
 */
static inline int32 *get_accent_table(uint32 ch, uint32 table)
{
    if ((keybd->acute_tables & table) && (ch_acute == ch)) {
       return keybd->acute_dead_key;
    }
    else if ((keybd->grave_tables & table) && (ch_grave == ch)) {
       return keybd->grave_dead_key;
    }
    else if ((keybd->circumflex_tables & table) && (ch_circumflex == ch)) {
       return keybd->circumflex_dead_key;
    }
    else if ((keybd->dieresis_tables & table) && (ch_dieresis == ch)) {
       return keybd->dieresis_dead_key;
    }
    else if ((keybd->tilde_tables & table) && (ch_tilde == ch)) {
       return keybd->tilde_dead_key;
    }
    else {
       return NULL;
    }
}



/* accent_character:
 */
static inline uint32 accent_character(uint32 ch, int32 accents[32])
{
   for (int index = 0; index < 32; index += 2) {
      if (make_character(accents[index]) == ch) {
         return make_character(accents[index + 1]);
      }
   }

   return 0;
}



/* lookup_character:
 */
static inline void lookup_character(uint32 be_key, uint32 be_modifiers, uint32 out[2])
{
   bool   invert;
   int32  offset;
   uint32 table;

   if((be_modifiers & B_NUM_LOCK) && is_member(be_key, keypad)) {
      invert = true;
   }
   else {
      invert = false;
   }

   if (be_modifiers & B_CONTROL_KEY) {
      offset = keybd->control_map[be_key];
      table  = B_CONTROL_TABLE;
   }
   else if ((be_modifiers & COMBI_OPTION_CAPS_SHIFT) == COMBI_OPTION_CAPS_SHIFT) {
      if(invert) {
         offset = keybd->option_caps_map[be_key];
         table  = B_OPTION_CAPS_TABLE;
      }
      else {
         offset = keybd->option_caps_shift_map[be_key];
         table  = B_OPTION_CAPS_SHIFT_TABLE;
      }
   }
   else if ((be_modifiers & COMBI_OPTION_CAPS) == COMBI_OPTION_CAPS) {
      if(invert) {
         offset = keybd->option_caps_shift_map[be_key];
         table  = B_OPTION_CAPS_SHIFT_TABLE;
      }
      else {
         offset = keybd->option_caps_map[be_key];
         table  = B_OPTION_CAPS_TABLE;
      }
   }
   else if ((be_modifiers & COMBI_OPTION_SHIFT) == COMBI_OPTION_SHIFT) {
      if(invert) {
         offset = keybd->option_map[be_key];
         table  = B_OPTION_TABLE;
      }
      else {
         offset = keybd->option_shift_map[be_key];
         table  = B_OPTION_SHIFT_TABLE;
      }
   }
   else if ((be_modifiers & B_OPTION_KEY) == B_OPTION_KEY) {
      if(invert) {
         offset = keybd->option_shift_map[be_key];
         table  = B_OPTION_SHIFT_TABLE;
      }
      else {
         offset = keybd->option_map[be_key];
         table  = B_OPTION_TABLE;
      }
   }
   else if ((be_modifiers & COMBI_CAPS_SHIFT) == COMBI_CAPS_SHIFT) {
      if(invert) {
         offset = keybd->caps_map[be_key];
         table  = B_CAPS_TABLE;
      }
      else {
         offset = keybd->caps_shift_map[be_key];
         table  = B_CAPS_SHIFT_TABLE;
      }
   }
   else if ((be_modifiers & B_CAPS_LOCK) == B_CAPS_LOCK) {
      if(invert) {
         offset = keybd->caps_shift_map[be_key];
         table  = B_CAPS_SHIFT_TABLE;
      }
      else {
         offset = keybd->caps_map[be_key];
         table  = B_CAPS_TABLE;
      }
   }
   else if ((be_modifiers & B_SHIFT_KEY) == B_SHIFT_KEY) {
      if(invert) {
         offset = keybd->normal_map[be_key];
         table  = B_NORMAL_TABLE;
      }
      else {
         offset = keybd->shift_map[be_key];
         table  = B_SHIFT_TABLE;
      }
   }
   else if (!be_modifiers) {
      if(invert) {
         offset = keybd->shift_map[be_key];
         table  = B_SHIFT_TABLE;
      }
      else {
         offset = keybd->normal_map[be_key];
         table  = B_NORMAL_TABLE;
      }
   }
   else {
      out[0] = 0;
      out[1] = 0;

      return;
   }

   uint32 ch = make_character(offset);

   if (accent_table == NULL) {
      accent_table = get_accent_table(ch, table);

      if (accent_table == NULL) {
         out[0] = ch;
         out[1] = 0;
      }
      else {
         ch_dead = ch;

         out[0] = 0;
         out[1] = 0;
      }
   }
   else {
      uint32 ch_accented = accent_character(ch, accent_table);

      if (ch_accented != 0) {
         out[0] = ch_accented;
         out[1] = 0;
      }
      else {
         out[0] = ch_dead;
         out[1] = ch;
      }

      accent_table = NULL;
      ch_dead      = 0;
   }
}



static int get_shift_flag(uint32 be_key)
{
   if(be_key == keybd->left_shift_key ||
      be_key == keybd->right_shift_key) {
      return KB_SHIFT_FLAG;
   }
   else if(be_key == keybd->right_control_key ||
           be_key == keybd->left_control_key) {
      return KB_CTRL_FLAG;
   }
   else if(be_key == keybd->right_command_key ||
           be_key == keybd->left_command_key) {
      return KB_ALT_FLAG;
   }
   else if(be_key == keybd->left_option_key) {
      return KB_LWIN_FLAG;
   }
   else if(be_key == keybd->right_option_key) {
      return KB_RWIN_FLAG;
   }
   else if(be_key == keybd->menu_key) {
      return KB_MENU_FLAG;
   }
   else if(be_key == keybd->scroll_key) {
      return KB_SCROLOCK_FLAG;
   }
   else if(be_key == keybd->num_key) {
      return KB_NUMLOCK_FLAG;
   }
   else if(be_key == keybd->caps_key) {
      return KB_CAPSLOCK_FLAG;
   }
   else {
      return 0;
   }
}



/* keyboard_thread:
 */
static int32 keyboard_thread(void *keyboard_started)
{
   key_info  key_info_old;
   key_info  key_info_new;

   get_key_info(&key_info_old);

   release_sem(*(sem_id *)keyboard_started);

   while(keyboard_thread_running) {
      int i;
      int j;

      get_key_info(&key_info_new);

      if (three_finger_flag &&
          (_key_shifts & KB_CTRL_FLAG) &&
          (_key_shifts & KB_ALT_FLAG)  &&
          (key_info_new.key_states[52 / 8] & (52 % 8)) ) {
         be_terminate(keyboard_thread_id);
      }

      for(i = 0; i < 16; i++) {
         for(j = 0; j < 8; j++) {
            int    new_key_pressed;
            int    old_key_pressed;
            uint32 be_key;

            new_key_pressed = ((key_info_new.key_states[i] << j) & 128);
            old_key_pressed = ((key_info_old.key_states[i] << j) & 128);
            be_key = (i * 8) + j;

            if (new_key_pressed != old_key_pressed) {
               int scancode;

               scancode = be_to_allegro[be_key];

               if (new_key_pressed && (focus_count > 0)) {
                  uint32 keycode[2];
                  int    shift_flag;

                  shift_flag = get_shift_flag(be_key);
                  _key_shifts |= shift_flag;

                  if (shift_flag == 0) {
                     lookup_character(be_key, key_info_new.modifiers, keycode);
                  }
                  else {
                     keycode[0] = -1;
                     keycode[1] = 0;
                  }

                  _handle_key_press((int)(keycode[0]), scancode);

                  if (keycode[1] != 0) {
                     _handle_key_press((int)(keycode[1]), scancode);
                  }
               }
               else if (old_key_pressed) {
                  _key_shifts &= ~(get_shift_flag(be_key));
                  _handle_key_release(scancode);
               }
            }
         }
      }

      key_info_old = key_info_new;

      snooze(KEY_THREAD_PERIOD);
   }

 AL_TRACE("keyboard thread exited\n");

 return 0;
}



/* set_default_key_repeat:
 */
static inline bool set_default_key_repeat(void)
{
   int32     rate;
   bigtime_t delay;

   if (get_key_repeat_rate(&rate) != B_OK) {
      return false;
   }

   if (get_key_repeat_delay(&delay) != B_OK) {
      return false;
   }

   set_keyboard_rate((delay / 1000), (10000 / rate));

   return true;
}



/* be_key_init:
 */
extern "C" int be_key_init(void)
{
   sem_id keyboard_started;

   if (get_keyboard_id(&keyboard_id) == B_ERROR) {
      goto cleanup;
   }

   if (keyboard_id != KEY_ID_PC101) {
      goto cleanup;
   }

   get_key_map(&keybd, &characters);

   if (keybd == NULL || characters == NULL) {
      goto cleanup;
   }

   ch_acute      = make_character(keybd->acute_dead_key[1]);
   ch_grave      = make_character(keybd->grave_dead_key[1]);
   ch_circumflex = make_character(keybd->circumflex_dead_key[1]);
   ch_dieresis   = make_character(keybd->dieresis_dead_key[1]);
   ch_tilde      = make_character(keybd->tilde_dead_key[1]);

   ch_dead       = 0;
   accent_table  = NULL;

   waiting_for_input = create_sem(0, "waiting for input...");

   if (waiting_for_input < B_NO_ERROR) {
      goto cleanup;
   }

   keyboard_started = create_sem(0, "starting keyboard thread...");

   if (keyboard_started < B_NO_ERROR) {
      goto cleanup;
   }

   keyboard_thread_id = spawn_thread(keyboard_thread, KEY_THREAD_NAME,
                           KEY_THREAD_PRIORITY, &keyboard_started);

   if (keyboard_thread_id < 0) {
      goto cleanup;
   }

   keyboard_thread_running = TRUE;
   resume_thread(keyboard_thread_id);
   acquire_sem(keyboard_started);
   delete_sem(keyboard_started);

   if(!set_default_key_repeat()) {
     goto cleanup;
   }

   return 0;

   cleanup: {
      if (keyboard_started > 0) {
         delete_sem(keyboard_started);
      }

      be_key_exit();
      return 1;
   }
}



/* be_key_exit:
 */
extern "C" void be_key_exit(void)
{
   if (keyboard_thread_id > 0) {
      keyboard_thread_running = FALSE;
      wait_for_thread(keyboard_thread_id, &ignore_result);
      keyboard_thread_id = -1;
   }

   if (waiting_for_input > 0) {
      delete_sem(waiting_for_input);
      waiting_for_input = -1;
   }

   ch_acute      = 0;
   ch_grave      = 0;
   ch_circumflex = 0;
   ch_dieresis   = 0;
   ch_tilde      = 0;

   ch_dead       = 0;
   accent_table  = NULL;

   free(characters);
   characters = NULL;

   free(keybd);
   keybd = NULL;

   keyboard_id = (uint16)(-1);
}



extern "C" void be_key_set_leds(int leds)
{
   uint32 modifiers;

   modifiers = 0;

   if (leds & KB_CAPSLOCK_FLAG) {
      modifiers |= B_CAPS_LOCK;
   }

   if (leds & KB_SCROLOCK_FLAG) {
      modifiers |= B_SCROLL_LOCK;
   }

   if (leds & KB_NUMLOCK_FLAG) {
      modifiers |= B_NUM_LOCK;
   }

   set_keyboard_locks(modifiers);
}



/* be_key_wait_for_input:
 */
extern "C" void be_key_wait_for_input(void)
{
   acquire_sem(waiting_for_input);
}



/* be_key_stop_waiting_for_input:
 */
extern "C" void be_key_stop_waiting_for_input(void)
{
   release_sem(waiting_for_input);
}



extern "C" void be_key_suspend(void)
{
   suspend_thread(keyboard_thread_id);
}


extern "C" void be_key_resume(void)
{
   resume_thread(keyboard_thread_id);
}
