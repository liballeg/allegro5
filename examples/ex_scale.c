/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Press 'w' to toggle "wide" mode.
 *    Press 's' to toggle memory source bitmap.
 *    Press ' ' to toggle scaling to backbuffer or off-screen bitmap.
 *    Press 't' to toggle translucency.
 *    Press 'h' to toggle horizontal flipping.
 *    Press 'v' to toggle vertical flipping.
 */


#include <allegro5/allegro5.h>
#include "allegro5/a5_iio.h"
#include <math.h>



int main(void)
{
   const int display_w = 640;
   const int display_h = 480;

   ALLEGRO_DISPLAY *dpy;
   ALLEGRO_BITMAP *buf;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_BITMAP *mem_bmp;
   ALLEGRO_BITMAP *src_bmp;
   int bmp_w;
   int bmp_h;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   double theta = 0;
   double k = 1.0;
   int mode = 0;
   bool wide_mode = false;
   bool mem_src_mode = false;
   bool trans_mode = false;
   int flags = 0;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_iio_init();

   dpy = al_create_display(display_w, display_h);
   if (!dpy) {
      TRACE("Unable to set any graphic mode\n");
      return 1;
   }

   buf = al_create_bitmap(display_w, display_h);
   if (!buf) {
      TRACE("Unable to create buffer\n\n");
      return 1;
   }

   bmp = al_iio_load("data/mysha.pcx");
   if (!bmp) {
      TRACE("Unable to load image\n");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   mem_bmp = al_iio_load("data/mysha.pcx");
   if (!mem_bmp) {
      TRACE("Unable to load image\n");
      return 1;
   }

   bmp_w = al_get_bitmap_width(bmp);
   bmp_h = al_get_bitmap_height(bmp);

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (true) {
      if (al_get_next_event(queue, &event)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               break;
            if (event.keyboard.unichar == ' ')
               mode = !mode;
            if (event.keyboard.unichar == 'w')
               wide_mode = !wide_mode;
            if (event.keyboard.unichar == 's')
               mem_src_mode = !mem_src_mode;
	    if (event.keyboard.unichar == 't')
		trans_mode = !trans_mode;
            if (event.keyboard.unichar == 'h')
               flags ^= ALLEGRO_FLIP_HORIZONTAL;
            if (event.keyboard.unichar == 'v')
               flags ^= ALLEGRO_FLIP_VERTICAL;
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
         al_set_target_bitmap(al_get_backbuffer());
      }

      src_bmp = (mem_src_mode) ? mem_bmp : bmp;
      k = (wide_mode) ? 2.0 : 1.0;

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
      if (mode == 0)
         al_clear(al_map_rgba_f(1, 0, 0, 1));
      else
         al_clear(al_map_rgba_f(0, 0, 1, 1));

      if (trans_mode) {
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
            al_map_rgba_f(1, 1, 1, 0.5));
      }

      al_draw_scaled_bitmap(src_bmp,
         0, 0, bmp_w, bmp_h,
         display_w/2, display_h/2,
         k * cos(theta) * display_w/2, k * sin(theta) * display_h/2,
         flags);

      if (mode == 0) {
         al_set_target_bitmap(al_get_backbuffer());
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         al_draw_bitmap(buf, 0, 0, 0);
      }

      al_flip_display();
      al_rest(0.01);
      theta += 0.01;
   }

   al_destroy_bitmap(bmp);
   al_destroy_bitmap(mem_bmp);
   al_destroy_bitmap(buf);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
