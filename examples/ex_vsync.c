/* Tests vsync.
 */

#include <allegro5/allegro5.h>
#include <stdio.h>

#include "common.c"

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

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_CONFIG *config;
   ALLEGRO_EVENT_QUEUE *queue;
   int vsync, fullscreen, frequency;
   bool write = false;
   bool flip = false;
   bool quit = false;

   if (!al_init()) {
      printf("Could not init Allegro.\n");
      return 1;
   }

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
      al_save_config_file(config, "ex_vsync.ini");
   }
   al_destroy_config(config);

   /* Vsync 1 means force on, 2 means forced off. */
   if (vsync)
      al_set_new_display_option(ALLEGRO_VSYNC, vsync, ALLEGRO_SUGGEST);
   
   /* Force fullscreen mode. */
   if (fullscreen) {
      al_set_new_display_flags(ALLEGRO_FULLSCREEN);
      /* Set a monitor frequency. */
      if (frequency)
         al_set_new_display_refresh_rate(frequency);
   }

   display = al_create_display(640, 480);
   if (!display) {
      printf("Error creating display.\n");
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

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

   al_destroy_event_queue(queue);  

   return 0;
}

/* vim: set sts=3 sw=3 et: */
