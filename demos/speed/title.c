/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Title screen and results display.
 */

#include <math.h>
#include <stdio.h>
#include <allegro.h>

#include "speed.h"



/* draws text with a dropshadow */
static void textout_shadow(BITMAP *bmp, char *msg, int x, int y, int c)
{
   textout_centre(bmp, font, msg, x+1, y+1, makecol(0, 0, 0));
   textout_centre(bmp, font, msg, x, y, c);
}



/* display the title screen */
int title_screen()
{
   BITMAP *bmp, *b;
   int c, i, j, y;

   bmp = create_bitmap(SCREEN_W, SCREEN_H);

   if (bitmap_color_depth(bmp) > 8) {
      for (i=0; i<SCREEN_H/2; i++) {
	 hline(bmp, 0, i, SCREEN_W, makecol(0, 0, i*255/(SCREEN_H/2)));
	 hline(bmp, 0, SCREEN_H-i-1, SCREEN_W, makecol(0, 0, i*255/(SCREEN_H/2)));
      }
   }
   else
      clear_to_color(bmp, makecol(0, 0, 128));

   b = create_bitmap(40, 8);
   clear_to_color(b, bitmap_mask_color(bmp));

   textout(b, font, "SPEED", 0, 0, makecol(0, 0, 0));
   stretch_sprite(bmp, b, SCREEN_W/128+8, SCREEN_H/24+8, SCREEN_W, SCREEN_H);

   textout(b, font, "SPEED", 0, 0, makecol(0, 0, 64));
   stretch_sprite(bmp, b, SCREEN_W/128, SCREEN_H/24, SCREEN_W, SCREEN_H);

   destroy_bitmap(b);

   textout_shadow(bmp, "Simultaneous Projections", SCREEN_W/2, SCREEN_H/2-80, makecol(255, 255, 255));
   textout_shadow(bmp, "Employing an Ensemble of Displays", SCREEN_W/2, SCREEN_H/2-64, makecol(255, 255, 255));

   textout_shadow(bmp, "Or alternatively: Stupid Pointless", SCREEN_W/2, SCREEN_H/2-32, makecol(255, 255, 255));
   textout_shadow(bmp, "Effort at Establishing a Dumb Acronym", SCREEN_W/2, SCREEN_H/2-16, makecol(255, 255, 255));

   textout_shadow(bmp, "By Shawn Hargreaves, 1999", SCREEN_W/2, SCREEN_H/2+16, makecol(255, 255, 255));
   textout_shadow(bmp, "Written for the Allegro", SCREEN_W/2, SCREEN_H/2+48, makecol(255, 255, 255));
   textout_shadow(bmp, "SpeedHack competition", SCREEN_W/2, SCREEN_H/2+64, makecol(255, 255, 255));

   c = retrace_count;

   for (i=0; i<=SCREEN_H/16; i++) {
      acquire_screen();

      for (j=0; j<=16; j++) {
	 y = j*(SCREEN_H/16) + i;
	 blit(bmp, screen, 0, y, 0, y, SCREEN_W, 1);
      }

      release_screen();

      do {
      } while (retrace_count < c + i*256/SCREEN_H);
   }

   destroy_bitmap(bmp);

   while (joy_b1)
      poll_joystick();

   while ((key[KEY_SPACE]) || (key[KEY_ENTER]) || (key[KEY_ESC]))
      poll_keyboard();

   while ((!key[KEY_SPACE]) && (!key[KEY_ENTER]) && (!key[KEY_ESC]) && (!joy_b1)) {
      poll_joystick();
      poll_keyboard();
   }

   if (key[KEY_ESC])
      return FALSE;

   sfx_ping(2);

   return TRUE;
}



