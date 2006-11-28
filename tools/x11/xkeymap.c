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

char unicode_description[256] = EMPTY_STRING;
static DIALOG main_dialog[];
static DIALOG keymap_dialog[];

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

static AL_CONST char *keycode_getter(int index, int *list_size)
{
   if (index >= 0) {
      return scancode_to_name(index + 1);
   }
   else {
      *list_size = KEY_MAX - 1;
      return NULL;
   }
}

static DIALOG keymap_dialog[] = {
   /* 0 */{d_clear_proc,    0,   0, 250, 230, 0,   0,   0, 0,      0, 0, 0, NULL, NULL},
   /* 1 */{d_ctext_proc,  125,  10,   0,  16, 0, 255,   0, 0,      0, 0, "Select Keycode:", NULL, NULL},
   /* 2 */{d_list_proc,     0,  32, 230,  92, 0, 255,   0, D_EXIT, 0, 0, (void *)keycode_getter, NULL, NULL},
   /* 3 */{d_ctext_proc,  125, 142,   0,  16, 0, 255,   0, 0,      0, 0, unicode_description, NULL, NULL},
   /* 4 */{d_button_proc,  10, 164, 230,  16, 0, 255,   0, D_EXIT, 0, 0, "Define &X Key !", NULL, NULL},
   /* 5 */{d_button_proc,  10, 186, 110,  16, 0, 255,  27, D_EXIT, 0, 0, "&Cancel", NULL, NULL},
   /* 6 */{d_button_proc, 130, 186, 110,  16 , 0, 255,  0, D_EXIT, 0, 0, "&Done", NULL, NULL},
   /* 7 */{d_ctext_proc,  125, 208, 0,    16, 0, 255,   0, 0,      0, 0, "", NULL, NULL},
          {d_yield_proc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL},
          {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
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
	       keycode_to_scancode[new_keycode] = keymap_dialog[2].d1 + 1;
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
   for (i = 0; i < KEY_MAX; i++)
      textprintf_ex(screen, font, 32 + (i % 4) * 160,
	      32 + (i / 4) * 14, black, -1, "%s", scancode_to_name (i));
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

      for (i = 0; i < KEY_MAX; i++) {
	 if (key[i])
	    key_was_pressed[i] = key_is_pressed[i] = 1;
	 else
	    key_is_pressed[i] = 0;
      }

      for (i = 0; i < KEY_MAX; i++) {
	 int x = 16 + (i % 4) * 160;
	 int y = 32 + (i / 4) * 14;

	 if (key_is_pressed[i])
	    rectfill(screen, x, y, x + 7, y + 7, red);
	 else if (key_was_pressed[i])
	    rectfill(screen, x, y, x + 7, y + 7, yellow);
	 else
	    rectfill(screen, x, y, x + 7, y + 7, white);
      }

      rest(1);
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

static DIALOG main_dialog[] = {
   /* 0 */{d_clear_proc,   0,   0, 250, 102, 0,   0,   0, 0,      0, 0, 0, NULL, NULL},
   /* 1 */{d_ctext_proc, 120,  10,   0,  16, 0, 255,   0, 0,      0, 0, "X Key Mapping Utility", NULL, NULL},
   /* 2 */{d_ctext_proc,  10,  32, 230,  16, 0, 255,   0, D_EXIT, 0, 0, "~", NULL, NULL},
   /* 3 */{d_button_proc, 10,  54, 230,  16, 0, 255, 'm', D_EXIT, 0, 0, "Setup X key &mappings", NULL, NULL},
   /* 4 */{d_button_proc, 10,  76, 230,  16, 0, 255, 't', D_EXIT, 0, 0, "&Test X key mappings", NULL, NULL},
   /* 5 */{d_button_proc, 10,  98, 230,  16, 0, 255, 's', D_EXIT, 0, 0, "&Save and exit", NULL, NULL},
   /* 6 */{d_button_proc, 10, 120, 230,  16, 0, 255,  27, D_EXIT, 0, 0, "E&xit", NULL, NULL},
          {d_yield_proc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL},
          {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
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

END_OF_MAIN()

