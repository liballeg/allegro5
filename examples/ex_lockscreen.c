#include "allegro5/allegro5.h"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION locked;
   int i;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;

   al_init();
   al_install_keyboard();

   al_set_new_display_format(ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA);
   display = al_create_display(100, 100);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   bitmap = al_get_backbuffer();
   al_lock_bitmap(bitmap, &locked, 0);
   for (i = 0; i < 100 * locked.pitch; i++) {
      ((char *)locked.data)[i] = (i / locked.pitch) * 255 / 99;
   }
   al_unlock_bitmap(bitmap);

   al_flip_display();

   events = al_create_event_queue();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (1) {
      al_wait_for_event(events, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&  
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
         break;
   }


   return 0;
}
END_OF_MAIN()
