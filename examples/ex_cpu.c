/* An example showing the use of al_get_cpu_count() and al_get_ram_size(). */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#include "common.c"

#define INTERVAL 0.1



int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;
   bool done = false;
   bool redraw = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   al_init_font_addon();
   init_platform_specific();


   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   font = al_create_builtin_font();

   timer = al_create_timer(INTERVAL);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

   while (!done) {
      ALLEGRO_EVENT event;

      if (redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgba_f(0, 0, 0, 1.0));
         al_draw_textf(font, al_map_rgba_f(1, 1, 0, 1.0), 16, 16, 0,
                       "Amount of CPU cores detected: %d.", al_get_cpu_count());
         al_draw_textf(font, al_map_rgba_f(0, 1, 1, 1.0), 16, 32, 0,
                       "Size of random access memory: %d MiB.", al_get_ram_size());
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               done = true;
            }
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;
      }
   }

   al_destroy_font(font);

   return 0;
}


/* vim: set sts=3 sw=3 et: */
