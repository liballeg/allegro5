/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Future work:
 *       test memory bitmap scaling as well
 *       test flipping
 */


#include <allegro5/allegro5.h>
#include <math.h>



int main(int argc, char *argv[])
{
   const int display_w = 640;
   const int display_h = 480;

   ALLEGRO_DISPLAY *dpy;
   ALLEGRO_BITMAP *buf;
   ALLEGRO_BITMAP *bmp;
   int bmp_w;
   int bmp_h;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   double theta = 0;
   int mode;

   if (!al_init())
      return 1;

   al_install_keyboard();

   dpy = al_create_display(display_w, display_h);
   if (!dpy) {
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }

   buf = al_create_bitmap(display_w, display_h);
   if (!buf) {
      allegro_message("Unable to create buffer\n");
      return 1;
   }

   bmp = al_load_bitmap("mysha.pcx");
   if (!bmp) {
      allegro_message("Unable to load image\n%s\n", allegro_error);
      return 1;
   }
   bmp_w = al_get_bitmap_width(bmp);
   bmp_h = al_get_bitmap_height(bmp);

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   mode = 0;
   while (true) {
      if (al_get_next_event(queue, &event)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (event.keyboard.keycode) {
               case ALLEGRO_KEY_ESCAPE:
                  goto Quit;
               case ALLEGRO_KEY_SPACE:
                  mode = !mode;
                  break;
            }
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

      al_clear(al_map_rgba_f(1, 0, 0, 0));
      al_draw_scaled_bitmap(bmp,
         0, 0, bmp_w, bmp_h,
         display_w/2, display_h/2,
         cos(theta) * display_w/2, sin(theta) * display_h/2,
         0);

      if (mode == 0) {
         al_set_target_bitmap(al_get_backbuffer());
         al_clear(al_map_rgba_f(0, 0, 1, 0));
         al_draw_bitmap(buf, 0, 0, 0);
      }

      al_flip_display();
      al_rest(0.01);
      theta += 0.01;
   }

Quit:

   al_destroy_bitmap(bmp);
   al_destroy_bitmap(buf);

   return 0;
}
END_OF_MAIN()
