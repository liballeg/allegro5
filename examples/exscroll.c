/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use hardware scrolling and split
 *    screens in mode-X. The split screen part only works on DOS and Linux
 *    console platforms, but the scrolling should work on anything that
 *    supports large virtual screens.
 */


#include <stdio.h>

#include "allegro.h"



int main()
{
   BITMAP *scroller, *status_bar;
   int counter = 0;
   char tmp[80];
   RGB black = { 0, 0, 0, 0 };
   int x = 0;
   int next_x;
   int h = 100;
   int split_h = 0;

   allegro_init();
   install_keyboard();

   #ifdef GFX_MODEX

      /* create a nice wide virtual screen, and split it at line 200 */
      if (set_gfx_mode(GFX_MODEX, 320, 240, 640, 240) != 0) {
	 if (set_gfx_mode(GFX_AUTODETECT, 320, 240, 640, 240) != 0) {
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	    allegro_message("Unable to set a 320x240 mode with 640x240 virtual dimensions\n");
	    return 1;
	 }
      }

      if (gfx_driver->id == GFX_MODEX) {
	 split_modex_screen(200);
	 scroll_screen(0, 40);
	 split_h = 40;
      }

   #else

      /* no mode-X on this platform, so just make do the best we can */
      if (set_gfx_mode(GFX_AUTODETECT, 320, 240, 640, 240) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set a 320x240 mode with 640x240 virtual dimensions\n");
	 return 1;
      }

   #endif

   /* the scrolling area is sized 640x200 and starts at line 40 */
   scroller = create_sub_bitmap(screen, 0, split_h, SCREEN_W*2, SCREEN_H-split_h);

   /* the status bar is sized 320x40 and starts at line 0 */
   if (split_h > 0)
      status_bar = create_sub_bitmap(screen, 0, 0, SCREEN_W, split_h);
   else
      status_bar = NULL;

   set_palette(desktop_palette);
   set_color(0, &black);

   if (status_bar)
      textout(status_bar, font, "This area isn't scrolling", 8, 8, 1);

   rectfill(scroller, 0, 0, SCREEN_W, 100, 6);
   rectfill(scroller, 0, 100, SCREEN_W, SCREEN_H-split_h, 2);

   do {
      /* update the status bar */
      if (status_bar) {
	 sprintf(tmp, "Counter = %d", counter++);
	 acquire_bitmap(status_bar);
	 textout(status_bar, font, tmp, 8, 20, 1);
	 release_bitmap(status_bar);
      }

      /* advance the scroller, wrapping every 320 pixels */
      next_x = x + 1;
      if (next_x >= 320)
	 next_x = 0;

      /* draw another column of the landscape */
      acquire_bitmap(scroller);
      vline(scroller, next_x+SCREEN_W-1, 0, h, 6);
      vline(scroller, next_x+SCREEN_W-1, h+1, SCREEN_H-split_h, 2);
      release_bitmap(scroller);

      /* scroll the screen */
      scroll_screen(next_x, split_h);

      /* duplicate the landscape column so we can wrap the scroller */
      if (next_x > 0) {
	 acquire_bitmap(scroller);
	 vline(scroller, x, 0, h, 6);
	 vline(scroller, x, h+1, SCREEN_H-split_h, 2);
	 release_bitmap(scroller);
      }

      /* randomly alter the landscape position */
      if (rand()&1) {
	 if (h > 5)
	    h--;
      }
      else {
	 if (h < 195)
	    h++;
      }

      x = next_x;

   } while (!keypressed());

   destroy_bitmap(scroller);

   if (status_bar)
      destroy_bitmap(status_bar);

   clear_keybuf();

   return 0;
}

END_OF_MAIN();
