#include <allegro5/allegro.h>
#include "allegro5/allegro_image.h"

#include "common.c"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *icon1;
   ALLEGRO_BITMAP *icon2;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_TIMER *timer;
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_install_keyboard();
   al_init_image_addon();

   display = al_create_display(320, 200);
   if (!display) {
      abort_example("Error creating display\n");
   }
   al_clear_to_color(al_map_rgb_f(0, 0, 0));
   al_flip_display();

   /* First icon: Read from file. */
   icon1 = al_load_bitmap("data/icon.tga");
   if (!icon1) {
      abort_example("icon.tga not found\n");
   }

   /* Second icon: Create it. */
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   icon2 = al_create_bitmap(16, 16);
   al_set_target_bitmap(icon2);
   for (i = 0; i < 256; i++) {
      int u = i % 16;
      int v = i / 16;
      al_put_pixel(u, v, al_map_rgb_f(u / 15.0, v / 15.0, 1));
   }
   al_set_target_backbuffer(display);

   al_set_window_title(display, "Changing icon example");

   timer = al_create_timer(0.5);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   for (;;) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_TIMER) {
         al_set_display_icon(display, (event.timer.count & 1) ? icon2 : icon1);
      }
   }

   al_uninstall_system();

   return 0;
}
