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
 *      Utility to create mapping from X keycodes to Allegro scancodes.
 *
 *      By Michael Bukin, modified by Elias Pschernig.
 *
 *      See readme.txt for copyright information.
 */

#include <stdio.h>

#include <allegro.h>
#include <xalleg.h>
#include <allegro/internal/aintern.h>

#define FILENAME_LENGTH  1024

char current_mapping_string[256] = "Current mapping: Default";
char unicode_description[256] = EMPTY_STRING;
static DIALOG main_dialog[];
static DIALOG keymap_dialog[];

static struct {
   char *string;
   int scancode;
} keymap[KEY_MAX + 2] = {
   {NULL, 0},
   {"KEY_A", 0x1E},
   {"KEY_B", 0x30},
   {"KEY_C", 0x2E},
   {"KEY_D", 0x20},
   {"KEY_E", 0x12},
   {"KEY_F", 0x21},
   {"KEY_G", 0x22},
   {"KEY_H", 0x23},
   {"KEY_I", 0x17},
   {"KEY_J", 0x24},
   {"KEY_K", 0x25},
   {"KEY_L", 0x26},
   {"KEY_M", 0x32},
   {"KEY_N", 0x31},
   {"KEY_O", 0x18},
   {"KEY_P", 0x19},
   {"KEY_Q", 0x10},
   {"KEY_R", 0x13},
   {"KEY_S", 0x1F},
   {"KEY_T", 0x14},
   {"KEY_U", 0x16},
   {"KEY_V", 0x2F},
   {"KEY_W", 0x11},
   {"KEY_X", 0x2D},
   {"KEY_Y", 0x15},
   {"KEY_Z", 0x2C},
   {"KEY_0", 0x0B},
   {"KEY_1", 0x02},
   {"KEY_2", 0x03},
   {"KEY_3", 0x04},
   {"KEY_4", 0x05},
   {"KEY_5", 0x06},
   {"KEY_6", 0x07},
   {"KEY_7", 0x08},
   {"KEY_8", 0x09},
   {"KEY_9", 0x0A},
   {"KEY_0_PAD", 0x52},
   {"KEY_1_PAD", 0x4F},
   {"KEY_2_PAD", 0x50},
   {"KEY_3_PAD", 0x51},
   {"KEY_4_PAD", 0x4B},
   {"KEY_5_PAD", 0x4C},
   {"KEY_6_PAD", 0x4D},
   {"KEY_7_PAD", 0x47},
   {"KEY_8_PAD", 0x48},
   {"KEY_9_PAD", 0x49},
   {"KEY_F1", 0x3B},
   {"KEY_F2", 0x3C},
   {"KEY_F3", 0x3D},
   {"KEY_F4", 0x3E},
   {"KEY_F5", 0x3F},
   {"KEY_F6", 0x40},
   {"KEY_F7", 0x41},
   {"KEY_F8", 0x42},
   {"KEY_F9", 0x43},
   {"KEY_F10", 0x44},
   {"KEY_F11", 0x57},
   {"KEY_F12", 0x58},
   {"KEY_ESC", 0x01},
   {"KEY_TILDE", 0x29},
   {"KEY_MINUS", 0x0C},
   {"KEY_EQUALS", 0x0D},
   {"KEY_BACKSPACE", 0x0E},
   {"KEY_TAB", 0x0F},
   {"KEY_OPENBRACE", 0x1A},
   {"KEY_CLOSEBRACE", 0x1B},
   {"KEY_ENTER", 0x1C},
   {"KEY_COLON", 0x27},
   {"KEY_QUOTE", 0x28},
   {"KEY_BACKSLASH", 0x2B},
   {"KEY_BACKSLASH2", 0x56 | 0x80},
   {"KEY_COMMA", 0x33},
   {"KEY_STOP", 0x34},
   {"KEY_SLASH", 0x35},
   {"KEY_SPACE", 0x39},
   {"KEY_INSERT", 0x52 | 0x80},
   {"KEY_DEL", 0x53 | 0x80},
   {"KEY_HOME", 0x47 | 0x80},
   {"KEY_END", 0x4F | 0x80},
   {"KEY_PGUP", 0x49 | 0x80},
   {"KEY_PGDN", 0x51 | 0x80},
   {"KEY_LEFT", 0x4B | 0x80},
   {"KEY_RIGHT", 0x4D | 0x80},
   {"KEY_UP", 0x48 | 0x80},
   {"KEY_DOWN", 0x50 | 0x80},
   {"KEY_SLASH_PAD", 0x35 | 0x80},
   {"KEY_ASTERISK", 0x37},
   {"KEY_MINUS_PAD", 0x4A | 0x80},
   {"KEY_PLUS_PAD", 0x4E},
   {"KEY_DEL_PAD", 0x53},
   {"KEY_ENTER_PAD", 0x1C | 0x80},
   {"KEY_PRTSCR", 0x54 | 0x80},
   {"KEY_PAUSE", 0x00 | 0x100},
   {"KEY_ABNT_C1", 0x73},
   {"KEY_YEN", 0x7D},
   {"KEY_KANA", 0x70},
   {"KEY_CONVERT", 0x79},
   {"KEY_NOCONVERT", 0x7B},
   {"KEY_AT", 0x11 | 0x80},
   {"KEY_CIRCUMFLEX", 0x10 | 0x80},
   {"KEY_COLON2", 0x12 | 0x80},
   {"KEY_KANJI", 0x14 | 0x80},
   {"KEY_EQUALS_PAD", 0},
   {"KEY_BACKQUOTE", 0},
   {"KEY_SEMICOLON", 0},
   {"KEY_COMMAND", 0},
   {"KEY_LSHIFT", 0x2A},
   {"KEY_RSHIFT", 0x36},
   {"KEY_LCONTROL", 0x1D},
   {"KEY_RCONTROL", 0x1D | 0x80},
   {"KEY_ALT", 0x38},
   {"KEY_ALTGR", 0x38 | 0x80},
   {"KEY_LWIN", 0x5B | 0x80},
   {"KEY_RWIN", 0x5C | 0x80},
   {"KEY_MENU", 0x5D | 0x80},
   {"KEY_SCRLOCK", 0x46},
   {"KEY_NUMLOCK", 0x45},
   {"KEY_CAPSLOCK", 0x3A},
   {NULL, 0}
};

