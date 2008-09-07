/*
 *    Simple (incomplete) test of pixel format conversions.
 *
 *    This should be made comprehensive.
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"


const enum ALLEGRO_PIXEL_FORMAT formats[] = {
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ARGB_8888,
   ALLEGRO_PIXEL_FORMAT_RGBA_8888,
   ALLEGRO_PIXEL_FORMAT_ARGB_4444,
   ALLEGRO_PIXEL_FORMAT_RGB_888,
   ALLEGRO_PIXEL_FORMAT_RGB_565,
   ALLEGRO_PIXEL_FORMAT_RGB_555,
   ALLEGRO_PIXEL_FORMAT_RGBA_5551,
   ALLEGRO_PIXEL_FORMAT_ARGB_1555,
   ALLEGRO_PIXEL_FORMAT_ABGR_8888,
   ALLEGRO_PIXEL_FORMAT_XBGR_8888,
   ALLEGRO_PIXEL_FORMAT_BGR_888,
   ALLEGRO_PIXEL_FORMAT_BGR_565,
   ALLEGRO_PIXEL_FORMAT_BGR_555,
   ALLEGRO_PIXEL_FORMAT_RGBX_8888,
   ALLEGRO_PIXEL_FORMAT_XRGB_8888
};

#define NUM_FORMATS  (sizeof(formats) / sizeof(formats[0]))


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_EVENT event;
   unsigned int i;
   int delta;

   al_init();
   al_iio_init();
   al_font_init();

   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());

   font = al_font_load_font("data/bmpfont.tga", NULL);
   if (!font) {
      TRACE("Failed to load bmpfont.tga\n");
      return 1;
   }

   i = 0;
   delta = 1;

   while (true) {
      i = (i + delta) % NUM_FORMATS;

      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      al_set_new_bitmap_format(formats[i]);

      bitmap = al_iio_load("data/allegro.pcx");
      if (!bitmap) {
         TRACE("Could not load image, bitmap format = %d\n", formats[i]);
         continue;
      }

      al_clear(al_map_rgb(0x80, 0x80, 0x80));

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 128, 80));
      al_font_textprintf_right(font, al_get_display_width()-1, 0, "%d", i);

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));
      al_draw_bitmap(bitmap, 0, 0, 0);

      al_flip_display();
      al_destroy_bitmap(bitmap);

      while (true) {
         al_wait_for_event(queue, &event);
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               goto Quit;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE ||
                  event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
               delta = +1;
               break;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_BACKSPACE ||
                  event.keyboard.keycode == ALLEGRO_KEY_LEFT) {
               delta = -1;
               break;
            }
         }
      }
   }

Quit:

   al_font_destroy_font(font);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
