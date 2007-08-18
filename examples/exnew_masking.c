#include <allegro.h>

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_COLOR color;

   /* Initialize the library */
   al_init();

   /* Create a window */
   display = al_create_display(640, 480);

   /* Load a bitmap */
   al_set_new_bitmap_flags(ALLEGRO_USE_MASKING);
   bitmap = al_load_bitmap("rgb.pcx");

   /* Clear the screen to white */
   al_clear(al_map_rgb(al_get_backbuffer(), &color, 255, 255, 255));

   /* Draw the bitmap (masking is enabled, default mask color is magenta) */
   al_draw_bitmap(bitmap, 0, 0, 0);

   /* Change the mask color to red and draw again */
   al_set_bitmap_mask_color(bitmap, al_map_rgb(bitmap, &color, 255, 0, 0));
   al_draw_bitmap(bitmap, 320, 0, 0);

   /* Change the mask color to green and draw again */
   al_set_bitmap_mask_color(bitmap, al_map_rgb(bitmap, &color, 0, 255, 0));
   al_draw_bitmap(bitmap, 0, 240, 0);

   /* Change the mask color to blue and draw again */
   al_set_bitmap_mask_color(bitmap, al_map_rgb(bitmap, &color, 0, 0, 255));
   al_draw_bitmap(bitmap, 320, 240, 0);

   /* Flip the backbuffer to the display */
   al_flip_display();

   al_rest(4000);
}
END_OF_MAIN()