static int black = 0;
static int white = 1;
static int red = 2;
static int yellow = 3;

static volatile int waiting_for_key = 0;
static volatile int new_keycode = 0;
static int keycode_to_scancode[256];

static void get_raw_keycode(int pressed, int keycode)
{
   if (pressed && waiting_for_key) {
      new_keycode = keycode;
      waiting_for_key = 0;
   }
}

static char *keycode_getter(int index, int *list_size)
{
   if (index >= 0) {
      return keymap[1 + index].string;
   }
   else {
      *list_size = KEY_MAX - 1;
      return NULL;
   }
}

/* update unicode chars produced by a keycode */
static void keycode_produces(int k)
{
   /* better would be to autodetect what to display, e.g. additional accents.. */
   unsigned short normal, shift, altgr_normal;
   if (k < KEY_MODIFIERS)
   {
      normal = _key_ascii_table[k];
      shift = _key_shift_table[k];
      altgr_normal = _key_altgr_lower_table[k];
      
      if (normal && normal != 65535 && normal >= 32 )
      {
	 uszprintf (unicode_description, 256, "Produces: %c %c %c",
	    normal, shift, altgr_normal);
      }
      else {
	 uszprintf (unicode_description, 256,  "Function key");
      }
   }
   else {
      uszprintf (unicode_description, 256,  "Modifier key");
   }
}

static int d_my_list_proc(int msg, DIALOG *d, int c)
{
   static int prev = -1;
   int ret = d_list_proc (msg, d, c);
   
   if (prev != d->d1) {
      prev = d->d1;
      keycode_produces (1 + d->d1);
      dialog_message (keymap_dialog, MSG_DRAW, 0, NULL);
   }

   return ret;
}

static DIALOG keymap_dialog[] = {
   /* 0 */{d_clear_proc,    0,   0, 250, 230, 0,   0,   0, 0,      0, 0, 0, NULL, NULL},
   /* 1 */{d_ctext_proc,  125,  10,   0,  16, 0, 255,   0, 0,      0, 0, "Select Keycode:", NULL, NULL},
   /* 2 */{d_my_list_proc, 10,  32, 230,  92, 0, 255,   0, D_EXIT, 0, 0, (void *)keycode_getter, NULL, NULL},
   /* 3 */{d_ctext_proc,  125, 142,   0,  16, 0, 255,   0, 0,      0, 0, unicode_description, NULL, NULL},
   /* 4 */{d_button_proc,  10, 164, 230,  16, 0, 255,   0, D_EXIT, 0, 0, "Define &X Key !", NULL, NULL},
   /* 5 */{d_button_proc,  10, 186, 110,  16, 0, 255,  27, D_EXIT, 0, 0, "&Cancel", NULL, NULL},
   /* 6 */{d_button_proc, 130, 186, 110,  16 , 0, 255,  0, D_EXIT, 0, 0, "&Done", NULL, NULL},
   /* 7 */{d_ctext_proc,  125, 208, 0,    16, 0, 255,   0, 0,      0, 0, "", NULL, NULL},
   /* 8 */{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL},
};

