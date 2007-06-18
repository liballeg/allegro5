/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to access the contents of an
 *    Allegro datafile (created by the grabber utility). The example
 *    loads the file `example.dat', then blits a bitmap and shows
 *    a font, both from this datafile.
 */


#include <allegro.h>


/* the grabber produces this header, which contains defines for the names
 * of all the objects in the datafile (BIG_FONT, SILLY_BITMAP, etc).
 */
#include "example.h"



int main(int argc, char *argv[])
{
   DATAFILE *datafile;
   char buf[256];

   if (allegro_init() != 0)
      return 1;
   install_keyboard();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n",
			 allegro_error);
	 return 1;
      }
   }

   /* we still don't have a palette => Don't let Allegro twist colors */
   set_color_conversion(COLORCONV_NONE);

   /* load the datafile into memory */
   replace_filename(buf, argv[0], "example.dat", sizeof(buf));
   datafile = load_datafile(buf);
   if (!datafile) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s!\n", buf);
      return 1;
   }

   /* select the palette which was loaded from the datafile */
   set_palette(datafile[THE_PALETTE].dat);

   /* aha, set a palette and let Allegro convert colors when blitting */
   set_color_conversion(COLORCONV_TOTAL);
   
   /* display the bitmap from the datafile */
   textout_ex(screen, font, "This is the bitmap:", 32, 16,
	      makecol(255, 255, 255), -1);
   blit(datafile[SILLY_BITMAP].dat, screen, 0, 0, 64, 32, 64, 64);

   /* and use the font from the datafile */
   textout_ex(screen, datafile[BIG_FONT].dat, "And this is a big font!",
	      32, 128, makecol(0, 255, 0), -1);

   readkey();

   /* unload the datafile when we are finished with it */
   unload_datafile(datafile);

   return 0;
}

END_OF_MAIN()
