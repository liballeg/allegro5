/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program shows how to specify colors in the various different
 *    truecolor pixel formats.
 */


#include "allegro.h"



void test(int colordepth)
{
   PALETTE pal;
   int x;

   /* set the screen mode */
   set_color_depth(colordepth);

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0)
      return;

   /* in case this is a 256 color mode, we'd better make sure that the
    * palette is set to something sensible. This function generates a
    * standard palette with a nice range of different colors...
    */
   generate_332_palette(pal);
   set_palette(pal);

   acquire_screen();

   clear_to_color(screen, makecol(0, 0, 0));

   text_mode(-1);
   textprintf(screen, font, 0, 0, makecol(255, 255, 255), "%d bit color...", colordepth);

   /* use the makecol() function to specify RGB values... */
   textout(screen, font, "Red",     32, 80,  makecol(255, 0,   0  ));
   textout(screen, font, "Green",   32, 100, makecol(0,   255, 0  ));
   textout(screen, font, "Blue",    32, 120, makecol(0,   0,   255));
   textout(screen, font, "Yellow",  32, 140, makecol(255, 255, 0  ));
   textout(screen, font, "Cyan",    32, 160, makecol(0,   255, 255));
   textout(screen, font, "Magenta", 32, 180, makecol(255, 0,   255));
   textout(screen, font, "Grey",    32, 200, makecol(128, 128, 128));

   /* or we could draw some nice smooth color gradients... */
   for (x=0; x<256; x++) {
      vline(screen, 192+x, 112, 176, makecol(x, 0, 0));
      vline(screen, 192+x, 208, 272, makecol(0, x, 0));
      vline(screen, 192+x, 304, 368, makecol(0, 0, x));
   }

   textout_centre(screen, font, "<press a key>", SCREEN_W/2, SCREEN_H-16, makecol(255, 255, 255));

   release_screen();

   readkey();
}



int main()
{
   allegro_init();
   install_keyboard(); 

   /* try each of the possible possible color depths... */
   test(8);
   test(15);
   test(16);
   test(24);
   test(32);

   return 0;
}

END_OF_MAIN();
