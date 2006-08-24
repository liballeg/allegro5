/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates the use of double buffering.
 *    It moves a circle across the screen, first just erasing and
 *    redrawing directly to the screen, then with a double buffer.
 */


#include <allegro.h>



int main(void)
{
   BITMAP *buffer;
   int c;

   if (allegro_init() != 0)
      return 1;
   install_timer();
   install_keyboard();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);

   /* allocate the memory buffer */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   /* First without any buffering...
    * Note use of the global retrace_counter to control the speed. We also
    * compensate screen size (GFX_SAFE) with a virtual 320 screen width.
    */
   clear_keybuf();
   c = retrace_count+32;
   while (retrace_count-c <= 320+32) {
      acquire_screen();
      clear_to_color(screen, makecol(255, 255, 255));
      circlefill(screen, (retrace_count-c)*SCREEN_W/320, SCREEN_H/2, 32, makecol(0, 0, 0));
      textprintf_ex(screen, font, 0, 0, makecol(0, 0, 0), -1, "No buffering (%s)",
		    gfx_driver->name);
      release_screen();

      if (keypressed())
	 break;
   }

   /* and now with a double buffer... */
   clear_keybuf();
   c = retrace_count+32;
   while (retrace_count-c <= 320+32) {
      clear_to_color(buffer, makecol(255, 255, 255));
      circlefill(buffer, (retrace_count-c)*SCREEN_W/320, SCREEN_H/2, 32, makecol(0, 0, 0));
      textprintf_ex(buffer, font, 0, 0, makecol(0, 0, 0), -1, "Double buffered (%s)",
		    gfx_driver->name);
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      if (keypressed())
	 break;
   }

   destroy_bitmap(buffer);

   return 0;
}

END_OF_MAIN()
