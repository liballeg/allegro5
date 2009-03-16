/*
 *    Benchmark for memory blenders.
 */

#include <stdio.h>
#include <allegro5/allegro5.h>
#include <allegro5/a5_iio.h>

#define REPEAT 300

enum Mode {
   PLAIN_BLIT,
   SCALED_BLIT,
   ROTATE_BLIT
};

void step(enum Mode mode, ALLEGRO_BITMAP *b2)
{
   switch (mode) {
      case PLAIN_BLIT:
         al_draw_bitmap(b2, 0, 0, 0);
         break;
      case SCALED_BLIT:
         al_draw_scaled_bitmap(b2, 0, 0, 320, 200, 0, 0, 640, 480, 0);
         break;
      case ROTATE_BLIT:
         al_draw_rotated_scaled_bitmap(b2, 10, 10, 10, 10, 2.0, 2.0,
            ALLEGRO_PI/30, 0);
         break;
   }
}

int main(int argc, const char *argv[])
{
   enum Mode mode = PLAIN_BLIT;
   ALLEGRO_STATE state;
   ALLEGRO_BITMAP *b1;
   ALLEGRO_BITMAP *b2;
   double t0, t1;
   int i;

   if (argc > 1) {
      i = strtol(argv[1], NULL, 10);
      switch (i) {
         case 1:
            mode = SCALED_BLIT;
            break;
         case 2:
            mode = ROTATE_BLIT;
            break;
         default:
            mode = PLAIN_BLIT;
            break;
      }
   }

   if (!al_init())
      return 1;

   al_iio_init();

   if (!al_create_display(640, 480)) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   b1 = al_iio_load("data/mysha.pcx");
   if (!b1) {
      TRACE("Error loading data/mysha.pcx\n");
      return 1;
   }

   b2 = al_iio_load("data/allegro.pcx");
   if (!b2) {
      TRACE("Error loading data/mysha.pcx\n");
      return 1;
   }

   al_set_target_bitmap(b1);
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgba_f(1, 1, 1, 0.5));
   step(mode, b2);

   /* Display the blended bitmap to the screen so we can see something. */
   al_store_state(&state, ALLEGRO_STATE_ALL);
   al_set_target_bitmap(al_get_backbuffer());
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
   al_draw_bitmap(b1, 0, 0, 0);
   al_flip_display();
   al_restore_state(&state);

   printf("Please wait...\n");

   t0 = al_current_time();
   for (i = 0; i < REPEAT; i++) {
      step(mode, b2);
   }
   t1 = al_current_time();

   printf("Time = %g s, FPS = %g\n", t1 - t0, REPEAT / (t1 - t0));

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
