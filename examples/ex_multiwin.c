#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"

#include "common.c"

const int W = 640;
const int H = 400;

int main(void)
{
   ALLEGRO_DISPLAY *display[2];
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_BITMAP *pictures[2];
   ALLEGRO_BITMAP *target;
   int width, height;
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_install_keyboard();
   al_install_mouse();
   al_init_image_addon();

   events = al_create_event_queue();

   al_set_new_display_flags(ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE);

   /* Create two windows. */
   display[0] = al_create_display(W, H);
   if (!display[0]) {
      abort_example("Error creating display\n");
   }
   pictures[0] = al_load_bitmap("data/mysha.pcx");
   if (!pictures[0]) {
      abort_example("failed to load mysha.pcx\n");
   }

   display[1] = al_create_display(W, H);
   if (!display[1]) {
      abort_example("Error creating display\n");
   }
   pictures[1] = al_load_bitmap("data/allegro.pcx");
   if (!pictures[1]) {
      abort_example("failed to load allegro.pcx\n");
   }

   /* This is only needed since we want to receive resize events. */
   al_register_event_source(events, al_get_display_event_source(display[0]));
   al_register_event_source(events, al_get_display_event_source(display[1]));
   al_register_event_source(events, al_get_keyboard_event_source());

   while (1) {
      /* read input */
      while (!al_is_event_queue_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            ALLEGRO_KEYBOARD_EVENT *key = &event.keyboard;
            if (key->keycode == ALLEGRO_KEY_ESCAPE) {
               goto done;
            }
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            ALLEGRO_DISPLAY_EVENT *de = &event.display;
            al_acknowledge_resize(de->source);
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
            log_printf("%p switching in\n", event.display.source);
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
            log_printf("%p switching out\n", event.display.source);
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            int i;
            for (i = 0; i < 2; i++) {
               if (display[i] == event.display.source)
                  display[i] = 0;
            }
            al_destroy_display(event.display.source);
            for (i = 0; i < 2; i++) {
               if (display[i])
                  goto not_done;
            }
            goto done;
         not_done:
            ;
         }
      }

      for (i = 0; i < 2; i++) {
         if (!display[i])
            continue;

         target = al_get_backbuffer(display[i]);
         width = al_get_bitmap_width(target);
         height = al_get_bitmap_height(target);

         al_set_target_bitmap(target);
         al_draw_scaled_bitmap(pictures[0], 0, 0,
            al_get_bitmap_width(pictures[0]),
            al_get_bitmap_height(pictures[0]),
            0, 0,
            width / 2, height,
            0);
         al_draw_scaled_bitmap(pictures[1], 0, 0,
            al_get_bitmap_width(pictures[1]),
            al_get_bitmap_height(pictures[1]),
            width / 2, 0,
            width / 2, height,
            0);

         al_flip_display();
      }

      al_rest(0.001);
   }

done:
   al_destroy_bitmap(pictures[0]);
   al_destroy_bitmap(pictures[1]);
   al_destroy_display(display[0]);
   al_destroy_display(display[1]);

   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
