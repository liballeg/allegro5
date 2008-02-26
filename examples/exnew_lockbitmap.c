#include "allegro5/allegro5.h"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION locked;
   uint8_t *ptr;
   int i, j;

   al_init();

   /* Create a 100 x 100 window. */
   display = al_create_display(100, 100);
   if (!display) {
      allegro_message("Error creating display");
      return 1;
   }

   /* Note: Usually it is a bad idea to create bitmaps not using the display
    * format, as additional conversion may be necessary. In this example,
    * we want direct access to the pixel memory though, so we better use
    * a format we know how to modify.
    */
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888);

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
         uint8_t alpha = 64;

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

         /* The ABGR format means, the 32 bits per pixel are layed out like
          * this, lowest bit right:
          * A A A A A A A A B B B B B B B B G G G G G G G G R R R R R R R R
          */
         *(ptr++) = red;
         *(ptr++) = green;
         *(ptr++) = blue;
         *(ptr++) = alpha;
      }

      ptr += locked.pitch - (4 * 100);
   }
   al_unlock_bitmap(bitmap);

   al_draw_bitmap(bitmap, 0, 0, 0);
   al_flip_display();
   al_rest(5.0);

   return 0;
}
END_OF_MAIN()
