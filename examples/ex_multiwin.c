#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"

const int W = 640;
const int H = 400;

int main(void)
{
   ALLEGRO_DISPLAY *display[2];
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_BITMAP *pictures[2];
   int i;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_install_mouse();
   al_iio_init();

   events = al_create_event_queue();

   al_set_new_display_flags(ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE);

   /* Create two windows. */
   display[0] = al_create_display(W, H);
   pictures[0] = al_iio_load("data/mysha.pcx");
   if (!pictures[0]) {
      TRACE("failed to load mysha.pcx\n");
      return 1;
   }

   display[1] = al_create_display(W, H);
   pictures[1] = al_iio_load("data/allegro.pcx");
   if (!pictures[1]) {
      TRACE("failed to load allegro.pcx\n");
      return 1;
   }
   al_show_mouse_cursor();

   /* This is only needed since we want to receive resize events. */
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display[0]);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display[1]);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (1) {
      /* read input */
      while (!al_event_queue_is_empty(events)) {
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
            printf("%p switching in\n", event.display.source);
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
            printf("%p switching out\n", event.display.source);
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

         al_set_current_display(display[i]);
         al_draw_scaled_bitmap(pictures[0], 0, 0,
            al_get_bitmap_width(pictures[0]),
            al_get_bitmap_height(pictures[0]),
            0, 0,
            al_get_display_width() / 2,
            al_get_display_height(), 0);
         al_draw_scaled_bitmap(pictures[1], 0, 0,
            al_get_bitmap_width(pictures[1]),
            al_get_bitmap_height(pictures[1]),
            al_get_display_width() / 2, 0,
            al_get_display_width() / 2,
            al_get_display_height(), 0);

         al_flip_display();
      }

      al_rest(0.001);
   }

done:
   al_destroy_bitmap(pictures[0]);
   al_destroy_bitmap(pictures[1]);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
