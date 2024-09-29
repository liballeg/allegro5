/*
 *    Test program for Allegro.
 *
 *    Constrains the window to a minimum and or maximum.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"

#include "common.c"

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bmp;
   A5O_FONT *f;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   bool redraw;
   int min_w, min_h, max_w, max_h;
   int ret_min_w, ret_min_h, ret_max_w, ret_max_h;
   bool constr_min_w, constr_min_h, constr_max_w, constr_max_h;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();
   al_init_image_addon();
   al_init_font_addon();

   al_set_new_display_flags(A5O_RESIZABLE |
      A5O_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to set any graphic mode\n");
   }

   bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Unable to load image\n");
   }

   f = al_load_font("data/a4_font.tga", 0, 0);
   if (!f) {
      abort_example("Failed to load a4_font.tga\n");
   }

   min_w = 640;
   min_h = 480;
   max_w = 800;
   max_h = 600;
   constr_min_w = constr_min_h = constr_max_w = constr_max_h = true;

   if (!al_set_window_constraints(
          display,
          constr_min_w ? min_w : 0,
          constr_min_h ? min_h : 0,
          constr_max_w ? max_w : 0,
          constr_max_h ? max_h : 0)) {
      abort_example("Unable to set window constraints.\n");
   }

   al_apply_window_constraints(display, true);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());

   redraw = true;
   while (true) {
      if (redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb(255, 0, 0));
         al_draw_scaled_bitmap(bmp,
            0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
            0, 0, al_get_display_width(display), al_get_display_height(display),
            0);

         /* Display screen resolution */
         al_draw_textf(f, al_map_rgb(255, 255, 255), 0, 0, 0,
            "Resolution: %dx%d",
            al_get_display_width(display), al_get_display_height(display));

         if (!al_get_window_constraints(display, &ret_min_w, &ret_min_h,
               &ret_max_w, &ret_max_h))
         {
            abort_example("Unable to get window constraints\n");
         }

         al_draw_textf(f, al_map_rgb(255, 255, 255), 0,
            al_get_font_line_height(f), 0,
            "Min Width: %d, Min Height: %d, Max Width: %d, Max Height: %d",
            ret_min_w, ret_min_h, ret_max_w, ret_max_h);

         al_draw_textf(f, al_map_rgb(255, 255, 255), 0,
            al_get_font_line_height(f) * 2,0,
            "Toggle Restriction: Min Width: Z, Min Height: X, Max Width: C, Max Height: V");

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
      if (event.type == A5O_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
            break;
         }
         else if (event.keyboard.keycode == A5O_KEY_Z) {
            constr_min_w = ! constr_min_w;
         }
         else if (event.keyboard.keycode == A5O_KEY_X) {
            constr_min_h = ! constr_min_h;
         }
         else if (event.keyboard.keycode == A5O_KEY_C) {
            constr_max_w = ! constr_max_w;
         }
         else if (event.keyboard.keycode == A5O_KEY_V) {
            constr_max_h = ! constr_max_h;
         }

         redraw = true;

         if (!al_set_window_constraints(display,
               constr_min_w ? min_w : 0,
               constr_min_h ? min_h : 0,
               constr_max_w ? max_w : 0,
               constr_max_h ? max_h : 0)) {
            abort_example("Unable to set window constraints.\n");
         }
         al_apply_window_constraints(display, true);
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
