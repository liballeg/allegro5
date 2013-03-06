/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Title screen and results display.
 */

#include <math.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "speed.h"



/* draws text with a dropshadow */
static void textout_shadow(char *msg, int x, int y, ALLEGRO_COLOR c)
{
   textout_centre(font, msg, x+1, y+1, makecol(0, 0, 0));
   textout_centre(font, msg, x, y, c);
}



/* display the title screen */
int title_screen()
{
   int SCREEN_W = al_get_display_width(screen);
   int SCREEN_H = al_get_display_height(screen);
   ALLEGRO_BITMAP *bmp, *b;
   ALLEGRO_COLOR white = makecol(255, 255, 255);
   int i, j, y;

   bmp = create_memory_bitmap(SCREEN_W, SCREEN_H);
   al_set_target_bitmap(bmp);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

   for (i=0; i<SCREEN_H/2; i++) {
      hline(0, i, SCREEN_W, makecol(0, 0, i*255/(SCREEN_H/2)));
      hline(0, SCREEN_H-i-1, SCREEN_W, makecol(0, 0, i*255/(SCREEN_H/2)));
   }

   solid_mode();

   b = create_memory_bitmap(40, 8);
   al_set_target_bitmap(b);
   al_clear_to_color(al_map_rgba(0, 0, 0, 0));

   textout(font, "SPEED", 0, 0, makecol(0, 0, 0));
   stretch_sprite(bmp, b, SCREEN_W/128+8, SCREEN_H/24+8, SCREEN_W, SCREEN_H);

   textout(font, "SPEED", 0, 0, makecol(0, 0, 64));
   stretch_sprite(bmp, b, SCREEN_W/128, SCREEN_H/24, SCREEN_W, SCREEN_H);

   al_set_target_bitmap(bmp);

   al_destroy_bitmap(b);

   textout_shadow("Simultaneous Projections", SCREEN_W/2, SCREEN_H/2-80, white);
   textout_shadow("Employing an Ensemble of Displays", SCREEN_W/2, SCREEN_H/2-64, white);

   textout_shadow("Or alternatively: Stupid Pointless", SCREEN_W/2, SCREEN_H/2-32, white);
   textout_shadow("Effort at Establishing a Dumb Acronym", SCREEN_W/2, SCREEN_H/2-16, white);

   textout_shadow("By Shawn Hargreaves, 1999", SCREEN_W/2, SCREEN_H/2+16, white);
   textout_shadow("Written for the Allegro", SCREEN_W/2, SCREEN_H/2+48, white);
   textout_shadow("SpeedHack competition", SCREEN_W/2, SCREEN_H/2+64, white);

   al_set_target_bitmap(al_get_backbuffer(screen));

   bmp = replace_bitmap(bmp);

   start_retrace_count();

   for (i=0; i<=SCREEN_H/16; i++) {
      al_clear_to_color(makecol(0, 0, 0));

      for (j=0; j<=16; j++) {
	 y = j*(SCREEN_H/16);
	 al_draw_bitmap_region(bmp, 0, y, SCREEN_W, i, 0, y, 0);
      }

      al_flip_display();

      do {
	 poll_input_wait();
      } while (retrace_count() < i*1024/SCREEN_W);
   }

   stop_retrace_count();

   while (joy_b1 || key[ALLEGRO_KEY_SPACE] || key[ALLEGRO_KEY_ENTER] || key[ALLEGRO_KEY_ESCAPE])
      poll_input_wait();

   while (!key[ALLEGRO_KEY_SPACE] && !key[ALLEGRO_KEY_ENTER] && !key[ALLEGRO_KEY_ESCAPE] && !joy_b1) {
      poll_input_wait();
      al_draw_bitmap(bmp, 0, 0, 0);
      al_flip_display();
   }

   al_destroy_bitmap(bmp);

   if (key[ALLEGRO_KEY_ESCAPE])
      return FALSE;

   sfx_ping(2);

   return TRUE;
}



/* display the results screen */
void show_results()
{
   int SCREEN_W = al_get_display_width(screen);
   int SCREEN_H = al_get_display_height(screen);
   ALLEGRO_BITMAP *bmp, *b;
   char buf[80];
   int i, j, x;

   bmp = create_memory_bitmap(SCREEN_W, SCREEN_H);
   al_set_target_bitmap(bmp);

   for (i=0; i<SCREEN_H/2; i++) {
      hline(0, SCREEN_H/2-i-1, SCREEN_W, makecol(i*255/(SCREEN_H/2), 0, 0));
      hline(0, SCREEN_H/2+i, SCREEN_W, makecol(i*255/(SCREEN_H/2), 0, 0));
   }

   b = create_memory_bitmap(72, 8);
   al_set_target_bitmap(b);
   al_clear_to_color(al_map_rgba(0, 0, 0, 0));

   textout(font, "GAME OVER", 0, 0, makecol(0, 0, 0));
   stretch_sprite(bmp, b, 4, SCREEN_H/3+4, SCREEN_W, SCREEN_H/3);

   textout(font, "GAME OVER", 0, 0, makecol(64, 0, 0));
   stretch_sprite(bmp, b, 0, SCREEN_H/3, SCREEN_W, SCREEN_H/3);

   al_destroy_bitmap(b);

   al_set_target_bitmap(bmp);
   sprintf(buf, "Score: %d", score);
   textout_shadow(buf, SCREEN_W/2, SCREEN_H*3/4, makecol(255, 255, 255));

   al_set_target_bitmap(al_get_backbuffer(screen));

   bmp = replace_bitmap(bmp);

   start_retrace_count();

   for (i=0; i<=SCREEN_W/16; i++) {
      al_clear_to_color(makecol(0, 0, 0));

      for (j=0; j<=16; j++) {
	 x = j*(SCREEN_W/16);
	 al_draw_bitmap_region(bmp, x, 0, i, SCREEN_H, x, 0, 0);
      }

      al_flip_display();

      do {
	 poll_input_wait();
      } while (retrace_count() < i*1024/SCREEN_W);
   }

   stop_retrace_count();

   while (joy_b1 || key[ALLEGRO_KEY_SPACE] || key[ALLEGRO_KEY_ENTER] || key[ALLEGRO_KEY_ESCAPE])
      poll_input_wait();

   while (!key[ALLEGRO_KEY_SPACE] && !key[ALLEGRO_KEY_ENTER] && !key[ALLEGRO_KEY_ESCAPE] && !joy_b1) {
      poll_input_wait();
      al_draw_bitmap(bmp, 0, 0, 0);
      al_flip_display();
   }

   al_destroy_bitmap(bmp);

   sfx_ping(2);
}



