/* Like exnew_mouse_events.c but with multiple windows. */

#include <allegro5/allegro5.h>

int main(void)
{
   ALLEGRO_DISPLAY *display[2];
   ALLEGRO_DISPLAY *mouse_display = NULL;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   int mx = 0;
   int my = 0;
   int i;

   al_init();

   al_install_mouse();
   al_install_keyboard();

   for (i = 0; i < 2; i++) {
      display[i] = al_create_display(640, 480);
      if (!display[i]) {
         allegro_message("Error creating display");
         return 1;
      }
      printf("created\n");
   }

   //al_hide_mouse_cursor();

   printf("loading\n");

   cursor = al_load_bitmap("cursor.tga");
   if (!cursor) {
      allegro_message("Error loading cursor.tga");
      return 1;
   }

   printf("register\n");

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   printf("begin\n");

   while (1) {
      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            mouse_display = event.mouse.display;
            break;

         /*
         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            goto done;
         */

         case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
            break;

         case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               goto done;
            }
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            goto done;
      }

      for (i = 0; i < 2; i++) {
         al_set_current_display(display[i]);
         al_clear(al_map_rgb(0, 0, 0));
         if (mouse_display == display[i]) {
            al_draw_bitmap(cursor, mx, my, 0);
         }
         al_flip_display();
      }
   }

done:

   al_destroy_event_queue(queue);

   return 0;
}

END_OF_MAIN()

/* vi: set sw=3 sts=3 et: */