/* handle the setup command */
static void setup_all_keys(void)
{
   int focus = 2;
   int i;

   /* Prepare dialog.  */
   set_dialog_color(keymap_dialog, black, white);
   centre_dialog(keymap_dialog);

   /* Parse input.  */
   while (1) {
      focus = do_dialog(keymap_dialog, focus);
      switch (focus) {
	 case 2:
	 case 4:
	 
	    textprintf_centre_ex (screen, font,
	       keymap_dialog[7].x, keymap_dialog[7].y, red, -1,
	       "Press a key to map to the current scancode");
	    textprintf_centre_ex (screen, font,
	       keymap_dialog[7].x, keymap_dialog[7].y + 8, red, -1,
	       "(or a mouse button to leave it unchanged)");
	 
	    /* Wait for new key press.  */
	    new_keycode = -1;
	    waiting_for_key = 1;
	    
	    do {
	       poll_keyboard();
	       poll_mouse();
      
	       if (mouse_b)
		  waiting_for_key = 0;
	    }
	    while (waiting_for_key);
	    
	    /* Save keycode to scancode mapping.  */
	    if ((new_keycode >= 0) && (new_keycode < 256)) {
	       keycode_to_scancode[new_keycode] = keymap[1 + keymap_dialog[2].d1].scancode;
	    }

	    clear_keybuf();

	    break;
	 case 5:
	    return;
	 case 6:

	    for (i = 0; i < 256; i++) {
               if (keycode_to_scancode[i] >= 0)
		  _xwin.keycode_to_scancode[i] = keycode_to_scancode[i];
            }

	    return;
      }
   }
}

static void test_key_map(void)
{
   int i, k, u;
   static int key_was_pressed[KEY_MAX + 1] = {0};
   static int key_is_pressed[KEY_MAX + 1] = {0};
   static char *welcome[] = {
      "Key that is pressed now is marked with red",
      "Key that was pressed is marked with yellow",
      "Press mouse button or Escape to exit test",
      0
   };

   /* Clear screen and output prompt.  */
   clear_to_color(screen, white);
   for (i = 0; welcome[i] != 0; i++)
      textout_ex(screen, font, welcome[i], 8, i * 8 + 8, black, -1);

   clear_to_color(screen, white);
   for (i = 1; keymap[i].string != 0; i++)
      textout_ex(screen, font, keymap[i].string, 32 + ((i-1) % 4) * 160,
	      40 + ((i-1) / 4) * 14, black, -1);
   do {
      poll_keyboard();
      poll_mouse();
   }
   while ((key[KEY_ESC]) || (mouse_b));

   do {
      
      while (keypressed()) {
	 u = ureadkey (&k);
	 textprintf_centre_ex (screen, font, SCREEN_W / 2, 8,
	    red, white, ">   %c   <", u);
      }
      
      poll_keyboard();
      poll_mouse();

      for (i = 1; i < KEY_MAX; i++) {
	 if (key[i])
	    key_was_pressed[i] = key_is_pressed[i] = 1;
	 else
	    key_is_pressed[i] = 0;
      }

      for (i = 1; keymap[i].string != 0; i++) {
	 int x = 16 + ((i - 1) % 4) * 160;
	 int y = 40 + ((i - 1) / 4) * 14;

	 if (key_is_pressed[i])
	    rectfill(screen, x, y, x + 7, y + 7, red);
	 else if (key_was_pressed[i])
	    rectfill(screen, x, y, x + 7, y + 7, yellow);
	 else
	    rectfill(screen, x, y, x + 7, y + 7, white);
      }
   }
   while ((!key[KEY_ESC]) && (!mouse_b));

   do {
      poll_keyboard();
      poll_mouse();
   }
   while ((key[KEY_ESC]) || (mouse_b));

   clear_keybuf();
}

/* handle the save command */
static void save_key_map(void)
{
   int i;
   char *section, *option_format, option[80], tmp1[80], tmp2[80];

   set_config_file("xkeymap.cfg");
   section = uconvert_ascii("xkeymap", tmp1);
   option_format = uconvert_ascii("keycode%d", tmp2);

   for (i = 0; i < 256; i++) {
      if (keycode_to_scancode[i] > 0) {
	 usprintf(option, option_format, i);
	 set_config_int(section, option, keycode_to_scancode[i]);
      }
   }
}

/* reads a specific keymapping table from the config file */
void load_table(unsigned short *table, char *section)
{
   char name[80];
   int i;

   for (i=0; i<KEY_MAX; i++) {
      sprintf(name, "key%d", i);
      table[i] = get_config_int(section, name, table[i]);
   }
}

