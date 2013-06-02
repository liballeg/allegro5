/*
 *       Example program for the Allegro library.
 *
 *       From left to right you should see Red, Green, Blue gradients.
 */

#include "allegro5/allegro.h"
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
   int mode = 0;
   int lock_flags = ALLEGRO_LOCK_WRITEONLY;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();

   open_log();

   /* Create a window. */
   display = al_create_display(3*256, 256);
   if (!display) {
      abort_example("Error creating display\n");
   }

   events = al_create_event_queue();
   al_register_event_source(events, al_get_keyboard_event_source());

   log_printf("Press space to change bitmap type\n");
   log_printf("Press w to toggle WRITEONLY mode\n");

restart:

   /* Create the bitmap to lock, or use the display backbuffer. */
   if (mode == 0) {
      log_printf("Locking video bitmap");
      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
      bitmap = al_create_bitmap(3*256, 256);
   }
   else if (mode == 1) {
      log_printf("Locking memory bitmap");
      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      bitmap = al_create_bitmap(3*256, 256);
   }
   else {
      log_printf("Locking display backbuffer");
      bitmap = al_get_backbuffer(display);
   }
   if (!bitmap) {
      abort_example("Error creating bitmap");
   }

   if (lock_flags & ALLEGRO_LOCK_WRITEONLY) {
      log_printf(" in write-only mode\n");
   }
   else {
      log_printf(" in read/write mode\n");
   }

   al_set_target_bitmap(bitmap);
   al_clear_to_color(al_map_rgb_f(0.8, 0.8, 0.9));
   al_set_target_backbuffer(display);

   /* Locking the bitmap means, we work directly with pixel data.  We can
    * choose the format we want to work with, which may imply conversions, or
    * use the bitmap's actual format so we can work directly with the bitmap's
    * pixel data.
    * We use a 16-bit format and odd positions and sizes to increase the
    * chances of uncovering bugs.
    */
   locked = al_lock_bitmap_region(bitmap, 193, 65, 3*127, 127,
      ALLEGRO_PIXEL_FORMAT_RGB_565, lock_flags);
   for (j = 0; j < 127; j++) {
      ptr = (uint8_t *)locked->data + j * locked->pitch;

      for (i = 0; i < 3*127; i++) {
         uint8_t red;
         uint8_t green;
         uint8_t blue;
         uint16_t col;
         uint16_t *cptr = (uint16_t *)ptr;

         if (j == 0 || j == 126 || i == 0 || i == 3*127-1) {
            red = green = blue = 0;
         }
         else if (i < 127) {
            red = 255;
            green = blue = j*2;
         }
         else if (i < 2*127) {
            green = 255;
            red = blue = j*2;
         }
         else {
            blue = 255;
            red = green = j*2;
         }

         /* The RGB_555 format means, the 16 bits per pixel are laid out like
          * this, least significant bit right: RRRRR GGGGGG BBBBB
          * Because the byte order can vary per platform (big endian or
          * little endian) we encode an integer and store that
          * directly rather than storing each component separately.
          */
         col = ((red/8) << 11) | ((green/4) << 5) | (blue/8);
         *cptr = col;
         ptr += 2;
      }
   }
   al_unlock_bitmap(bitmap);

   if (mode < 2) {
      al_draw_bitmap(bitmap, 0, 0, 0);
      al_destroy_bitmap(bitmap);
      bitmap = NULL;
   }

   al_flip_display();

   while (1) {
      al_wait_for_event(events, &event);

      if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;

         if (event.keyboard.unichar == ' ') {
            if (++mode > 2)
               mode = 0;
            goto restart;
         }

         if (event.keyboard.unichar == 'w' || event.keyboard.unichar == 'W') {
            lock_flags ^= ALLEGRO_LOCK_WRITEONLY;
            goto restart;
         }
      }
   }

   close_log(false);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
