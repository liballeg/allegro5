/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    In this example, each thread handles its own window and event queue.
 */

#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

#define MAX_THREADS     100
#define MAX_BACKGROUNDS 10
#define MAX_SQUARES     25

typedef struct Background {
   double rmax;
   double gmax;
   double bmax;
} Background;


typedef struct Square {
   float cx, cy;
   float dx, dy;
   float size, dsize;
   float rot, drot;
   float life, dlife;
} Square;


static float rand01(void)
{
   return (rand() % 10000) / 10000.0;
}


static float rand11(void)
{
   return (-10000 + (rand() % 20000)) / 20000.0;
}


static void gen_square(Square *sq, int w, int h)
{
   sq->cx = rand() % w;
   sq->cy = rand() % h;
   sq->dx = 3.0 * rand11();
   sq->dy = 3.0 * rand11();
   sq->size = 10 + (rand() % 10);
   sq->dsize = rand11();
   sq->rot = ALLEGRO_PI * rand01();
   sq->drot = rand11() / 3.0;
   sq->life = 0.0;
   sq->dlife = (ALLEGRO_PI / 100.0) + (ALLEGRO_PI / 30.0) * rand01();
}


static void animate_square(Square *sq)
{
   sq->cx += sq->dx;
   sq->cy += sq->dy;
   sq->size += sq->dsize;
   sq->rot += sq->drot;
   sq->life += sq->dlife;

   if (sq->size < 1.0 || sq->life > ALLEGRO_PI) {
      ALLEGRO_BITMAP *bmp = al_get_target_bitmap();
      gen_square(sq, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp));
   }
}


static void draw_square(Square *sq)
{
   ALLEGRO_TRANSFORM trans;
   float alpha;
   float size;
   ALLEGRO_COLOR tint;

   al_build_transform(&trans, sq->cx, sq->cy, 1.0, 1.0, sq->rot);
   al_use_transform(&trans);

   alpha = sin(sq->life);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ONE);
   tint = al_map_rgba_f(0.5, 0.3, 0, alpha);

   size = sq->size;
   al_draw_filled_rounded_rectangle(-size, -size, size, size, 3, 3, tint);

   size *= 1.1;
   al_draw_rounded_rectangle(-size, -size, size, size, 3, 3, tint, 2);
}


static void *thread_func(ALLEGRO_THREAD *thr, void *arg)
{
   const int INITIAL_WIDTH = 300;
   const int INITIAL_HEIGHT = 300;
   const Background *background = (Background *) arg;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue = NULL;
   ALLEGRO_TIMER *timer = NULL;
   ALLEGRO_EVENT event;
   ALLEGRO_STATE state;
   Square squares[MAX_SQUARES];
   double theta = 0.0;
   bool redraw = true;
   int i;

   (void)thr;

   al_set_new_display_flags(ALLEGRO_RESIZABLE);

   display = al_create_display(INITIAL_WIDTH, INITIAL_HEIGHT);
   if (!display) {
      goto Quit;
   }
   queue = al_create_event_queue();
   if (!queue) {
      goto Quit;
   }
   timer = al_create_timer(0.1);
   if (!timer) {
      goto Quit;
   }

   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));

   for (i = 0; i < MAX_SQUARES; i++) {
      gen_square(&squares[i], INITIAL_WIDTH, INITIAL_HEIGHT);
   }

   al_start_timer(timer);

   while (true) {
      if (al_is_event_queue_empty(queue) && redraw) {
         double r = 0.7 + 0.3 * (sin(theta) + 1.0) / 2.0;
         ALLEGRO_COLOR c = al_map_rgb_f(
            background->rmax * r,
            background->gmax * r,
            background->bmax * r
         );
         al_clear_to_color(c);

         al_store_state(&state, ALLEGRO_STATE_BLENDER | ALLEGRO_STATE_TRANSFORM);
         for (i = 0; i < MAX_SQUARES; i++) {
            draw_square(&squares[i]);
         }
         al_restore_state(&state);

         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_TIMER) {
         for (i = 0; i < MAX_SQUARES; i++) {
            animate_square(&squares[i]);
         }
         theta += 0.1;
         redraw = true;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN
            && event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
      }
   }

Quit:

   if (timer) {
      al_destroy_timer(timer);
   }
   if (queue) {
      al_destroy_event_queue(queue);
   }
   if (display) {
      al_destroy_display(display);
   }

   return NULL;
}


int main(int argc, char **argv)
{
   ALLEGRO_THREAD *thread[MAX_THREADS];
   Background background[MAX_BACKGROUNDS] = {
      { 1.0, 0.5, 0.5 },
      { 0.5, 1.0, 0.5 },
      { 0.5, 0.5, 1.0 },
      { 1.0, 1.0, 0.5 },
      { 0.5, 1.0, 1.0 },
      { 1.0, 0.7, 0.5 },
      { 0.5, 1.0, 0.7 },
      { 0.7, 0.5, 1.0 },
      { 1.0, 0.7, 0.5 },
      { 0.5, 0.7, 1.0 }
   };
   int num_threads;
   int i;

   if (argc > 1) {
      num_threads = strtol(argv[1], NULL, 10);
      if (num_threads > MAX_THREADS)
         num_threads = MAX_THREADS;
      else if (num_threads < 1)
         num_threads = 1;
   }
   else {
      num_threads = 3;
   }

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();

   for (i = 0; i < num_threads; i++) {
      thread[i] = al_create_thread(thread_func,
         &background[i % MAX_BACKGROUNDS]);
   }
   for (i = 0; i < num_threads; i++) {
      al_start_thread(thread[i]);
   }
   for (i = 0; i < num_threads; i++) {
      al_join_thread(thread[i], NULL);
      al_destroy_thread(thread[i]);
   }

   return 0;
}


/* vim: set sts=3 sw=3 et: */
