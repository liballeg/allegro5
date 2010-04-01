/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Hiscore table.
 */

#include <stdio.h>
#include <string.h>
#include <allegro.h>

#include "speed.h"



#define NUM_SCORES      8
#define MAX_NAME_LEN    24


/* the score table */
static int scores[NUM_SCORES] =
{
   666, 512, 440, 256, 192, 128, 64, 42
};

static char names[NUM_SCORES][MAX_NAME_LEN+1];

static char yourname[MAX_NAME_LEN+1] = "";



/* initialises the hiscore system */
void init_hiscore()
{
   char buf1[256], buf2[256];
   int i;

   get_executable_name(buf1, sizeof(buf1));
   replace_extension(buf2, buf1, "rec", sizeof(buf2));

   push_config_state();
   set_config_file(buf2);

   for (i=0; i<NUM_SCORES; i++) {
      sprintf(buf1, "score%d", i+1);
      scores[i] = get_config_int("hiscore", buf1, scores[i]);

      sprintf(buf1, "name%d", i+1);
      strncpy(names[i], get_config_string("hiscore", buf1, "Shawn Hargreaves"), MAX_NAME_LEN);
      names[i][MAX_NAME_LEN] = 0;
   }

   pop_config_state();
}



/* shuts down the hiscore system */
void shutdown_hiscore()
{
   char buf1[256], buf2[256];
   int i;

   get_executable_name(buf1, sizeof(buf1));
   replace_extension(buf2, buf1, "rec", sizeof(buf2));

   push_config_state();
   set_config_file(buf2);

   for (i=0; i<NUM_SCORES; i++) {
      sprintf(buf1, "score%d", i+1);
      set_config_int("hiscore", buf1, scores[i]);

      sprintf(buf1, "name%d", i+1);
      set_config_string("hiscore", buf1, names[i]);
   }

   pop_config_state();
}



/* displays the text entry box */
static void draw_entry_box(BITMAP *bmp, int which)
{
   BITMAP *b = create_bitmap(MAX_NAME_LEN*8+16, 16);
   int x;

   clear_to_color(b, makecol(0, 96, 0));
   hline(b, 0, b->h-1, b->w, makecol(0, 32, 0));
   vline(b, b->w-1, 0, b->h, makecol(0, 32, 0));

   textprintf(b, font, 9, 5, makecol(0, 0, 0), "%s", yourname);
   textprintf(b, font, 8, 4, makecol(255, 255, 255), "%s", yourname);

   if (retrace_count & 8) {
      x = strlen(yourname)*8 + 8;
      rectfill(b, x, 12, x+7, 14, makecol(0, 0, 0));
   }

   blit(b, bmp, 0, 0, SCREEN_W/2-56, SCREEN_H/2+(which-NUM_SCORES/2)*16-4, b->w, b->h);

   destroy_bitmap(b);
}