/* handle the load command */
static int load_keyconfig(void)
{
   char buf[FILENAME_LENGTH] = "";

   if (file_select_ex("Load Keyboard Config", buf, "CFG", sizeof(buf), 0, 0)) {
      strlwr(buf);

      push_config_state();
      set_config_file(buf);

      load_table(_key_ascii_table,               "key_ascii");
      load_table(_key_capslock_table,            "key_capslock");
      load_table(_key_shift_table,               "key_shift");
      load_table(_key_control_table,             "key_control");

      load_table(_key_altgr_lower_table,         "key_altgr");
      load_table(_key_altgr_upper_table,         "key_altgr");

      load_table(_key_altgr_lower_table,         "key_altgr_lower");
      load_table(_key_altgr_upper_table,         "key_altgr_upper");

      load_table(_key_accent1_lower_table,       "key_accent1_lower");
      load_table(_key_accent1_upper_table,       "key_accent1_upper");
      load_table(_key_accent2_lower_table,       "key_accent2_lower");
      load_table(_key_accent2_upper_table,       "key_accent2_upper");
      load_table(_key_accent3_lower_table,       "key_accent3_lower");
      load_table(_key_accent3_upper_table,       "key_accent3_upper");
      load_table(_key_accent4_lower_table,       "key_accent4_lower");
      load_table(_key_accent4_upper_table,       "key_accent4_upper");

      _key_accent1 = get_config_int("key_escape", "accent1", 0);
      _key_accent2 = get_config_int("key_escape", "accent2", 0);
      _key_accent3 = get_config_int("key_escape", "accent3", 0);
      _key_accent4 = get_config_int("key_escape", "accent4", 0);
      _key_accent1_flag = get_config_int("key_escape", "accent1_flag", 0);
      _key_accent2_flag = get_config_int("key_escape", "accent2_flag", 0);
      _key_accent3_flag = get_config_int("key_escape", "accent3_flag", 0);
      _key_accent4_flag = get_config_int("key_escape", "accent4_flag", 0);

      ustrcpy(current_mapping_string, "Current mapping: ");
      ustrzcat(current_mapping_string, 256, get_config_string(NULL, "keyboard_name", ""));

      pop_config_state();

   }

   return D_REDRAW;
}

static DIALOG main_dialog[] = {
   /* 0 */{d_clear_proc,   0,   0, 250, 102, 0,   0,   0, 0,      0, 0, 0, NULL, NULL},
   /* 1 */{d_ctext_proc, 120,  10,   0,  16, 0, 255, 'c', 0,      0, 0, current_mapping_string, NULL, NULL},
   /* 2 */{d_button_proc, 10,  32, 230,  16, 0, 255,   0, D_EXIT, 0, 0, "&Change Allegro key mappings", NULL, NULL},
   /* 3 */{d_button_proc, 10,  54, 230,  16, 0, 255, 'm', D_EXIT, 0, 0, "Setup X key &mappings", NULL, NULL},
   /* 4 */{d_button_proc, 10,  76, 230,  16, 0, 255, 't', D_EXIT, 0, 0, "&Test X key mappings", NULL, NULL},
   /* 5 */{d_button_proc, 10,  98, 230,  16, 0, 255, 's', D_EXIT, 0, 0, "&Save and exit", NULL, NULL},
   /* 6 */{d_button_proc, 10, 120, 230,  16, 0, 255,  27, D_EXIT, 0, 0, "E&xit", NULL, NULL},
   /* 7 */{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL},
};

static void show_main_dialog(void)
{
   int focus = 1;

   /* Prepare dialog.  */
   set_dialog_color(main_dialog, black, white);
   centre_dialog(main_dialog);

   /* Parse input.  */
   while (1) {
      focus = do_dialog(main_dialog, focus);
      switch (focus) {
	 case 2:
	    /* Load a key config.  */
	    load_keyconfig();
	    break;
	 case 3:
	    /* Setup all key mappings.  */
	    setup_all_keys();
	    break;
	 case 4:
	    /* Test key mappings.  */
	    test_key_map();
	    break;
	 case 5:
	    /* Save and quit.  */
	    save_key_map();
	    return;
	 case 6:
	    /* Just exit.  */
	    return;
      }
   }
}

int main(void)
{
   int i;

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();

   if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      exit(EXIT_FAILURE);
   }
   set_palette(desktop_palette);
   gui_fg_color = palette_color[255];
   gui_bg_color = palette_color[16];

   black = makecol(0, 0, 0);
   white = makecol(255, 255, 255);
   red = makecol(255, 0, 0);
   yellow = makecol(255, 255, 0);

   /* Clear key mappings.  */
   for (i = 0; i < 256; i++)
      keycode_to_scancode[i] = -1;

   /* Hook the X keyboard callback.  */
   _xwin_keyboard_callback = get_raw_keycode;

   show_main_dialog();

   return 0;
}

END_OF_MAIN();

