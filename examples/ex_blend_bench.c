/*
 *    Benchmark for memory blenders.
 */

#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <time.h>

#include "common.c"

/* Do a few un-timed runs to switch CPU to performance mode and cache
 * data and so on - seems to make the results more stable here.
 * Also used to guess the number of timed iterations.
 */
#define WARMUP 100
/* How many seconds the timing should approximately take - a fixed
 * number of iterations is not enough on very fast systems but takes
 * too long on slow systems.
 */
#define TEST_TIME 5.0

enum Mode {
   PLAIN_BLIT,
   SCALED_BLIT,
   ROTATE_BLIT
};

static void step(enum Mode mode, ALLEGRO_BITMAP *b2)
{
   switch (mode) {
      case PLAIN_BLIT:
         al_draw_bitmap(b2, 0, 0, 0);
         break;
      case SCALED_BLIT:
         al_draw_scaled_bitmap(b2, 0, 0, 320, 200, 0, 0, 640, 480, 0);
         break;
      case ROTATE_BLIT:
         al_draw_scaled_rotated_bitmap(b2, 10, 10, 10, 10, 2.0, 2.0,
            ALLEGRO_PI/30, 0);
         break;
   }
}

/* al_get_current_time() measures wallclock time - but for the benchmark
 * result we prefer CPU time so clock() is better.
 */
static double current_clock(void)
{
   clock_t c = clock();
   return (double)c / CLOCKS_PER_SEC;
}

int main(int argc, const char *argv[])
{
   enum Mode mode = PLAIN_BLIT;
   ALLEGRO_STATE state;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *b1;
   ALLEGRO_BITMAP *b2;
   int REPEAT;
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

   open_log();

   al_init_image_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   b1 = al_load_bitmap("data/mysha.pcx");
   if (!b1) {
      abort_example("Error loading data/mysha.pcx\n");
      return 1;
   }

   b2 = al_load_bitmap("data/allegro.pcx");
   if (!b2) {
      abort_example("Error loading data/mysha.pcx\n");
      return 1;
   }

   al_set_target_bitmap(b1);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
   step(mode, b2);

   /* Display the blended bitmap to the screen so we can see something. */
   al_store_state(&state, ALLEGRO_STATE_ALL);
   al_set_target_backbuffer(display);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_draw_bitmap(b1, 0, 0, 0);
   al_flip_display();
   al_restore_state(&state);

   log_printf("Please wait...\n");

   /* Do warmup run and estimate required runs for real test. */
   t0 = current_clock();
   for (i = 0; i < WARMUP; i++) {
      step(mode, b2);
   }
   t1 = current_clock();
   REPEAT = TEST_TIME * 100 / (t1 - t0);

   /* Do the real test. */
   t0 = current_clock();
   for (i = 0; i < REPEAT; i++) {
      step(mode, b2);
   }
   t1 = current_clock();

   log_printf("Time = %g s, %d steps, FPS = %g\n",
      t1 - t0, REPEAT, REPEAT / (t1 - t0));

   al_destroy_display(display);

   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