/* displays the score table */
void score_table()
{
   BITMAP *bmp, *b;
   int c, i, j, x, y;
   int myscore = -1;

   for (i=0; i<NUM_SCORES; i++) {
      if (score > scores[i]) {
	 for (j=NUM_SCORES-1; j>i; j--) {
	    scores[j] = scores[j-1];
	    strcpy(names[j], names[j-1]);
	 }

	 scores[i] = score;
	 strcpy(names[i], yourname);

	 myscore = i;
	 break;
      }
   }

   bmp = create_bitmap(SCREEN_W, SCREEN_H);

   if (bitmap_color_depth(bmp) > 8) {
      for (i=0; i<SCREEN_W/2; i++) {
	 vline(bmp, SCREEN_W/2-i-1, 0, SCREEN_H, makecol(0, i*255/(SCREEN_W/2), 0));
	 vline(bmp, SCREEN_W/2+i, 0, SCREEN_H, makecol(0, i*255/(SCREEN_W/2), 0));
      }
   }
   else
      clear_to_color(bmp, makecol(0, 128, 0));

   b = create_bitmap(104, 8);
   clear_to_color(b, bitmap_mask_color(bmp));

   textout(b, font, "HISCORE TABLE", 0, 0, makecol(0, 0, 0));
   stretch_sprite(bmp, b, SCREEN_W/64+4, SCREEN_H/64+4, SCREEN_W*31/32, SCREEN_H/8);
   stretch_sprite(bmp, b, SCREEN_W/64+4, SCREEN_H*55/64+4, SCREEN_W*31/32, SCREEN_H/8);

   textout(b, font, "HISCORE TABLE", 0, 0, makecol(0, 64, 0));
   stretch_sprite(bmp, b, SCREEN_W/64, SCREEN_H/64, SCREEN_W*31/32, SCREEN_H/8);
   stretch_sprite(bmp, b, SCREEN_W/64, SCREEN_H*55/64, SCREEN_W*31/32, SCREEN_H/8);

   destroy_bitmap(b);

   for (i=0; i<NUM_SCORES; i++) {
      y = SCREEN_H/2 + (i-NUM_SCORES/2) * 16;

      textprintf(bmp, font, SCREEN_W/2-142, y+2, makecol(0, 0, 0), "#%d - %d", i+1, scores[i]);
      textprintf(bmp, font, SCREEN_W/2-47, y+1, makecol(0, 0, 0), "%s", names[i]);

      if (i == myscore)
	 c = makecol(255, 0, 0);
      else
	 c = makecol(255, 255, 255);

      textprintf(bmp, font, SCREEN_W/2-144, y, c, "#%d - %d", i+1, scores[i]);
      textprintf(bmp, font, SCREEN_W/2-48, y, c, "%s", names[i]);
   }

   if (myscore >= 0)
      draw_entry_box(bmp, myscore);

   c = retrace_count;

   for (i=0; i<=SCREEN_H/16; i++) {
      acquire_screen();

      for (j=0; j<=16; j++) {
	 x = j*(SCREEN_W/16) + i;
	 blit(bmp, screen, x, 0, x, 0, 1, SCREEN_H);

	 y = j*(SCREEN_H/16) + i;
	 blit(bmp, screen, 0, y, 0, y, SCREEN_W, 1);
      }

      release_screen();

      do {
      } while (retrace_count < c + i*512/SCREEN_W);
   }

   destroy_bitmap(bmp);

   while (joy_b1)
      poll_joystick();

   while ((key[KEY_SPACE]) || (key[KEY_ENTER]) || (key[KEY_ESC]))
      poll_keyboard();

   if (myscore >= 0) {
      clear_keybuf();

      for (;;) {
	 poll_joystick();

	 if ((joy_b1) && (yourname[0])) {
	    strcpy(names[myscore], yourname);
	    break;
	 }

	 if (keypressed()) {
	    c = readkey();

	    if (((c >> 8) == KEY_ENTER) && (yourname[0])) {
	       strcpy(names[myscore], yourname);
	       sfx_explode_player();
	       break;
	    }
	    else if (((c >> 8) == KEY_ESC) && (names[myscore][0])) {
	       strcpy(yourname, names[myscore]);
	       sfx_ping(2);
	       break;
	    }
	    else if (((c >> 8) == KEY_BACKSPACE) && (strlen(yourname) > 0)) {
	       yourname[strlen(yourname)-1] = 0;
	       sfx_shoot();
	    }
	    else if (((c & 0xFF) >= ' ') && ((c & 0xFF) <= '~') && (strlen(yourname) < MAX_NAME_LEN)) {
	       yourname[strlen(yourname)+1] = 0;
	       yourname[strlen(yourname)] = (c & 0xFF);
	       sfx_explode_alien();
	    }
	 }

	 draw_entry_box(screen, myscore);
      }
   }
   else {
      while ((!key[KEY_SPACE]) && (!key[KEY_ENTER]) && (!key[KEY_ESC]) && (!joy_b1)) {
	 poll_joystick();
	 poll_keyboard();
      }

      sfx_ping(2);
   }
}



/* returns the best score, for other modules to display */
int get_hiscore()
{
   return scores[0];
}

