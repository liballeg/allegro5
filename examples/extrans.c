/*
 *    Example program for the Allegro library, by Owen Embury.
 *
 *    This program demonstrates how to use the lighting and translucency 
 *    functions.
 */


#include <stdio.h>

#include "allegro.h"



/* RGB -> color mapping table. Not needed, but speeds things up */
RGB_MAP rgb_table;

/* lighting color mapping table */
COLOR_MAP light_table;

/* translucency color mapping table */
COLOR_MAP trans_table;



int main(int argc, char *argv[])
{
   PALETTE pal;
   BITMAP *s;
   BITMAP *temp;
   BITMAP *spotlight;
   BITMAP *background;
   int i, x, y;
   char buf[256];
   char *filename;

   allegro_init(); 
   install_keyboard(); 
   install_timer(); 
   install_mouse(); 

   set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);

   /* load the main screen image */
   if (argc > 1)
      filename = argv[1];
   else {
      replace_filename(buf, argv[0], "allegro.pcx", sizeof(buf));
      filename = buf;
   }

   background = load_bitmap(filename, pal);
   if (!background) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s!\n", filename);
      return 1;
   }

   /* this isn't needed, but it speeds up the color table calculations */
   create_rgb_table(&rgb_table, pal, NULL);
   rgb_map = &rgb_table;

   /* build a color lookup table for lighting effects */
   create_light_table(&light_table, pal, 0, 0, 0, NULL);

   /* build a color lookup table for translucent drawing */
   create_trans_table(&trans_table, pal, 128, 128, 128, NULL);

   set_palette(pal);

   s = create_bitmap(320, 200);
   spotlight = create_bitmap(128, 128);
   temp = create_bitmap(128, 128);

   /* generate a spotlight image */
   clear(spotlight);
   for(i=0; i<256; i++)
      circlefill(spotlight, 64, 64, 64-i/4, i);

   /* select the lighting table */
   color_map = &light_table;

   /* display a spotlight effect */
   do {
      poll_mouse();

      x = mouse_x - 64;
      y = mouse_y - 64;

      clear(s);

      blit(background, s, x, y, x, y, 128, 128);
      draw_trans_sprite(s, spotlight, x, y);

      blit(s, screen, 0, 0, 0, 0, 320, 200);

   } while (!keypressed());

   clear_keybuf();

   /* generate an overlay image (just shrink the main image) */
   stretch_blit(background, spotlight, 0, 0, 320, 200, 0, 0, 128, 128);

   /* select the translucency table */
   color_map = &trans_table;

   /* display a translucent overlay */
   do {
      poll_mouse();

      x = mouse_x - 64;
      y = mouse_y - 64;

      blit(background, s, 0, 0, 0, 0, 320, 200);
      draw_trans_sprite(s, spotlight, x, y);

      blit(s, screen, 0, 0, 0, 0, 320, 200);

   } while (!keypressed());

   clear_keybuf();

   destroy_bitmap(s);
   destroy_bitmap(spotlight);
   destroy_bitmap(temp);
   destroy_bitmap(background);

   return 0;
}

END_OF_MAIN();
