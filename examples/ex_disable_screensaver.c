#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"

#include "common.c"

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;
   bool done = false;
   bool active = true;
   bool fullscreen = false;

   if (argc == 2) {
      if (!strcmp(argv[1], "-fullscreen")) {
         fullscreen = true;
      }
   }

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();
   al_init_font_addon();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS |
      (fullscreen ? ALLEGRO_FULLSCREEN : 0));

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   font = al_create_builtin_font();
   if (!font) {
      abort_example("Error creating builtin font\n");
   }

   events = al_create_event_queue();
   al_register_event_source(events, al_get_keyboard_event_source());
   /* For expose events */
   al_register_event_source(events, al_get_display_event_source(display));

   do {
      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_draw_textf(font, al_map_rgb_f(1, 1, 1), 0, 0, 0,
         "Screen saver: %s", active ? "Normal" : "Inhibited");
      al_flip_display();
      al_wait_for_event(events, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            else if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
               if (al_inhibit_screensaver(active)) {
                  active = !active;
               }
            }
            break;
      }
   } while (!done);

   al_destroy_font(font);
   al_destroy_event_queue(events);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
