#include "allegro5/allegro5.h"

int main(void)
{
   ALLEGRO_DISPLAY *win, *full;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();

   al_set_current_video_adapter(1);
   al_set_new_display_flags(ALLEGRO_WINDOWED);
   win = al_create_display(640, 480);
   if (!win) {
      TRACE("Error creating windowed display\n");
      return 1;
   }

   al_set_current_video_adapter(0);
   al_set_new_display_flags(ALLEGRO_FULLSCREEN);
   full = al_create_display(640, 480);
   if (!full) {
      TRACE("Error creating fullscreen display\n");
      return 1;
   }

   events = al_create_event_queue();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (1) {
      while (!al_event_queue_is_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
               event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            goto done;
      }

      al_set_current_display(full);
      al_clear_to_color(al_map_rgb(rand()%255, rand()%255, rand()%255));
      al_flip_display();

      al_set_current_display(win);
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
END_OF_MAIN()

