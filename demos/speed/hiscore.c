/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Hiscore table.
 */

#include <stdio.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

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
   ALLEGRO_PATH *path;
   ALLEGRO_CONFIG *cfg;
   char buf1[256];
   int i;

   path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
   if (!path) return;
   al_make_directory(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
   
   al_set_path_filename(path, "speed.rec");

   cfg = al_load_config_file(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
   if (!cfg)
      cfg = al_create_config();

   for (i=0; i<NUM_SCORES; i++) {
      sprintf(buf1, "score%d", i+1);
      scores[i] = get_config_int(cfg, "hiscore", buf1, scores[i]);

      sprintf(buf1, "name%d", i+1);
      strncpy(names[i], get_config_string(cfg, "hiscore", buf1, "Shawn Hargreaves"), MAX_NAME_LEN);
      names[i][MAX_NAME_LEN] = 0;
   }

   al_destroy_config(cfg);
   al_destroy_path(path);
}



/* shuts down the hiscore system */
void shutdown_hiscore()
{
   ALLEGRO_PATH *path;
   ALLEGRO_CONFIG *cfg;
   char buf1[256];
   int i;

   path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
   if (!path) return;
   al_make_directory(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));

   al_set_path_filename(path, "speed.rec");

   cfg = al_create_config();

   for (i=0; i<NUM_SCORES; i++) {
      sprintf(buf1, "score%d", i+1);
      set_config_int(cfg, "hiscore", buf1, scores[i]);

      sprintf(buf1, "name%d", i+1);
      al_set_config_value(cfg, "hiscore", buf1, names[i]);
   }

   al_save_config_file(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP), cfg);

   al_destroy_config(cfg);
   al_destroy_path(path);
}



/* displays the text entry box */
static void draw_entry_box(int which, int64_t retrace_count)
{
   const int w = MAX_NAME_LEN*8+16;
   const int h = 16;
   ALLEGRO_BITMAP *b;
   ALLEGRO_BITMAP *screen;
   int SCREEN_W, SCREEN_H;
   int x;

   screen = al_get_target_bitmap();
   SCREEN_W = al_get_bitmap_width(screen);
   SCREEN_H = al_get_bitmap_height(screen);

   b = create_memory_bitmap(w, h);
   al_set_target_bitmap(b);
   al_clear_to_color(makecol(0, 96, 0));
   hline(0, w, h-1, makecol(0, 32, 0));
   vline(0, w-1, h, makecol(0, 32, 0));

   textprintf(font, 9, 5, makecol(0, 0, 0), "%s", yourname);
   textprintf(font, 8, 4, makecol(255, 255, 255), "%s", yourname);

   if (retrace_count & 8) {
      x = strlen(yourname)*8 + 8;
      rectfill(x, 12, x+7, 14, makecol(0, 0, 0));
   }

   al_set_target_bitmap(screen);
   al_draw_bitmap(b, SCREEN_W/2-56, SCREEN_H/2+(which-NUM_SCORES/2)*16-4, 0);

   al_destroy_bitmap(b);
}



