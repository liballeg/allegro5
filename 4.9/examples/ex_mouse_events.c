#include <allegro5/allegro5.h>
#include "allegro5/a5_iio.h"

#define NUM_BUTTONS  3

void draw_mouse_button(int but, bool down)
{
   const int offset[NUM_BUTTONS] = {0, 70, 35};
   ALLEGRO_COLOR grey;
   ALLEGRO_COLOR black;
   int x;
   int y;
   
   x = 400 + offset[but];
   y = 130;

   grey = al_map_rgb(0xe0, 0xe0, 0xe0);
   black = al_map_rgb(0, 0, 0);

   al_draw_rectangle(x, y, x + 26.5, y + 41.5, grey, ALLEGRO_FILLED);
   al_draw_rectangle(x, y, x + 26.5, y + 41.5, black, ALLEGRO_OUTLINED);
   if (down) {
      al_draw_rectangle(x + 2, y + 2, x + 24.5, y + 39.5, black,
         ALLEGRO_FILLED);
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   int mx = 0;
   int my = 0;
   bool buttons[NUM_BUTTONS] = {false};
   int i;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_mouse();
   al_install_keyboard();
   al_iio_init();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_hide_mouse_cursor();

   cursor = al_iio_load("data/cursor.tga");
   if (!cursor) {
      TRACE("Error loading cursor.tga\n");
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);

   while (1) {
      al_clear(al_map_rgb(0xff, 0xff, 0xc0));
      for (i = 0; i < NUM_BUTTONS; i++) {
         draw_mouse_button(i, buttons[i]);
      }
      al_draw_bitmap(cursor, mx, my, 0);
      al_flip_display();

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            if (event.mouse.button-1 < NUM_BUTTONS) {
               buttons[event.mouse.button-1] = true;
            }
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            if (event.mouse.button-1 < NUM_BUTTONS) {
               buttons[event.mouse.button-1] = false;
            }
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               goto done;
            }
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            goto done;
      }
   }

done:

   al_destroy_event_queue(queue);

   return 0;
}

END_OF_MAIN()

/* vim: set sw=3 sts=3 et: */
