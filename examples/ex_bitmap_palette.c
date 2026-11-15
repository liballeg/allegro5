#define ALLEGRO_UNSTABLE
#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"

#include "common.c"

int main(int argc, char **argv)
{
   int i, j;
   float r, g, b, a;
   int count;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_COLOR *palette = NULL;
   const char *filename;

   if (!al_init())
      abort_example("Could not init Allegro.\n");
   if (!al_init_image_addon())
      abort_example("Could not init the image addon.\n");

   for (i = 1; i < argc; i++) {
      filename = argv[i];
      bitmap = al_load_bitmap_flags(filename, ALLEGRO_KEEP_PALETTE);
      if (bitmap) {
         count = al_get_bitmap_palette(bitmap, NULL);
         if (count) {
            palette = al_realloc(palette, count * sizeof(ALLEGRO_COLOR));
            printf("%s: Palette with %d entries\n", filename, count);
            al_get_bitmap_palette(bitmap, palette);
            for (j = 0; j < count; j++) {
               al_unmap_rgba_f(palette[j], &r, &g, &b, &a);
               printf("%3d: %.3f %.3f %.3f %.3f\n", j, r, g, b, a);
            }
         } else {
            printf("%s: No palette\n", filename);
         }
         al_destroy_bitmap(bitmap);
      } else {
         printf("%s: Could not load\n", filename);
      }
   }
   if (palette)
      al_free(palette);
   return 0;
}
