/*
 *    Test program for Allegro.
 *
 *    Resizing the window currently shows broken behaviour.
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include <stdio.h>

#include "common.c"

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bmp;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   A5O_FONT *font;
   bool redraw;
   bool halt_drawing;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_install_keyboard();
   al_init_image_addon();
   al_init_font_addon();

   al_set_config_value(al_get_system_config(), "osx", "allow_live_resize", "false");
   al_set_new_display_flags(A5O_RESIZABLE |
      A5O_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to set any graphic mode\n");
   }

   al_set_new_bitmap_flags(A5O_MEMORY_BITMAP);
   bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Unable to load image\n");
   }

   font = al_create_builtin_font();

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());

   redraw = true;
   halt_drawing = false;
   while (true) {
      if (!halt_drawing && redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb(255, 0, 0));
         al_draw_scaled_bitmap(bmp,
            0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
            0, 0, al_get_display_width(display), al_get_display_height(display),
            0);
      al_draw_multiline_textf(font, al_map_rgb(255, 255, 0), 0, 0, 640,
         al_get_font_line_height(font), 0,
         "size: %d x %d\n"
         "maximized: %s\n"
         "+ key to maximize\n"
         "- key to un-maximize",
         al_get_display_width(display),
         al_get_display_height(display),
         al_get_display_flags(display) & A5O_MAXIMIZED ? "yes" :
         "no");

         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
         redraw = true;
      }
      if (event.type == A5O_EVENT_DISPLAY_EXPOSE) {
         redraw = true;
      }
      if (event.type == A5O_EVENT_DISPLAY_HALT_DRAWING) {
         halt_drawing = true;
         al_acknowledge_drawing_halt(display);
      }
      if (event.type == A5O_EVENT_DISPLAY_RESUME_DRAWING) {
         halt_drawing = false;
         al_acknowledge_drawing_resume(display);
      }
      if (event.type == A5O_EVENT_KEY_DOWN &&
            event.keyboard.keycode == A5O_KEY_ESCAPE) {
         break;
      }
      if (event.type == A5O_EVENT_KEY_CHAR &&
            event.keyboard.unichar == '+') {
         al_set_display_flag(display, A5O_MAXIMIZED, true);
      }
      if (event.type == A5O_EVENT_KEY_CHAR &&
            event.keyboard.unichar == '-') {
         al_set_display_flag(display, A5O_MAXIMIZED, false);
      }
      if (event.type == A5O_EVENT_DISPLAY_CLOSE) {
         break;
      }
   }

   al_destroy_bitmap(bmp);
   al_destroy_display(display);

   return 0;

}

/* vim: set sts=3 sw=3 et: */
