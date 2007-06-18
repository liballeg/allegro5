/*
 *    Example program for the Allegro library, by Patrick Hogan
 *
 *    This program demonstrates how to draw Gouraud shaded (lit)
 *    sprites.  In an apparently black screen, a planet like sprite
 *    is drawn close to the middle of the screen. In a similar
 *    way to how the first test of extrans works, you move the
 *    cursor on the screen with the mouse. Attached to this mouse
 *    you can imagine a virtual spotlight illuminating the scene
 *    around. Depending on where the mouse is, the Gouraud shaded
 *    sprite will show the direction of the light.
 */


#include <math.h>

#include <allegro.h>



/* RGB -> color mapping table. Not needed, but speeds things up */
RGB_MAP rgb_table;

COLOR_MAP light_table;



/* Considered a line between (x1, y1) and (x2, y2), the longer the line,
 * the smaller the return value will be. If the line length is zero, the
 * function returns the maximum value of 255.
 */
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

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 240, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 240, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n",
			 allegro_error);
	 return 1;
      }
   }

   replace_filename(buf, argv[0], "planet.pcx", sizeof(buf));

   planet = load_bitmap(buf, pal);
   if (!planet) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s\n", buf);
      return 1;
   }

   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   clear_bitmap(buffer);

   set_palette(pal);

   create_rgb_table(&rgb_table, pal, NULL);
   rgb_map = &rgb_table;

   create_light_table(&light_table, pal, 0, 0, 0, NULL);
   color_map = &light_table;

   set_trans_blender(0, 0, 0, 128);

   do {
      poll_mouse();

      draw_gouraud_sprite(buffer, planet, SCREEN_W/2, SCREEN_H/2,
			  distance(SCREEN_W/2, SCREEN_H/2, mouse_x, mouse_y),
			  distance(SCREEN_W/2 + planet->w, SCREEN_H/2,
				   mouse_x, mouse_y),
			  distance(SCREEN_W/2 + planet->w,
				   SCREEN_H/2 + planet->h, mouse_x, mouse_y),
			  distance(SCREEN_W/2, SCREEN_H/2 + planet->h,
				   mouse_x, mouse_y));

      textout_ex(buffer, font, "Gouraud Shaded Sprite Demo", 0, 0,
		 palette_color[10], -1);

      show_mouse(buffer);
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      show_mouse(NULL);

   } while (!keypressed());

   destroy_bitmap(planet);
   destroy_bitmap(buffer);

   return 0;
}

END_OF_MAIN()
