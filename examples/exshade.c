/*
 *    Example program for the Allegro library, by Patrick Hogan
 *
 *    This program demonstrates how to draw gouraud shaded (lit) sprites.
 */


#include <math.h>

#include "allegro.h"



/* RGB -> color mapping table. Not needed, but speeds things up */
RGB_MAP rgb_table;

COLOR_MAP light_table;



int distance(int x1, int y1, int x2, int y2)
{
   int dx = x2 - x1;
   int dy = y2 - y1;
   int temp = sqrt((dx * dx) + (dy * dy));

   temp *= 2;
   if (temp > 255)
      temp = 255;

   return (255 - temp);
}



int main(int argc, char *argv[])
{
   PALETTE pal;
   BITMAP *buffer;
   BITMAP *planet;
   char buf[256];

   allegro_init();
   install_keyboard();
   install_mouse();
   set_gfx_mode(GFX_SAFE, 320, 240, 0, 0);

   replace_filename(buf, argv[0], "planet.pcx", sizeof(buf));

   planet = load_bitmap(buf, pal);
   if (!planet) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s\n", buf);
      return 1;
   }

   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   clear(buffer);

   set_palette(pal);

   create_rgb_table(&rgb_table, pal, NULL);
   rgb_map = &rgb_table;

   create_light_table(&light_table, pal, 0, 0, 0, NULL);
   color_map = &light_table;

   do {
      poll_mouse();

      draw_gouraud_sprite(buffer, planet, 160, 100,
			  distance(160, 100, mouse_x, mouse_y),
			  distance(160 + planet->w, 100, mouse_x, mouse_y),
			  distance(160 + planet->w, 100 + planet->h, mouse_x, mouse_y),
			  distance(160, 100 + planet->h, mouse_x, mouse_y));

      textout(buffer, font, "Gouraud Shaded Sprite Demo", 0, 0, 10);

      show_mouse(buffer);
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      show_mouse(NULL);

   } while (!keypressed());

   destroy_bitmap(planet);
   destroy_bitmap(buffer);

   return 0;
}

END_OF_MAIN();
