/*
 * Ryan Dickie
 * This example was derived from the acodec one
 * It reads image files and displays them one by one
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/icodec.h"

#define WIDTH 640
#define HEIGHT 480

int main(int argc, char **argv)
{
   int i;
   ALLEGRO_DISPLAY *display;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s {image_files}\n", argv[0]);
      return 1;
   }

   al_init();

   display = al_create_display(WIDTH, HEIGHT);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_show_mouse_cursor();
   for (i = 1; i < argc; ++i)
   {
      unsigned int width, height;
      const char* filename = argv[i]; 
      ALLEGRO_BITMAP* bmp = al_load_image(filename);
      if (!bmp) {
         fprintf(stderr, "Could not load ALLEGRO_BITMAP from '%s'!\n", filename);
         continue;
      }
      fprintf(stderr, "Drawing scaled bitmap %s\n",filename);
      width = al_get_bitmap_width(bmp);
      height = al_get_bitmap_height(bmp);
      al_draw_scaled_bitmap(bmp, 0,0, width, height, 0,0,WIDTH,HEIGHT,0);
      al_flip_display();
      al_rest(3.0);
      al_save_image(bmp, "output.jpg");
      al_destroy_bitmap(bmp);
   }

   return 0;
}
END_OF_MAIN()
