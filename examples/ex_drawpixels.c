#include <allegro5/allegro.h>
#include <stdio.h>

#include "common.c"

#define WIDTH 640
#define HEIGHT 480
#define NUM_STARS 300
#define TARGET_FPS 9999


typedef struct Point
{
   float x, y;
} Point;


int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_KEYBOARD_STATE key_state;
   Point stars[3][NUM_STARS/3];
   float speeds[3] = { 0.0001f, 0.05f, 0.15f };
   A5O_COLOR colors[3];
   long start, now, elapsed, frame_count;
   int total_frames = 0;
   double program_start;
   double length;
   int layer, star;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_install_keyboard();
   
   display = al_create_display(WIDTH, HEIGHT);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   colors[0] = al_map_rgba(255, 100, 255, 128);
   colors[1] = al_map_rgba(255, 100, 100, 255);
   colors[2] = al_map_rgba(100, 100, 255, 255);
         
   for (layer = 0; layer < 3; layer++) {
      for (star = 0; star < NUM_STARS/3; star++) {
         Point *p = &stars[layer][star];
         p->x = rand() % WIDTH;
         p->y = rand() % HEIGHT;
      }
   }


   start = al_get_time() * 1000;
   now = start;
   elapsed = 0;
   frame_count = 0;
   program_start = al_get_time();


   while (1) {
      if (frame_count < (1000/TARGET_FPS)) {
         frame_count += elapsed;
      }
      else {
         int X, Y;

         frame_count -= (1000/TARGET_FPS);
         al_clear_to_color(al_map_rgb(0, 0, 0));
         for (star = 0; star < NUM_STARS/3; star++) {
            Point *p = &stars[0][star];
            al_draw_pixel(p->x, p->y, colors[0]);
         }
         al_lock_bitmap(al_get_backbuffer(display), A5O_PIXEL_FORMAT_ANY, 0);

         for (layer = 1; layer < 3; layer++) {
            for (star = 0; star < NUM_STARS/3; star++) {
               Point *p = &stars[layer][star];
               // put_pixel ignores blending
               al_put_pixel(p->x, p->y, colors[layer]);
            }
         }

         /* Check that dots appear at the window extremes. */
         X = WIDTH - 1;
         Y = HEIGHT - 1;
         al_put_pixel(0, 0, al_map_rgb_f(1, 1, 1));
         al_put_pixel(X, 0, al_map_rgb_f(1, 1, 1));
         al_put_pixel(0, Y, al_map_rgb_f(1, 1, 1));
         al_put_pixel(X, Y, al_map_rgb_f(1, 1, 1));

         al_unlock_bitmap(al_get_backbuffer(display));
         al_flip_display();
         total_frames++;
      }

      now = al_get_time() * 1000;
      elapsed = now - start;
      start = now;

      for (layer = 0; layer < 3; layer++) {
         for (star = 0; star < NUM_STARS/3; star++) {
            Point *p = &stars[layer][star];
            p->y -= speeds[layer] * elapsed;
            if (p->y < 0) {
               p->x = rand() % WIDTH;
               p->y = HEIGHT;
            }
         }
      }

      al_rest(0.001);

      al_get_keyboard_state(&key_state);
      if (al_key_down(&key_state, A5O_KEY_ESCAPE))
         break;
   }

   length = al_get_time() - program_start;

   if (length != 0) {
      log_printf("%d FPS\n", (int)(total_frames / length));
   }

   al_destroy_display(display);

   close_log(true);

   return 0;
}

