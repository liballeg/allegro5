#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"
#include <stdio.h>

void go(void)
{
   ALLEGRO_DISPLAY *d1, *d2;
   ALLEGRO_BITMAP *b1, *b2;

   al_set_new_display_flags(ALLEGRO_FULLSCREEN);

   al_set_current_video_adapter(0);
   d1 = al_create_display(640, 480);
   if (!d1) {
      printf("Error creating first display\n");
      return;
   }
   b1 = iio_load("mysha.pcx");
   if (!b1) {
      printf("Error loading mysha.pcx\n");
      return;
   }

   al_set_current_video_adapter(1);
   d2 = al_create_display(640, 480);
   if (!d2) {
      printf("Error creating second display\n");
      return;
   }
   b2 = iio_load("allegro.pcx");
   if (!b2) {
      printf("Error loading allegro.pcx\n");
      return;
   }

   al_set_current_display(d1);
   al_draw_scaled_bitmap(b1, 0, 0, 320, 200, 0, 0, 640, 480, 0);
   al_flip_display();

   al_set_current_display(d2);
   al_draw_scaled_bitmap(b2, 0, 0, 320, 200, 0, 0, 640, 480, 0);
   al_flip_display();

   al_rest(10);

   al_destroy_bitmap(b1);
   al_destroy_bitmap(b2);
   al_destroy_display(d1);
   al_destroy_display(d2);
}

int main(void)
{
   al_init();
   iio_init();

   if (al_get_num_video_adapters() < 2) {
      TRACE("You need 2 or more adapters/monitors for this example.\n");
      return 1;
   }

   go();

   return 0;

}
END_OF_MAIN()

