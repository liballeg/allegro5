/*
 *    Benchmark for memory blenders.
 */

#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
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
   ALL,
   PLAIN_BLIT,
   SCALED_BLIT,
   ROTATE_BLIT,
   DRAW_PRIM,
};

static char const *names[] = {
   "", "Plain blit", "Scaled blit", "Rotated blit", "Draw primitive"
};

ALLEGRO_DISPLAY *display;

ALLEGRO_VERTEX_ELEMENT elems[] = {
   {ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_FLOAT_3, offsetof(ALLEGRO_VERTEX, x)},
   {ALLEGRO_PRIM_TEX_COORD_PIXEL, ALLEGRO_PRIM_FLOAT_2, offsetof(ALLEGRO_VERTEX, u)},
   {ALLEGRO_PRIM_COLOR_ATTR, 0, offsetof(ALLEGRO_VERTEX, color)},
   {0, 0, 0}
};
ALLEGRO_VERTEX_DECL* decl;

static void replacement_al_draw_scaled_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   ALLEGRO_VERTEX v[4];
   ALLEGRO_COLOR c = al_map_rgb_f(1, 1, 1);
   int w = al_get_bitmap_width(bitmap);
   int h = al_get_bitmap_height(bitmap);
   ALLEGRO_TRANSFORM trans;
   al_identity_transform(&trans);
   al_translate_transform(&trans, -cx, -cy);
   al_scale_transform(&trans, xscale, yscale);
   al_rotate_transform(&trans, angle);
   al_translate_transform(&trans, dx, dy);
   v[0].x = 0;
   v[0].y = 0;
   v[0].z = 0;
   v[0].u = 0;
   v[0].v = 0;
   v[0].color = c;
   v[1].x = w;
   v[1].y = 0;
   v[1].z = 0;
   v[1].u = w;
   v[1].v = 0;
   v[1].color = c;
   v[3].x = w;
   v[3].y = h;
   v[3].z = 0;
   v[3].u = w;
   v[3].v = h;
   v[3].color = c;
   v[2].x = 0;
   v[2].y = h;
   v[2].z = 0;
   v[2].u = 0;
   v[2].v = h;
   v[2].color = c;
   
   al_use_transform(&trans);
   al_draw_prim(v, decl, bitmap, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
}

static void setup(enum Mode mode)
{
   if (mode == DRAW_PRIM) {
      decl = al_create_vertex_decl(elems, sizeof(ALLEGRO_VERTEX));
   }
}

static void tear_down(enum Mode mode)
{
   if (mode == DRAW_PRIM) {
      al_destroy_vertex_decl(decl);
   }
}

static void step(enum Mode mode, ALLEGRO_BITMAP *b2)
{
   switch (mode) {
      case ALL: break;
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
      case DRAW_PRIM:
         replacement_al_draw_scaled_rotated_bitmap(b2, 10, 10, 10, 10,
            2.0, 2.0, ALLEGRO_PI/30, 0);
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

static bool do_test(enum Mode mode)
{
   ALLEGRO_STATE state;
   ALLEGRO_BITMAP *b1;
   ALLEGRO_BITMAP *b2;
   int REPEAT;
   double t0, t1;
   int i;

   setup(mode);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   b1 = al_load_bitmap("data/mysha.pcx");
   if (!b1) {
      abort_example("Error loading data/mysha.pcx\n");
      return false;
   }

   b2 = al_load_bitmap("data/allegro.pcx");
   if (!b2) {
      abort_example("Error loading data/mysha.pcx\n");
      return false;
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

   log_printf("Benchmark: %s\n", names[mode]);
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

   log_printf("Time = %g s, %d steps\n",
      t1 - t0, REPEAT);
   log_printf("%s: %g FPS\n", names[mode], REPEAT / (t1 - t0));
   
   al_destroy_bitmap(b1);
   al_destroy_bitmap(b2);
      
   tear_down(mode);
   
   return true;
}

int main(int argc, const char *argv[])
{
   enum Mode mode = ALL;
   int i;

   if (argc > 1) {
      i = strtol(argv[1], NULL, 10);
      switch (i) {
         case 0:
            mode = PLAIN_BLIT;
            break;
         case 1:
            mode = SCALED_BLIT;
            break;
         case 2:
            mode = ROTATE_BLIT;
            break;
         case 3:
            mode = DRAW_PRIM;
            break;
      }
   }

   if (!al_init())
      return 1;

   open_log();

   al_init_image_addon();
   al_init_primitives_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }
   
   if (mode == ALL) {
      for (mode = PLAIN_BLIT; mode <= DRAW_PRIM; mode++) {
         do_test(mode);
      }
   }
   else {
      do_test(mode);
   }

   al_destroy_display(display);

   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
