/* Image conversion example */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"

#include "common.c"


int main(int argc, char **argv)
{
   ALLEGRO_BITMAP *bitmap;
   double t0;
   double t1;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   if (argc < 3) {
      log_printf("This example needs to be run from the command line.\n");
      log_printf("Usage: %s <infile> <outfile>\n", argv[0]);
      log_printf("\tPossible file types: BMP PCX PNG TGA\n");
      goto done;
   }

   al_init_image_addon();

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP
      | ALLEGRO_NO_PREMULTIPLIED_ALPHA);

   bitmap = al_load_bitmap(argv[1]);
   if (!bitmap) {
      log_printf("Error loading input file\n");
      goto done;
   }

   t0 = al_get_time();
   if (!al_save_bitmap(argv[2], bitmap)) {
      log_printf("Error saving bitmap\n");
      goto done;
   }
   t1 = al_get_time();
   log_printf("Saving took %.4f seconds\n", t1 - t0);

   al_destroy_bitmap(bitmap);

done:
   close_log(true);

   return 0;
}

