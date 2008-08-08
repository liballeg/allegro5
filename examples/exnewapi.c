/* This is a temporary example to get a feeling for the new API, which was very
 * much in flux yet when this example was created.
 *
 * Right now, it moves a red rectangle across the screen with a speed of
 * 100 pixels / second, limits the FPS to 100 Hz to save CPU, and prints the
 * current FPS in the top right corner.
 */

#include <math.h>
#include <stdio.h>
#include "allegro5/allegro5.h"

int main(void)
{
   ALLEGRO_DISPLAY *display[3];
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_QUEUE *events;
   int quit = 0;
   int fps_accumulator = 0, fps_time = 0;
   double fps = 0;
   int FPS = 500;
   double start;
   double last_move;
   int frames = 0;
   int x = 0, y = 100;
   int dx = 1;
   int w = 640, h = 480;
   ALLEGRO_BITMAP *picture;
   ALLEGRO_BITMAP *mask;
   ALLEGRO_BITMAP *mem_bmp;
   ALLEGRO_COLOR colors[3];
   ALLEGRO_LOCKED_REGION lr;
   int i;
   bool clip = false;

   al_init();

   events = al_create_event_queue();

   al_set_new_display_flags(ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE);
   //al_set_new_display_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

   /* Create three windows. */
   display[0] = al_create_display(w, h);
   display[1] = al_create_display(w, h);
   display[2] = al_create_display(w, h);


   //al_set_clipping_rectangle(100, 100, 440, 280);

   /* This is only needed since we want to receive resize events. */
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display[0]);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display[1]);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display[2]);

   /* Apparently, need to think a bit more about memory/display bitmaps.. should
    * only need to load it once (as memory bitmap), then make available on all
    * displays.
    */
   al_set_current_display(display[2]);

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

   picture = al_load_bitmap("mysha.tga");
   if (!picture) {
      allegro_message("failed to load mysha.tga");
      return 1;
   }

   mask = al_load_bitmap("mask.pcx");
   if (!mask) {
      allegro_message("failed to load mask.pcx");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   mem_bmp = al_load_bitmap("mysha.tga");
   if (!mem_bmp) {
      allegro_message("failed to load mysha.tga (mem)");
      return 1;
   }

   al_install_keyboard();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());


   for (i = 0; i < 3; i++) {
      al_set_current_display(display[i]);
      if (i == 0)
         colors[0] = al_map_rgba_f(1, 0, 0, 0.5f);
      else if (i == 1)
         colors[1] = al_map_rgba_f(0, 1, 0, 0.5f);
      else
         colors[2] = al_map_rgba_f(0, 0, 1, 0.5f);
   }

   al_set_target_bitmap(picture);
   al_draw_line(0, 0, 320, 200, al_map_rgba_f(1, 1, 0, 1));
   al_draw_line(0, 0, 100, 0, al_map_rgba_f(1, 1, 0, 0.5));
   al_draw_line(0, 0, 0, 100, al_map_rgba_f(1, 1, 0, 0.5));
	
   start = al_current_time();
   last_move = al_current_time();

   while (!quit) {
      /* read input */
      while (!al_event_queue_is_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            ALLEGRO_KEYBOARD_EVENT *key = &event.keyboard;
            if (key->keycode == ALLEGRO_KEY_ESCAPE) {
               quit = 1;
            }
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            ALLEGRO_DISPLAY_EVENT *display = &event.display;
            w = display->width;
            h = display->height;
            al_acknowledge_resize(display->source);
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            int i;
            for (i = 0; i < 3; i++) {
               if (display[i] == event.display.source)
                  display[i] = 0;
            }
            al_destroy_display(event.display.source);
            for (i = 0; i < 3; i++) {
               if (display[i])
                  goto not_done;
            }
            quit = 1;
         not_done:
            ;
         }
      }

      while (last_move < al_current_time()) {
         last_move += 0.001;
         x += dx;
         if (x == 0)
            dx = 1;
         if (x == w - 140)
            dx = -1;
      }

      frames++;
      for (i = 0; i < 3; i++) {
         if (!display[i])
            continue;

         al_set_current_display(display[i]);
         al_clear(al_map_rgb_f(1, 1, 1));
         if (i == 1) {
            al_draw_line(50, 50, 150, 150, colors[0]);
         }

         if (i == 2) {
            al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO,
               al_map_rgba_f(1, 1, 1, 1));
            al_draw_scaled_bitmap(picture, 0, 0,
                  al_get_bitmap_width(picture), al_get_bitmap_height(picture),
                  0, 0, 640, 480, 0);
            al_set_blender(ALLEGRO_ALPHA, ALLEGRO_ONE,
               al_map_rgba_f(1, 1, 0, 1));
            al_draw_rotated_bitmap(picture, 160, 100, 320, 240, AL_PI/4, 0);
            al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
               al_map_rgba_f(1, 1, 1, 1));
         }

         al_draw_rectangle(x, y, x + 140, y + 140, colors[i], ALLEGRO_FILLED);
         al_flip_display();
      }
   }

   printf("fps=%f\n", (double)frames / (al_current_time()-start));

   al_destroy_bitmap(picture);
   al_destroy_bitmap(mask);
   al_destroy_bitmap(mem_bmp);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
