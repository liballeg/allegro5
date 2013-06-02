/*
 *    Test program for Allegro.
 *
 *    Resizing the window currently shows broken behaviour.
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include <stdio.h>

#include "common.c"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   bool redraw;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_RESIZABLE |
      ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to set any graphic mode\n");
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Unable to load image\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());

   redraw = true;
   while (true) {
      if (redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb(255, 0, 0));
         al_draw_scaled_bitmap(bmp,
            0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
            0, 0, al_get_display_width(display), al_get_display_height(display),
            0);
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
         redraw = true;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE) {
         redraw = true;
      }
      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
   }

   al_destroy_bitmap(bmp);
   al_destroy_display(display);

   return 0;

}

/* vim: set sts=3 sw=3 et: */
