/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to load and display a bitmap
 *    file.  You have to use this example from the command line to
 *    specify as first parameter a graphic file in one of Allegro's
 *    supported formats.  If the file is loaded successfully,
 *    it will be displayed until you press a key.
 */


#include <allegro.h>



int main(int argc, char *argv[])
{
   BITMAP *the_image;
   PALETTE the_palette;

   if (allegro_init() != 0)
      return 1;

   if (argc != 2) {
      allegro_message("Usage: 'exbitmap filename.[bmp|lbm|pcx|tga]'\n");
      return 1;
   }

   install_keyboard();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   /* read in the bitmap file */
   the_image = load_bitmap(argv[1], the_palette);
   if (!the_image) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading bitmap file '%s'\n", argv[1]);
      return 1;
   }

   /* select the bitmap palette */
   set_palette(the_palette);

   /* blit the image onto the screen */
   blit(the_image, screen, 0, 0, (SCREEN_W-the_image->w)/2,
	(SCREEN_H-the_image->h)/2, the_image->w, the_image->h);

   /* destroy the bitmap */
   destroy_bitmap(the_image);

   readkey();
   return 0;
}

END_OF_MAIN()
