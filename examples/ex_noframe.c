#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"

#include <stdio.h>

#include "common.c"

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bitmap;
   A5O_EVENT_QUEUE *events;
   A5O_EVENT event;
   bool down = false;
   int down_x = 0, down_y = 0;
   A5O_TIMER *timer;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();
   init_platform_specific();

   al_set_new_display_flags(A5O_FRAMELESS);
   display = al_create_display(300, 200);
   if (!display) {
      abort_example("Error creating display\n");
   }
   
   bitmap = al_load_bitmap("data/fakeamp.bmp");
   if (!bitmap) {
      abort_example("Error loading fakeamp.bmp\n");
   }

   timer = al_create_timer(1.0f/30.0f);

   events = al_create_event_queue();
   al_register_event_source(events, al_get_mouse_event_source());
   al_register_event_source(events, al_get_keyboard_event_source());
   al_register_event_source(events, al_get_display_event_source(display));
   al_register_event_source(events, al_get_timer_event_source(timer));

   al_start_timer(timer);

   for (;;) {
      al_wait_for_event(events, &event);
      if (event.type == A5O_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1 && event.mouse.x) {
            down = true;
            down_x = event.mouse.x;
            down_y = event.mouse.y;
         }
         if (event.mouse.button == 2) {
            al_set_display_flag(display, A5O_FRAMELESS,
               !(al_get_display_flags(display) & A5O_FRAMELESS));
         }
      }
      else if (event.type == A5O_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == A5O_EVENT_MOUSE_BUTTON_UP) {
         if (event.mouse.button == 1) {
            down = false;
         }
      }
      else if (event.type == A5O_EVENT_MOUSE_AXES) {
         if (down) {
            int cx, cy;
            if (al_get_mouse_cursor_position(&cx, &cy)) {
               al_set_window_position(display, cx - down_x, cy - down_y);
            }
         }
      }
      else if (event.type == A5O_EVENT_KEY_DOWN &&
	    event.keyboard.keycode == A5O_KEY_ESCAPE) {
         break;
      }
      else if (event.type == A5O_EVENT_TIMER) {
         al_draw_bitmap(bitmap, 0, 0, 0);
         al_flip_display();
      }
   }

   al_destroy_timer(timer);
   al_destroy_event_queue(events);
   al_destroy_display(display);

   return 0;
}

