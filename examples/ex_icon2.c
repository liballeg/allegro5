/*
 *      Example program for the Allegro library.
 *
 *      Set multiple window icons, a big one and a small one.
 *      The small would would be used for the task bar,
 *      and the big one for the alt-tab popup, for example.
 */

#include <allegro5/allegro.h>
#include "allegro5/allegro_image.h"

#include "common.c"

#define NUM_ICONS 2

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *icons[NUM_ICONS];
   ALLEGRO_EVENT_QUEUE *queue;
   int u, v;

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

   /* First icon 16x16: Read from file. */
   icons[0] = al_load_bitmap("data/cursor.tga");
   if (!icons[0]) {
      abort_example("icons.tga not found\n");
   }

   /* Second icon 32x32: Create it. */
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   icons[1] = al_create_bitmap(32, 32);
   al_set_target_bitmap(icons[1]);
   for (v = 0; v < 32; v++) {
      for (u = 0; u < 32; u++) {
         al_put_pixel(u, v, al_map_rgb_f(u / 31.0, v / 31.0, 1));
      }
   }
   al_set_target_backbuffer(display);

   al_set_display_icons(display, NUM_ICONS, icons);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

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
   }

   al_uninstall_system();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
