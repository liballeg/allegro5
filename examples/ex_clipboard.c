/* An example showing bitmap flipping flags, by Steven Wallace. */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#include "common.c"

#define INTERVAL 0.1



int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_FONT *font;
   char *text = NULL;
   bool done = false;
   bool redraw = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
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

   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      abort_example("Error loading data/fixed_font.tga\n");
   }

   timer = al_create_timer(INTERVAL);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);

   while (!done) {
      A5O_EVENT event;

      if (redraw && al_is_event_queue_empty(queue)) {
         if (text)
            al_free(text);
         if (al_clipboard_has_text(display)) {
            text = al_get_clipboard_text(display);
         }
         else {
            text = NULL;
         }


         al_clear_to_color(al_map_rgb_f(0, 0, 0));

         if (text) {
            al_draw_text(font, al_map_rgba_f(1, 1, 1, 1.0), 0, 0, 0, text);
         }
         else {
            al_draw_text(font, al_map_rgba_f(1, 0, 0, 1.0), 0, 0, 0,
                         "No clipboard text available.");
         }
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
               done = true;
            }
            else if (event.keyboard.keycode == A5O_KEY_SPACE) {
               al_set_clipboard_text(display, "Copied from Allegro!");
            }
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case A5O_EVENT_TIMER:
            redraw = true;
            break;
      }
   }

   al_destroy_font(font);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
