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
   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);
   text_mode(-1);

   /* allocate the memory buffer */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   /* First without any buffering...
    * Note use of the global retrace_counter to control the speed. We also
    * compensate screen size (GFX_SAFE) with a virtual 320 screen width.
    */
   c = retrace_count+32;
   while (retrace_count-c <= 320+32) {
      acquire_screen();
      clear_to_color(screen, makecol(255, 255, 255));
      circlefill(screen, (retrace_count-c)*SCREEN_W/320, SCREEN_H/2, 32, makecol(0, 0, 0));
      textprintf(screen, font, 0, 0, makecol(0, 0, 0), "No buffering (%s)",
		 gfx_driver->name);
      release_screen();
   }

   /* and now with a double buffer... */
   c = retrace_count+32;
   while (retrace_count-c <= 320+32) {
      clear_to_color(buffer, makecol(255, 255, 255));
      circlefill(buffer, (retrace_count-c)*SCREEN_W/320, SCREEN_H/2, 32, makecol(0, 0, 0));
      textprintf(buffer, font, 0, 0, makecol(0, 0, 0), "Double buffered (%s)",
		 gfx_driver->name);
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }

   destroy_bitmap(buffer);

   return 0;
}

END_OF_MAIN();