/* print the shutdown message */
void goodbye()
{
   static int data1[] =
   {
      0, 2, 0, 1, 2, 3, 0, 3, 5, 3, 4, 6, 0, 2, 0, 1, 2,
      3, 0, 3, 7, 3, 5, 6, 0, 2, 0, 1, 12, 3, 9, 3, 5, 3,
      4, 3, 2, 3, 10, 2, 10, 1, 9, 3, 5, 3, 7, 3, 5, 9

   };

   static int data2[] =
   {
      12, 3, 7, 1, 6, 1, 7, 1, 8, 3, 7, 3
   };

   ALLEGRO_SAMPLE *s1, *s2;
   ALLEGRO_SAMPLE_ID id;
   ALLEGRO_BITMAP *b;
   ALLEGRO_BITMAP *screen_backbuffer;
   char *sdata1, *sdata2;
   int SCREEN_W, SCREEN_H;
   int i;

   if (!al_is_audio_installed()) {
      al_destroy_display(screen);
      screen = NULL;
      printf("Couldn't install a digital sound driver, so no closing tune is available.\n");
      return;
   }

   s1 = create_sample_u8(44100, 256);
   s2 = create_sample_u8(44100, 256);

   sdata1 = (char *)al_get_sample_data(s1);
   sdata2 = (char *)al_get_sample_data(s2);

   for (i=0; i<256; i++) {
      sdata1[i] = i;
      sdata2[i] = (i < 128) ? 255 : 0;
   }

   SCREEN_W = al_get_display_width(screen);
   SCREEN_H = al_get_display_height(screen);

   rectfill(0, 0, SCREEN_W/2, SCREEN_H/2, makecol(255, 255, 0));
   rectfill(SCREEN_W/2, 0, SCREEN_W, SCREEN_H/2, makecol(0, 255, 0));
   rectfill(0, SCREEN_H/2, SCREEN_W/2, SCREEN_H, makecol(0, 0, 255));
   rectfill(SCREEN_W/2, SCREEN_H/2, SCREEN_W, SCREEN_H, makecol(255, 0, 0));

   b = al_create_bitmap(168, 8);
   al_set_target_bitmap(b);
   al_clear_to_color(al_map_rgba(0, 0, 0, 0));

   screen_backbuffer = al_get_backbuffer(screen);

   textout(font, "Happy birthday Arron!", 0, 0, makecol(0, 0, 0));
   stretch_sprite(screen_backbuffer, b, SCREEN_W/8+4, SCREEN_H*3/8+4, SCREEN_W*3/4, SCREEN_H/4);

   textout(font, "Happy birthday Arron!", 0, 0, makecol(255, 255, 255));
   stretch_sprite(screen_backbuffer, b, SCREEN_W/8, SCREEN_H*3/8, SCREEN_W*3/4, SCREEN_H/4);

   al_set_target_bitmap(screen_backbuffer);
   al_destroy_bitmap(b);

   al_flip_display();

   while (key[ALLEGRO_KEY_SPACE] || key[ALLEGRO_KEY_ENTER] || key[ALLEGRO_KEY_ESCAPE])
      poll_input_wait();

   clear_keybuf();

   for (i=0; i < (int)(sizeof(data1)/sizeof(int)); i += 2) {
      al_play_sample(s1, 64/255.0, 0.0, pow(2.0, (float)data1[i]/12.0),
		     ALLEGRO_PLAYMODE_LOOP, &id);
      rest(100*data1[i+1]);
      al_stop_sample(&id);
      rest(50*data1[i+1]);

      if (keypressed())
	 return;
   }

   rest(500);

   al_destroy_display(screen);
   screen = NULL;
   printf("\nAnd thanks for organising this most excellent competition...\n");

   for (i=0; i < (int)(sizeof(data2)/sizeof(int)); i += 2) {
      al_play_sample(s2, 64/255.0, 0.0, pow(2.0, (float)data2[i]/12.0),
		     ALLEGRO_PLAYMODE_LOOP, &id);
      rest(75*data2[i+1]);
      al_stop_sample(&id);
      rest(25*data2[i+1]);

      if (keypressed())
	 return;
   }

   rest(300);
   putchar('\007'); fflush(stdout);
   rest(300);
   putchar('\007'); fflush(stdout);

   al_stop_samples();

   al_destroy_sample(s1);
   al_destroy_sample(s2);
}