/* displays the score table */
void score_table()
{
   int SCREEN_W = al_get_display_width(screen);
   int SCREEN_H = al_get_display_height(screen);
   ALLEGRO_BITMAP *bmp, *b;
   ALLEGRO_COLOR col;
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

   bmp = create_memory_bitmap(SCREEN_W, SCREEN_H);
   al_set_target_bitmap(bmp);

   for (i=0; i<SCREEN_W/2; i++) {
      vline(SCREEN_W/2-i-1, 0, SCREEN_H, makecol(0, i*255/(SCREEN_W/2), 0));
      vline(SCREEN_W/2+i, 0, SCREEN_H, makecol(0, i*255/(SCREEN_W/2), 0));
   }

   b = create_memory_bitmap(104, 8);
   al_set_target_bitmap(b);
   al_clear_to_color(al_map_rgba(0, 0, 0, 0));

   textout(font, "HISCORE TABLE", 0, 0, makecol(0, 0, 0));
   stretch_sprite(bmp, b, SCREEN_W/64+4, SCREEN_H/64+4, SCREEN_W*31/32, SCREEN_H/8);
   stretch_sprite(bmp, b, SCREEN_W/64+4, SCREEN_H*55/64+4, SCREEN_W*31/32, SCREEN_H/8);

   textout(font, "HISCORE TABLE", 0, 0, makecol(0, 64, 0));
   stretch_sprite(bmp, b, SCREEN_W/64, SCREEN_H/64, SCREEN_W*31/32, SCREEN_H/8);
   stretch_sprite(bmp, b, SCREEN_W/64, SCREEN_H*55/64, SCREEN_W*31/32, SCREEN_H/8);

   al_destroy_bitmap(b);
   al_set_target_bitmap(bmp);

   for (i=0; i<NUM_SCORES; i++) {
      y = SCREEN_H/2 + (i-NUM_SCORES/2) * 16;

      textprintf(font, SCREEN_W/2-142, y+2, makecol(0, 0, 0), "#%d - %d", i+1, scores[i]);
      textprintf(font, SCREEN_W/2-47, y+1, makecol(0, 0, 0), "%s", names[i]);

      if (i == myscore)
	 col = makecol(255, 0, 0);
      else
	 col = makecol(255, 255, 255);

      textprintf(font, SCREEN_W/2-144, y, col, "#%d - %d", i+1, scores[i]);
      textprintf(font, SCREEN_W/2-48, y, col, "%s", names[i]);
   }

   if (myscore >= 0)
      draw_entry_box(myscore, 0);

   bmp = replace_bitmap(bmp);

   al_set_target_bitmap(al_get_backbuffer(screen));

   start_retrace_count();

   for (i=0; i<=SCREEN_H/16; i++) {
      al_clear_to_color(makecol(0, 0, 0));

      for (j=0; j<=16; j++) {
	 x = j*(SCREEN_W/16);
	 al_draw_bitmap_region(bmp, x, 0, i, SCREEN_H, x, 0, 0);

	 y = j*(SCREEN_H/16);
	 al_draw_bitmap_region(bmp, 0, y, SCREEN_W, i, 0, y, 0);
      }

      al_flip_display();

      do {
	 poll_input_wait();
      } while (retrace_count() < i*512/SCREEN_W);
   }

   while (joy_b1 || key[ALLEGRO_KEY_SPACE] || key[ALLEGRO_KEY_ENTER] || key[ALLEGRO_KEY_ESCAPE])
      poll_input_wait();

   if (myscore >= 0) {
      clear_keybuf();

      for (;;) {
	 poll_input_wait();

	 if ((joy_b1) && (yourname[0])) {
	    strcpy(names[myscore], yourname);
	    break;
	 }

	 if (keypressed()) {
	    c = readkey();

	    if (((c >> 8) == ALLEGRO_KEY_ENTER) && (yourname[0])) {
	       strcpy(names[myscore], yourname);
	       sfx_explode_player();
	       break;
	    }
	    else if (((c >> 8) == ALLEGRO_KEY_ESCAPE) && (names[myscore][0])) {
	       strcpy(yourname, names[myscore]);
	       sfx_ping(2);
	       break;
	    }
	    else if (((c >> 8) == ALLEGRO_KEY_BACKSPACE) && (strlen(yourname) > 0)) {
	       yourname[strlen(yourname)-1] = 0;
	       sfx_shoot();
	    }
	    else if (((c & 0xFF) >= ' ') && ((c & 0xFF) <= '~') && (strlen(yourname) < MAX_NAME_LEN)) {
	       yourname[strlen(yourname)+1] = 0;
	       yourname[strlen(yourname)] = (c & 0xFF);
	       sfx_explode_alien();
	    }
	 }

	 al_draw_bitmap(bmp, 0, 0, 0);
	 draw_entry_box(myscore, retrace_count());
	 al_flip_display();
      }
   }
   else {
      while (!key[ALLEGRO_KEY_SPACE] && !key[ALLEGRO_KEY_ENTER] && !key[ALLEGRO_KEY_ESCAPE] && !joy_b1) {
	 poll_input_wait();
	 al_draw_bitmap(bmp, 0, 0, 0);
	 al_flip_display();
      }

      sfx_ping(2);
   }

   stop_retrace_count();

   al_destroy_bitmap(bmp);
}



/* returns the best score, for other modules to display */
int get_hiscore()
{
   return scores[0];
}

