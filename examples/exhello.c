/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This is a very simple program showing how to get into graphics
 *    mode and draw text onto the screen.
 */


#include "allegro.h"



int main()
{
   /* you should always do this at the start of Allegro programs */
   allegro_init();

   /* set up the keyboard handler */
   install_keyboard(); 

   /* set a graphics mode sized 320x200 */
   set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);

   /* set the color palette */
   set_palette(desktop_palette);

   /* you don't need to do this, but on some platforms (eg. Windows) things
    * will be drawn more quickly if you always acquire the screen before
    * trying to draw onto it.
    */
   acquire_screen();

   /* write some text to the screen */
   textout_centre(screen, font, "Hello, world!", SCREEN_W/2, SCREEN_H/2, 255);

   /* you must always release bitmaps before calling any input functions */
   release_screen();

   /* wait for a keypress */
   readkey();

   return 0;
}

END_OF_MAIN();
