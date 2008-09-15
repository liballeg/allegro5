/*
 *    Simple (incomplete) test of pixel format conversions.
 *
 *    This should be made comprehensive.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"


typedef struct FORMAT
{
   int format;
   char const *name;
} FORMAT;

const FORMAT formats[] = {
   {ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA, "any"},
   {ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA, "alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA, "15"},
   {ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA, "15 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA, "16"},
   {ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA, "16 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA, "24"},
   {ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA, "24 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA, "32"},
   {ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA, "32 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ARGB_8888, "ARGB8888"},
   {ALLEGRO_PIXEL_FORMAT_RGBA_8888, "RGBA8888"},
   {ALLEGRO_PIXEL_FORMAT_ARGB_4444, "ARGB4444"},
   {ALLEGRO_PIXEL_FORMAT_RGB_888, "RGB888"},
   {ALLEGRO_PIXEL_FORMAT_RGB_565, "RGB565"},
   {ALLEGRO_PIXEL_FORMAT_RGB_555, "RGB555"},
   {ALLEGRO_PIXEL_FORMAT_RGBA_5551, "RGBA5551"},
   {ALLEGRO_PIXEL_FORMAT_ARGB_1555, "ARGB1555"},
   {ALLEGRO_PIXEL_FORMAT_ABGR_8888, "ABGR8888"},
   {ALLEGRO_PIXEL_FORMAT_XBGR_8888, "XBGR8888"},
   {ALLEGRO_PIXEL_FORMAT_BGR_888, "BGR888"},
   {ALLEGRO_PIXEL_FORMAT_BGR_565, "BGR565"},
   {ALLEGRO_PIXEL_FORMAT_BGR_555, "BGR555"},
   {ALLEGRO_PIXEL_FORMAT_RGBX_8888, "RGBX8888"},
   {ALLEGRO_PIXEL_FORMAT_XRGB_8888, "XRGB8888"},
};

#define NUM_FORMATS  (sizeof(formats) / sizeof(formats[0]))


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;
   ALLEGRO_BITMAP *bitmap1;
   ALLEGRO_BITMAP *bitmap2;
   ALLEGRO_EVENT event;
   int i, j;
   int delta1, delta2;

   al_init();
   al_iio_init();
   al_font_init();

   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   printf("Display format = %d\n", al_get_display_format());

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());

   font = al_font_load_font("data/bmpfont.tga", NULL);
   if (!font) {
      TRACE("Failed to load bmpfont.tga\n");
      return 1;
   }
   
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   i = j = 0;
   delta1 = delta2 = 0;

   while (true) {
      i = (i + delta1 + NUM_FORMATS) % NUM_FORMATS;
      j = (j + delta2 + NUM_FORMATS) % NUM_FORMATS;

      al_set_new_bitmap_format(formats[i].format);

      bitmap1 = al_iio_load("data/allegro.pcx");
      if (!bitmap1) {
         TRACE("Could not load image, bitmap format = %d\n",
            formats[i].format);
         printf("Could not load image, bitmap format = %d\n",
            formats[i].format);
      }
      delta1 = 0;

      al_set_new_bitmap_format(formats[j].format);

      bitmap2 = al_create_bitmap(320, 200);
      if (!bitmap2) {
         TRACE("Could not create bitmap, format = %d\n", formats[j].format);
         printf("Could not create bitmap, format = %d\n", formats[j].format);
      }
      delta2 = 0;

      al_clear(al_map_rgb(0x80, 0x80, 0x80));

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 128, 80));
      al_font_textprintf_right(font, al_get_display_width()-1, 0,
         "%d %d", i, j);
      al_font_textprintf_right(font, al_get_display_width()-1,
         al_font_text_height(font), "(%d %d)",
         bitmap1 ? al_get_bitmap_format(bitmap1) : 0,
         bitmap2 ? al_get_bitmap_format(bitmap2) : 0);
      
      al_font_textprintf(font, 0, 208,
         "%s->%s", formats[i].name, formats[j].name);

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));
      
      if (bitmap1 && bitmap2) {
         al_set_target_bitmap(bitmap2);
         al_draw_bitmap(bitmap1, 0, 0, 0);
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(bitmap2, 0, 0, 0);
      }
      else {
         al_draw_line(0, 0, 320, 200, al_map_rgb_f(1, 0, 0));
         al_draw_line(0, 200, 320, 0, al_map_rgb_f(1, 0, 0));
      }

      al_flip_display();
      al_destroy_bitmap(bitmap1);
      al_destroy_bitmap(bitmap2);

      while (true) {
         al_wait_for_event(queue, &event);
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               goto Quit;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE ||
                  event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
               delta1 = +1;
               break;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_BACKSPACE ||
                  event.keyboard.keycode == ALLEGRO_KEY_LEFT) {
               delta1 = -1;
               break;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
               delta2 = +1;
               break;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
               delta2 = -1;
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
