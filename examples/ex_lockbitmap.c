#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION locked;
   uint8_t *ptr;
   int i, j;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_iio_init();

   /* Create a 100 x 100 window. */
   display = al_create_display(100, 100);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   /* Note: Usually it is a bad idea to create bitmaps not using the display
    * format, as additional conversion may be necessary. In this example,
    * we want direct access to the pixel memory though, so we better use
    * a format we know how to modify.
    */
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);

   /* Create the bitmap. */
   bitmap = al_create_bitmap(100, 100);

   /* Locking the bitmap means, we get direct access to its pixel data, in
    * whatever format it uses.
    */
   al_lock_bitmap(bitmap, &locked, 0);
   ptr = locked.data;
   for (j = 0; j < 100; j++) {
      for (i = 0; i < 100; i++) {
         uint8_t red = 0;
         uint8_t green = 0;
         uint8_t blue = 0;
         uint8_t alpha = 255;
         uint32_t col;
         uint32_t *cptr = (uint32_t *)ptr;

         if (j < 50) {
            if (i < 50)
               red = 223;
            else
               green = 127;
         }
         else {
            if (i < 50)
               blue = 255;
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
         ptr+=4;
      }

      ptr += locked.pitch - (4 * 100);
   }
   al_unlock_bitmap(bitmap);

   al_draw_bitmap(bitmap, 0, 0, 0);
   al_flip_display();

   events = al_create_event_queue();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (1) {
      al_wait_for_event(events, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&  
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
         break;
   }

   return 0;
}
END_OF_MAIN()
