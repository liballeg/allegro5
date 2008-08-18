#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"

#include <stdio.h>

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;
   bool down = false;
   int down_x, down_y;
   ALLEGRO_TIMER *timer;
   bool frame = false;

   al_init();
   al_install_mouse();
   al_install_keyboard();

   al_set_new_display_flags(ALLEGRO_NOFRAME);
   display = al_create_display(300, 200);
   
   bitmap = al_load_bitmap("fakeamp.bmp");

   al_show_mouse_cursor();

   timer = al_install_timer(1.0f/30.0f);

   events = al_create_event_queue();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)timer);

   al_start_timer(timer);

   for (;;) {
      al_wait_for_event(events, &event);
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1 && event.mouse.x) {
            down = true;
            down_x = event.mouse.x;
            down_y = event.mouse.y;
         }
         if (event.mouse.button == 2) {
            frame = !frame;
            al_toggle_window_frame(display, frame);
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         if (event.mouse.button == 1) {
            down = false;
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         if (down) {
            int cx, cy;
            al_get_cursor_position(&cx, &cy);
            al_set_window_position(display, cx - down_x, cy - down_y);
         }
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_TIMER) {
         al_draw_bitmap(bitmap, 0, 0, 0);
         al_flip_display();
      }
   }

   al_uninstall_timer(timer);
   al_destroy_event_queue(events);
   al_destroy_display(display);

   return 0;
}
END_OF_MAIN()

