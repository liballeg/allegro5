/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates the use of double buffering.
 *    It moves a circle across the screen, first just erasing and
 *    redrawing directly to the screen, then with a double buffer.
 */


#include "allegro.h"



int main()
{
   BITMAP *buffer;
   int c;

   allegro_init();
   install_timer();
   set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);
   set_palette(desktop_palette);

   /* allocate the memory buffer */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   /* First without any buffering...
    * Note use of the global retrace_counter to control the speed.
    */
   c = retrace_count+32;
   while (retrace_count-c <= SCREEN_W+32) {
      acquire_screen();
      clear(screen);
      circlefill(screen, retrace_count-c, SCREEN_H/2, 32, 255);
      textprintf(screen, font, 0, 0, 255, "No buffering (%s)", gfx_driver->name);
      release_screen();
   }

   /* and now with a double buffer... */
   c = retrace_count+32;
   while (retrace_count-c <= SCREEN_W+32) {
      clear(buffer);
      circlefill(buffer, retrace_count-c, SCREEN_H/2, 32, 255);
      textprintf(buffer, font, 0, 0, 255, "Double buffered (%s)", gfx_driver->name);
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }

   destroy_bitmap(buffer);

   return 0;
}

END_OF_MAIN();
