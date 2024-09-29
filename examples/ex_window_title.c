/* An example showing how to set the title of a window, by Beoran. */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#include "common.c"

#define INTERVAL 1.0


float bmp_x = 200;
float bmp_y = 200;
float bmp_dx = 96;
float bmp_dy = 96;
int bmp_flag = 0;

#define NEW_WINDOW_TITLE "A Custom Window Title. Press space to start changing it."
#define TITLE_SIZE 256

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_FONT *font;
   int step = 0;
   const char *text;
   char   title[TITLE_SIZE] = "";
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

   text = NEW_WINDOW_TITLE;

   al_set_new_window_title(NEW_WINDOW_TITLE);

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

   text = al_get_new_window_title();

   timer = al_create_timer(INTERVAL);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(display));



   while (!done) {
      A5O_EVENT event;

      if (redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         al_draw_text(font, al_map_rgba_f(1, 1, 1, 0.5), 0, 0, 0, text);
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE)
               done = true;
            else if (event.keyboard.keycode == A5O_KEY_SPACE)
               al_start_timer(timer);
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case A5O_EVENT_TIMER:
            redraw = true;
            step++;
            sprintf(title, "Title: %d", step);
            text = title;
            al_set_window_title(display, title);
            break;
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
