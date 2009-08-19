/*
 *       Example program for the Allegro library.
 *
 *       From left to right you should see Red, Green, Blue gradients.
 */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"

#include "common.c"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION *locked;
   uint8_t *ptr;
   int i, j;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();

   /* Create a window. */
   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(3*256, 256);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }

   /* Create the bitmap. */
   bitmap = al_create_bitmap(3*256, 256);

   /* Locking the bitmap means, we work directly with pixel data.  We can
    * choose the format we want to work with, which may imply conversions, or
    * use the bitmap's actual format so we can work directly with the bitmap's
    * pixel data.
    */
   locked = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ARGB_8888,
      ALLEGRO_LOCK_WRITEONLY);
   for (j = 0; j < 256; j++) {
      ptr = (uint8_t *)locked->data + j * locked->pitch;

      for (i = 0; i < 3*256; i++) {
         uint8_t red;
         uint8_t green;
         uint8_t blue;
         uint8_t alpha = 255;
         uint32_t col;
         uint32_t *cptr = (uint32_t *)ptr;

         if (i < 256) {
            red = 255;
            green = blue = j;
         }
         else if (i < 2*256) {
            green = 255;
            red = blue = j;
         }
         else {
            blue = 255;
            red = green = j;
         }

         /* The ARGB format means, the 32 bits per pixel are layed out like
          * this, least significant bit right:
          * A A A A A A A A R R R R R R R R G G G G G G G G B B B B B B B B
          * Because the byte order can vary per platform (big endian or
          * little endian) we encode a 32 bit integer and store that
          * directly rather than storing each component seperately.
          */
         col = (alpha << 24) | (red << 16) | (green << 8) | blue;
         *cptr = col;
         ptr += 4;
      }
   }
   al_unlock_bitmap(bitmap);

   al_draw_bitmap(bitmap, 0, 0, 0);
   al_flip_display();

   events = al_create_event_queue();
   al_register_event_source(events, al_get_keyboard_event_source());
   al_register_event_source(events, al_get_display_event_source(display));

   while (1) {
      al_wait_for_event(events, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE) {
          al_draw_bitmap(bitmap, 0, 0, 0);
          al_flip_display();
      }
   }

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
