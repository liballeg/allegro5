/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use hardware scrolling.
 *    The scrolling should work on anything that supports virtual
 *    screens larger than the physical screen.
 */


#include <stdio.h>

#include <allegro.h>



int main(void)
{
   BITMAP *scroller;
   RGB black = { 0, 0, 0, 0 };
   int x = 0;
   int next_x;
   int h = 100;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 240, 640, 240) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set a 320x240 mode with 640x240 "
		      "virtual dimensions\n");
      return 1;
   }

   /* the scrolling area is twice the width of the screen (640x240) */
   scroller = create_sub_bitmap(screen, 0, 0, SCREEN_W*2, SCREEN_H);

   set_palette(desktop_palette);
   set_color(0, &black);

   rectfill(scroller, 0, 0, SCREEN_W, 100, 6);
   rectfill(scroller, 0, 100, SCREEN_W, SCREEN_H, 2);

   do {
      /* advance the scroller, wrapping every 320 pixels */
      next_x = x + 1;
      if (next_x >= 320)
	 next_x = 0;

      /* draw another column of the landscape */
      acquire_bitmap(scroller);
      vline(scroller, next_x+SCREEN_W-1, 0, h, 6);
      vline(scroller, next_x+SCREEN_W-1, h+1, SCREEN_H, 2);
      release_bitmap(scroller);

      /* scroll the screen */
      scroll_screen(next_x, 0);

      /* duplicate the landscape column so we can wrap the scroller */
      if (next_x > 0) {
	 acquire_bitmap(scroller);
	 vline(scroller, x, 0, h, 6);
	 vline(scroller, x, h+1, SCREEN_H, 2);
	 release_bitmap(scroller);
      }

      /* randomly alter the landscape position */
      if (AL_RAND()&1) {
	 if (h > 5)
	    h--;
      }
      else {
	 if (h < 195)
	    h++;
      }

      x = next_x;

      rest(0);
   } while (!keypressed());

   destroy_bitmap(scroller);

   clear_keybuf();

   return 0;
}

END_OF_MAIN()
