#include <stdio.h>

#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"


static void print(ALLEGRO_FONT *myfont, char *message, int x, int y)
{
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(0, 0, 0));
   al_font_textout(myfont, message, x+2, y+2);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(255, 255, 255));
   al_font_textout(myfont, message, x, y);
}

static void test(ALLEGRO_BITMAP *bitmap, ALLEGRO_FONT *font, char *message)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   double start_time;
   long frames = 0;
   double fps = 0;
   char second_line[100];

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());

   start_time = al_current_time();

   for (;;) {
      if (al_get_next_event(queue, &event)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
            event.keyboard.keycode == ALLEGRO_KEY_SPACE)
         {
            break;
         }
      }

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO,
         al_map_rgb(255, 255, 255));

      /* Clear the backbuffer with red so we can tell if the bitmap does not
       * cover the entire backbuffer.
       */
      al_clear(al_map_rgb(255, 0, 0));

      al_draw_scaled_bitmap(bitmap, 0, 0,
         al_get_bitmap_width(bitmap),
         al_get_bitmap_height(bitmap),
         0, 0,
         al_get_display_width(),
         al_get_display_height(),
         0);

      print(font, message, 0, 0);
      sprintf(second_line, "%.1f FPS", fps);
      print(font, second_line, 0, al_font_text_height(font)+5);

      al_flip_display();

      frames++;
      fps = (double)frames / (al_current_time() - start_time);
   }

   al_destroy_event_queue(queue);
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *accelfont;
   ALLEGRO_FONT *memfont;
   ALLEGRO_BITMAP *accelbmp;
   ALLEGRO_BITMAP *membmp;

   al_init();
   al_install_keyboard();
   al_font_init();

   display = al_create_display(640, 400);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   accelfont = al_font_load_font("font.tga", 0);
   if (!accelfont) {
      TRACE("font.tga not found\n");
      return 1;
   }
   accelbmp = al_iio_load("mysha.pcx");
   if (!accelbmp) {
      TRACE("mysha.pcx not found\n");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   memfont = al_font_load_font("font.tga", 0);
   membmp = al_iio_load("mysha.pcx");

   test(membmp, memfont, "Memory bitmap (press SPACE key)");

   test(accelbmp, accelfont, "Accelerated bitmap (press SPACE key)");

   return 0;
}
END_OF_MAIN()