/* display the results screen */
void show_results()
{
   char buf[80];
   BITMAP *bmp, *b;
   int c, i, j, x;

   bmp = create_bitmap(SCREEN_W, SCREEN_H);

   if (bitmap_color_depth(bmp) > 8) {
      for (i=0; i<SCREEN_H/2; i++) {
	 hline(bmp, 0, SCREEN_H/2-i-1, SCREEN_W, makecol(i*255/(SCREEN_H/2), 0, 0));
	 hline(bmp, 0, SCREEN_H/2+i, SCREEN_W, makecol(i*255/(SCREEN_H/2), 0, 0));
      }
   }
   else
      clear_to_color(bmp, makecol(128, 0, 0));

   b = create_bitmap(72, 8);
   clear_to_color(b, bitmap_mask_color(bmp));

   textout(b, font, "GAME OVER", 0, 0, makecol(0, 0, 0));
   stretch_sprite(bmp, b, 4, SCREEN_H/3+4, SCREEN_W, SCREEN_H/3);

   textout(b, font, "GAME OVER", 0, 0, makecol(64, 0, 0));
   stretch_sprite(bmp, b, 0, SCREEN_H/3, SCREEN_W, SCREEN_H/3);

   destroy_bitmap(b);

   sprintf(buf, "Score: %d", score);
   textout_shadow(bmp, buf, SCREEN_W/2, SCREEN_H*3/4, makecol(255, 255, 255));

   c = retrace_count;

   for (i=0; i<=SCREEN_W/16; i++) {
      acquire_screen();

      for (j=0; j<=16; j++) {
	 x = j*(SCREEN_W/16) + i;
	 blit(bmp, screen, x, 0, x, 0, 1, SCREEN_H);
      }

      release_screen();

      do {
      } while (retrace_count < c + i*1024/SCREEN_W);
   }

   destroy_bitmap(bmp);

   while (joy_b1)
      poll_joystick();

   while ((key[KEY_SPACE]) || (key[KEY_ENTER]) || (key[KEY_ESC]))
      poll_keyboard();

   while ((!key[KEY_SPACE]) && (!key[KEY_ENTER]) && (!key[KEY_ESC]) && (!joy_b1)) {
      poll_joystick();
      poll_keyboard();
   }

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

   SAMPLE *s1, *s2;
   BITMAP *b;
   int i;

   if (digi_driver->id == DIGI_NONE) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Couldn't install a digital sound driver, so no closing tune is available.\n");
      return;
   }

   s1 = create_sample(8, FALSE, 44100, 256);
   s2 = create_sample(8, FALSE, 44100, 256);

   for (i=0; i<256; i++) {
      ((char *)s1->data)[i] = i;
      ((char *)s2->data)[i] = (i < 128) ? 255 : 0;
   }

   rectfill(screen, 0, 0, SCREEN_W/2, SCREEN_H/2, makecol(255, 255, 0));
   rectfill(screen, SCREEN_W/2, 0, SCREEN_W, SCREEN_H/2, makecol(0, 255, 0));
   rectfill(screen, 0, SCREEN_H/2, SCREEN_W/2, SCREEN_H, makecol(0, 0, 255));
   rectfill(screen, SCREEN_W/2, SCREEN_H/2, SCREEN_W, SCREEN_H, makecol(255, 0, 0));

   b = create_bitmap(168, 8);
   clear_to_color(b, bitmap_mask_color(b));

   textout(b, font, "Happy birthday Arron!", 0, 0, makecol(0, 0, 0));
   stretch_sprite(screen, b, SCREEN_W/8+4, SCREEN_H*3/8+4, SCREEN_W*3/4, SCREEN_H/4);

   textout(b, font, "Happy birthday Arron!", 0, 0, makecol(255, 255, 255));
   stretch_sprite(screen, b, SCREEN_W/8, SCREEN_H*3/8, SCREEN_W*3/4, SCREEN_H/4);

   destroy_bitmap(b);

   while ((key[KEY_SPACE]) || (key[KEY_ENTER]) | (key[KEY_ESC]))
      poll_keyboard();

   clear_keybuf();

   for (i=0; i < (int)(sizeof(data1)/sizeof(int)); i += 2) {
      play_sample(s1, 64, 128, (int)(1000 * pow(2.0, (float)data1[i]/12.0)), TRUE);
      rest(100*data1[i+1]);
      stop_sample(s1);
      rest(50*data1[i+1]);

      if (keypressed())
	 return;
   }

   rest(500);

   clear(screen);
   set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
   allegro_message("\nAnd thanks for organising this most excellent competition...\n");

   for (i=0; i < (int)(sizeof(data2)/sizeof(int)); i += 2) {
      play_sample(s2, 64, 128, (int)(1000 * pow(2.0, (float)data2[i]/12.0)), TRUE);
      rest(75*data2[i+1]);
      stop_sample(s2);
      rest(25*data2[i+1]);

      if (keypressed())
	 return;
   }

   rest(300);
   putchar('\007'); fflush(stdout);
   rest(300);
   putchar('\007'); fflush(stdout);

   destroy_sample(s1);
   destroy_sample(s2);
}

