#include <allegro5/allegro5.h>
#include <stdio.h>


const int NUM_STARS = 300;
const int TARGET_FPS = 60;


struct Point
{
   float x, y;
};


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_KBDSTATE key_state;
   Point stars[3][NUM_STARS/3];
   float speeds[3] = { 0.0001f, 0.05f, 0.15f };
   ALLEGRO_COLOR colors[3] = {
      al_map_rgba(200, 0, 255, 128),
      al_map_rgba(100, 0, 255, 255),
      al_map_rgba(0, 0, 200, 255)
   };
   long start, now, elapsed, frame_count;
   int total_frames = 0;
   double program_start;


   al_init();
   al_install_keyboard();
   

   display = al_create_display(640, 480);

         
   for (int layer = 0; layer < 3; layer++) {
      for (int star = 0; star < NUM_STARS/3; star++) {
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
         for (int layer = 0; layer < 3; layer++) {
            for (int star = 0; star < NUM_STARS/3; star++) {
               Point *p = &stars[layer][star];
               if (layer == 0) {
                  // draw_pixel obeys blending
                  al_draw_pixel(p->x, p->y, colors[layer]);
               }
               else {
                  // put_pixel ignores blending
                  al_put_pixel(p->x, p->y, colors[layer]);
               }
            }
         }
         al_flip_display();
         total_frames++;
      }


      now = al_current_time() * 1000;
      elapsed = now - start;
      start = now;

      for (int layer = 0; layer < 3; layer++) {
         for (int star = 0; star < NUM_STARS/3; star++) {
            Point *p = &stars[layer][star];
            p->y -= speeds[layer] * elapsed;
            if (p->y < 0) {
               p->x = rand() % al_get_display_width();
               p->y = al_get_display_height();
            }
         }
      }

      al_get_keyboard_state(&key_state);
      if (al_key_down(&key_state, ALLEGRO_KEY_ESCAPE))
         break;
   }

   double length = al_current_time() - program_start;

   if (length != 0) {
      printf("%d FPS\n", (int)(total_frames / length));
   }

   al_destroy_display(display);
}
END_OF_MAIN()

