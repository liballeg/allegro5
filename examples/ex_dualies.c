#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include <stdio.h>

#include "common.c"

static void go(void)
{
   ALLEGRO_DISPLAY *d1, *d2;
   ALLEGRO_BITMAP *b1, *b2;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;

   al_set_new_display_flags(ALLEGRO_FULLSCREEN);

   al_set_new_display_adapter(0);
   d1 = al_create_display(640, 480);
   if (!d1) {
      abort_example("Error creating first display\n");
   }
   b1 = al_load_bitmap("data/mysha.pcx");
   if (!b1) {
      abort_example("Error loading mysha.pcx\n");
   }

   al_set_new_display_adapter(1);
   d2 = al_create_display(640, 480);
   if (!d2) {
      abort_example("Error creating second display\n");
   }
   b2 = al_load_bitmap("data/allegro.pcx");
   if (!b2) {
      abort_example("Error loading allegro.pcx\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   while (1) {
      if (!al_is_event_queue_empty(queue)) {
         al_get_next_event(queue, &event);
	 if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
	    if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
	       break;
	    }
	 }
      }

      al_set_target_backbuffer(d1);
      al_draw_scaled_bitmap(b1, 0, 0, 320, 200, 0, 0, 640, 480, 0);
      al_flip_display();

      al_set_target_backbuffer(d2);
      al_draw_scaled_bitmap(b2, 0, 0, 320, 200, 0, 0, 640, 480, 0);
      al_flip_display();

      al_rest(0.1);
   }

   al_destroy_bitmap(b1);
   al_destroy_bitmap(b2);
   al_destroy_display(d1);
   al_destroy_display(d2);
}

int main(void)
{
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();

   al_init_image_addon();

   if (al_get_num_video_adapters() < 2) {
      abort_example("You need 2 or more adapters/monitors for this example.\n");
   }

   go();

   return 0;

}

