/* Tests vsync.
 */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"

#include "common.c"

int vsync, fullscreen, frequency;

static int option(ALLEGRO_CONFIG *config, char *name, int v)
{
   char const *value;
   char str[256];
   value = al_get_config_value(config, "settings", name);
   if (value)
      v = strtol(value, NULL, 0);
   sprintf(str, "%d", v);
   al_set_config_value(config, "settings", name, str);
   return v;
}

static bool display_warning(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_FONT *font)
{
   ALLEGRO_EVENT event;
   float x = 320.0;
   float h = al_get_font_line_height(font);
   ALLEGRO_COLOR white = al_map_rgb_f(1, 1, 1);

   for (;;) {
      float y = 200.0;
      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_draw_text(font, white, x, y, ALLEGRO_ALIGN_CENTRE,
         "Do not continue if you suffer from photosensitive epilepsy");
      al_draw_text(font, white, x, y + 15, ALLEGRO_ALIGN_CENTRE,
         "or simply hate flashing screens.");
      al_draw_text(font, white, x, y + 40, ALLEGRO_ALIGN_CENTRE,
         "Press Escape to quit or Enter to continue.");
      
      y += 100;
      al_draw_text(font, white, x, y, ALLEGRO_ALIGN_CENTRE, "Parameters from ex_vsync.ini:");
      y += h;
      al_draw_textf(font, white, x, y, ALLEGRO_ALIGN_CENTRE, "vsync: %d", vsync);
      y += h;
      al_draw_textf(font, white, x, y, ALLEGRO_ALIGN_CENTRE, "fullscreen: %d", fullscreen);
      y += h;
      al_draw_textf(font, white, x, y, ALLEGRO_ALIGN_CENTRE, "frequency: %d", frequency);
      
      al_flip_display();

      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            return true;
         }
         if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
            return false;
         }
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         return true;
      }
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;
   ALLEGRO_CONFIG *config;
   ALLEGRO_EVENT_QUEUE *queue;
   bool write = false;
   bool flip = false;
   bool quit;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_font_addon();
   al_init_image_addon();
   al_install_keyboard();
   al_install_mouse();

   /* Read parameters from ex_vsync.ini. */
   config = al_load_config_file("ex_vsync.ini");
   if (!config) {
      config = al_create_config();
      write = true;
   }

   /* 0 -> Driver chooses.
    * 1 -> Force vsync on.
    * 2 -> Force vsync off.
    */
   vsync = option(config, "vsync", 0);

   fullscreen = option(config, "fullscreen", 0);
   frequency = option(config, "frequency", 0);

   /* Write the file back (so a template is generated on first run). */
   if (write) {
      al_save_config_file("ex_vsync.ini", config);
   }
   al_destroy_config(config);

   /* Vsync 1 means force on, 2 means forced off. */
   if (vsync)
      al_set_new_display_option(ALLEGRO_VSYNC, vsync, ALLEGRO_SUGGEST);
   
   /* Force fullscreen mode. */
   if (fullscreen) {
      al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
      /* Set a monitor frequency. */
      if (frequency)
         al_set_new_display_refresh_rate(frequency);
   }

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   font = al_load_font("data/a4_font.tga", 0, 0);
   if (!font) {
      abort_example("Failed to load a4_font.tga\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   quit = display_warning(queue, font);
   al_flush_event_queue(queue);

   while (!quit) {
      ALLEGRO_EVENT event;

      /* With vsync, this will appear as a 50% gray screen (maybe
       * flickering a bit depending on monitor frequency).
       * Without vsync, there will be black/white shearing all over.
       */
      if (flip)
         al_clear_to_color(al_map_rgb_f(1, 1, 1));
      else
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
      
      al_flip_display();

      flip = !flip;

      while (al_get_next_event(queue, &event)) {
         switch (event.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
               quit = true;

            case ALLEGRO_EVENT_KEY_DOWN:
               if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                  quit = true;
         }
      }
      /* Let's not go overboard and limit flipping at 1000 Hz. Without
       * this my system locks up and requires a hard reboot :P
       */
      al_rest(0.001);
   }

   al_destroy_font(font);
   al_destroy_event_queue(queue);  

   return 0;
}

/* vim: set sts=3 sw=3 et: */
