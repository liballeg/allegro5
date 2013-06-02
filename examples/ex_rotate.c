/*
 *    Example program for the Allegro library, by Peter Wang.
 */


#include <allegro5/allegro.h>
#include "allegro5/allegro_image.h"
#include <math.h>

#include "common.c"

int main(void)
{
   const int display_w = 640;
   const int display_h = 480;

   ALLEGRO_DISPLAY *dpy;
   ALLEGRO_BITMAP *buf;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_BITMAP *mem_bmp;
   ALLEGRO_BITMAP *src_bmp;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   double theta = 0;
   double k = 1.0;
   int mode = 0;
   bool wide_mode = false;
   bool mem_src_mode = false;
   bool trans_mode = false;
   int flags = 0;
   bool clip_mode = false;
   ALLEGRO_COLOR trans;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_install_keyboard();
   al_init_image_addon();

   open_log();
   log_printf("Press 'w' to toggle wide mode.\n");
   log_printf("Press 's' to toggle memory source bitmap.\n");
   log_printf("Press space to toggle drawing to backbuffer or off-screen bitmap.\n");
   log_printf("Press 't' to toggle translucency.\n");
   log_printf("Press 'h' to toggle horizontal flipping.\n");
   log_printf("Press 'v' to toggle vertical flipping.\n");
   log_printf("Press 'c' to toggle clipping.\n");
   log_printf("\n");

   dpy = al_create_display(display_w, display_h);
   if (!dpy) {
      abort_example("Unable to set any graphic mode\n");
   }

   buf = al_create_bitmap(display_w, display_h);
   if (!buf) {
      abort_example("Unable to create buffer\n\n");
   }

   bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Unable to load image\n");
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   mem_bmp = al_load_bitmap("data/mysha.pcx");
   if (!mem_bmp) {
      abort_example("Unable to load image\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   while (true) {
      if (al_get_next_event(queue, &event)) {
         if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               break;
            if (event.keyboard.unichar == ' ') {
               mode = !mode;
               if (mode == 0)
                  log_printf("Drawing to off-screen buffer\n");
               else
                  log_printf("Drawing to display backbuffer\n");
            }
            if (event.keyboard.unichar == 'w')
               wide_mode = !wide_mode;
            if (event.keyboard.unichar == 's') {
               mem_src_mode = !mem_src_mode;
               if (mem_src_mode)
                  log_printf("Source is memory bitmap\n");
               else
                  log_printf("Source is display bitmap\n");
            }
            if (event.keyboard.unichar == 't')
               trans_mode = !trans_mode;
            if (event.keyboard.unichar == 'h')
               flags ^= ALLEGRO_FLIP_HORIZONTAL;
            if (event.keyboard.unichar == 'v')
               flags ^= ALLEGRO_FLIP_VERTICAL;
            if (event.keyboard.unichar == 'c')
               clip_mode = !clip_mode;
         }
      }

      /*
       * mode 0 = draw scaled to off-screen buffer before
       *          blitting to display backbuffer
       * mode 1 = draw scaled to display backbuffer
       */

      if (mode == 0) {
         al_set_target_bitmap(buf);
      }
      else {
         al_set_target_backbuffer(dpy);
      }

      src_bmp = (mem_src_mode) ? mem_bmp : bmp;
      k = (wide_mode) ? 2.0 : 1.0;

      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
      trans = al_map_rgba_f(1, 1, 1, 1);
      if (mode == 0)
         al_clear_to_color(al_map_rgba_f(1, 0, 0, 1));
      else
         al_clear_to_color(al_map_rgba_f(0, 0, 1, 1));

      if (trans_mode) {
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
         trans = al_map_rgba_f(1, 1, 1, 0.5);
      }

      if (clip_mode) {
         al_set_clipping_rectangle(50, 50, display_w - 100, display_h - 100);
      }
      else {
         al_set_clipping_rectangle(0, 0, display_w, display_h);
      }

      al_draw_tinted_scaled_rotated_bitmap(src_bmp,
         trans,
         50, 50, display_w/2, display_h/2,
         k, k, theta,
         flags);

      if (mode == 0) {
         al_set_target_backbuffer(dpy);
         al_set_clipping_rectangle(0, 0, display_w, display_h);
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
         al_draw_bitmap(buf, 0, 0, 0);
      }

      al_flip_display();
      al_rest(0.01);
      theta -= 0.01;
   }

   al_destroy_bitmap(bmp);
   al_destroy_bitmap(mem_bmp);
   al_destroy_bitmap(buf);

   close_log(false);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
