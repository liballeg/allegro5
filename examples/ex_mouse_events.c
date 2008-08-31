#include <allegro5/allegro5.h>
#include "allegro5/a5_iio.h"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   int mx = 0;
   int my = 0;

   al_init();
   al_install_mouse();
   al_install_keyboard();
   al_iio_init();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_hide_mouse_cursor();

   cursor = al_iio_load("cursor.tga");
   if (!cursor) {
      TRACE("Error loading cursor.tga\n");
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (1) {
      if (!al_event_queue_is_empty(queue)) {
         while (al_get_next_event(queue, &event)) {
            switch (event.type) {
               case ALLEGRO_EVENT_MOUSE_AXES:
                  mx = event.mouse.x;
                  my = event.mouse.y;
                  break;

               case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                  goto done;

               case ALLEGRO_EVENT_KEY_DOWN:
                  if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                     goto done;
                  }
                  break;
            }
         }
      }

      al_clear(al_map_rgb(0, 0, 0));
      al_draw_bitmap(cursor, mx, my, 0);
      al_flip_display();
      al_rest(0.005);
   }

done:

   al_destroy_event_queue(queue);

   return 0;
}

END_OF_MAIN()

/* vi: set sw=3 sts=3 et: */
