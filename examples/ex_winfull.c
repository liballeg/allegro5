#include "allegro5/allegro.h"

#include "common.c"

int main(int argc, char **argv)
{
   A5O_DISPLAY *win, *full;
   A5O_EVENT_QUEUE *events;
   A5O_EVENT event;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();

   if (al_get_num_video_adapters() < 2) {
      abort_example("This example requires multiple video adapters.\n");
   }

   al_set_new_display_adapter(1);
   al_set_new_display_flags(A5O_WINDOWED);
   win = al_create_display(640, 480);
   if (!win) {
      abort_example("Error creating windowed display on adapter 1 "
	  "(do you have multiple adapters?)\n");
   }

   al_set_new_display_adapter(0);
   al_set_new_display_flags(A5O_FULLSCREEN);
   full = al_create_display(640, 480);
   if (!full) {
      abort_example("Error creating fullscreen display on adapter 0\n");
   }

   events = al_create_event_queue();
   al_register_event_source(events, al_get_keyboard_event_source());

   while (1) {
      while (!al_is_event_queue_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == A5O_EVENT_KEY_DOWN &&
               event.keyboard.keycode == A5O_KEY_ESCAPE)
            goto done;
      }

      al_set_target_backbuffer(full);
      al_clear_to_color(al_map_rgb(rand()%255, rand()%255, rand()%255));
      al_flip_display();

      al_set_target_backbuffer(win);
      al_clear_to_color(al_map_rgb(rand()%255, rand()%255, rand()%255));
      al_flip_display();

      al_rest(0.5);
   }

done:
   al_destroy_event_queue(events);

   al_destroy_display(win);
   al_destroy_display(full);

   return 0;
}

