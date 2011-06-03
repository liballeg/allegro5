#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

int load_count, load_total = 100;
ALLEGRO_BITMAP *bitmaps[100];

static void *loading_thread(ALLEGRO_THREAD *thread, void *arg)
{
   ALLEGRO_FONT *font = al_load_font("data/fixed_font.tga", 0, 0);
   ALLEGRO_COLOR text = al_map_rgb_f(255, 255, 255);

   /* In this example we load mysha.pcx 100 times to simulate loading
    * many bitmaps.
    */
   for (load_count = 0; load_count < load_total; load_count++) {
      ALLEGRO_COLOR color = al_map_rgb(rand() % 256, rand() % 256,
         rand() % 256);
      color.r /= 4;
      color.g /= 4;
      color.b /= 4;
      color.a /= 4;
      
      if (al_get_thread_should_stop(thread)) break;

      bitmaps[load_count] = al_load_bitmap("data/mysha.pcx");

      /* Simulate different contents. */
      al_set_target_bitmap(bitmaps[load_count]);
      al_draw_filled_rectangle(0, 0, 320, 200, color);
      al_draw_textf(font, text, 0, 0, 0, "bitmap %d", 1 + load_count);
      al_set_target_bitmap(NULL);
      /* Simulate that it's slow. */
      al_rest(0.05);
   }
   
   al_destroy_font(font); 
   return arg;
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   bool redraw = true;
   ALLEGRO_FONT *font;
   ALLEGRO_BITMAP *spin;
   int current_bitmap = 0, loaded_bitmap = 0;
   ALLEGRO_THREAD *thread;

   al_init();
   al_init_image_addon();
   al_init_font_addon();
   al_init_primitives_addon();

   al_install_mouse();
   al_install_keyboard();
   
   /* We call this before creating the display on purpose. The
    * font glyphs are all created as video bitmaps by default, but
    * because there is no display yet they are forced to be memory
    * bitmaps.
    */
   font = al_load_font("data/fixed_font.tga", 0, 0);
   spin = al_load_bitmap("data/cursor.tga");

   display = al_create_display(640, 480);
   
   /* Calling this converts all the memory bitmaps who are marked as
    * video bitmaps to real video bitmaps, belonging to the current
    * display.
    */
   al_convert_all_video_bitmaps();

   thread = al_create_thread(loading_thread, NULL);
   al_start_thread(thread);

   timer = al_create_timer(1.0 / 30);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
             break;
         }
      if (event.type == ALLEGRO_EVENT_TIMER)
         redraw = true;

      if (redraw && al_is_event_queue_empty(queue)) {
         float x = 20, y = 320;
         int i;
         ALLEGRO_COLOR color = al_map_rgb_f(0, 0, 0);
         float t = al_current_time();

         redraw = false;
         al_clear_to_color(al_map_rgb_f(0.5, 0.6, 1));
         
         al_draw_textf(font, color, x + 40, y, 0, "Loading %d%%",
            100 * load_count / load_total);

         if (loaded_bitmap < load_count) {
            /* This will convert any video bitmaps without a display
             * (all the bitmaps being loaded in the loading_thread) to
             * video bitmaps we can use in the main thread.
             */
            al_convert_bitmap(bitmaps[loaded_bitmap]);
            loaded_bitmap++;
         }
         if (current_bitmap < loaded_bitmap) {
            int bw, bh;
            al_draw_bitmap(bitmaps[current_bitmap], 0, 0, 0);
            if (current_bitmap + 1 < loaded_bitmap)
               current_bitmap++;

            for (i = 0; i <= current_bitmap; i++) {
               bw = al_get_bitmap_width(bitmaps[i]);
               bh = al_get_bitmap_width(bitmaps[i]);
               al_draw_scaled_rotated_bitmap(bitmaps[i],
                  0, 0, (i % 20) * 640 / 20, 360 + (i / 20) * 24,
                  32.0 / bw, 32.0 / bw, 0, 0);
            }
         }
         
         if (loaded_bitmap < load_total) {
            al_draw_scaled_rotated_bitmap(spin,
               16, 16, x, y, 1.0, 1.0, t * ALLEGRO_PI * 2, 0);
         }
         
         al_flip_display();
      }
   }

   al_join_thread(thread, NULL);
   al_destroy_font(font); 

   return 0;
}
