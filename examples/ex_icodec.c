/*
 * Example program by Ryan Dickie.
 *
 * This example reads image files using the icodec addon (ImageMagick) and
 * displays them one by one.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/icodec.h"

#define WIDTH	640
#define HEIGHT	480

int main(int argc, char **argv)
{
   int i;
   ALLEGRO_DISPLAY *display;

   if (argc < 2) {
      fprintf(stderr, "Usage: %s {image_files}\n", argv[0]);
      return 1;
   }

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   display = al_create_display(WIDTH, HEIGHT);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   for (i = 1; i < argc; i++) {
      const char *filename = argv[i];
      ALLEGRO_BITMAP *bmp;
      int width, height;
      
      bmp = al_load_image(filename);
      if (!bmp) {
	 fprintf(stderr, "Could not load '%s'!\n", filename);
	 continue;
      }

      width = al_get_bitmap_width(bmp);
      height = al_get_bitmap_height(bmp);
      al_draw_scaled_bitmap(bmp, 0, 0, width, height, 0, 0, WIDTH, HEIGHT, 0);
      al_flip_display();
      al_rest(3.0);

      /* XXX icodec saving is currently broken */
      /* al_save_image(bmp, "output.jpg"); */

      al_destroy_bitmap(bmp);
   }

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
