#include <allegro5/allegro5.h>
#include <stdio.h>


#define NUM_STARS 300
#define TARGET_FPS 9999


typedef struct Point
{
   float x, y;
} Point;


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_KBDSTATE key_state;
   Point stars[3][NUM_STARS/3];
   float speeds[3] = { 0.0001f, 0.05f, 0.15f };
   ALLEGRO_COLOR colors[3];
   long start, now, elapsed, frame_count;
   int total_frames = 0;
   double program_start;
   double length;
   int layer, star;
   ALLEGRO_LOCKED_REGION lr;


   al_init();
   al_install_keyboard();
   

   display = al_create_display(640, 480);

   colors[0] = al_map_rgba(200, 0, 255, 128);
   colors[1] = al_map_rgba(100, 0, 255, 255);
   colors[2] = al_map_rgba(0, 0, 200, 255);
         
   for (layer = 0; layer < 3; layer++) {
      for (star = 0; star < NUM_STARS/3; star++) {
         Point *p = &stars[layer][star];
         p->x = rand() % al_get_display_width();
         p->y = rand() % al_get_display_height();
      }
   }


   start = al_current_time() * 1000;
   now = start;
   elapsed = 0;
   frame_count = 0;
   program_start = al_current_time();


   while (1) {
      if (frame_count < (1000/TARGET_FPS)) {
         frame_count += elapsed;
      }
      else {
         frame_count -= (1000/TARGET_FPS);
         al_clear(al_map_rgb(0, 0, 0));
         for (star = 0; star < NUM_STARS/3; star++) {
            Point *p = &stars[0][star];
            al_draw_pixel(p->x, p->y, colors[0]);
         }
         al_lock_bitmap(al_get_backbuffer(), &lr, 0);
         for (layer = 1; layer < 3; layer++) {
            for (star = 0; star < NUM_STARS/3; star++) {
               Point *p = &stars[layer][star];
               // put_pixel ignores blending
               al_put_pixel(p->x, p->y, colors[layer]);
            }
         }
         al_unlock_bitmap(al_get_backbuffer());
         al_flip_display();
         total_frames++;
      }

      now = al_current_time() * 1000;
      elapsed = now - start;
      start = now;

      for (layer = 0; layer < 3; layer++) {
         for (star = 0; star < NUM_STARS/3; star++) {
            Point *p = &stars[layer][star];
            p->y -= speeds[layer] * elapsed;
            if (p->y < 0) {
               p->x = rand() % al_get_display_width();
               p->y = al_get_display_height();
            }
         }
      }

      al_rest(0.001);

      al_get_keyboard_state(&key_state);
      if (al_key_down(&key_state, ALLEGRO_KEY_ESCAPE))
         break;
   }

   length = al_current_time() - program_start;

   if (length != 0) {
      printf("%d FPS\n", (int)(total_frames / length));
   }

   al_destroy_display(display);

   return 0;
}
END_OF_MAIN()

