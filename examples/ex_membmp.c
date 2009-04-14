#include <stdio.h>

#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"


static void print(ALLEGRO_FONT *myfont, char *message, int x, int y)
{
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(0, 0, 0));
   al_draw_text(myfont, x+2, y+2, 0, message);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(255, 255, 255));
   al_draw_text(myfont, x, y, 0, message);
}

static bool test(ALLEGRO_BITMAP *bitmap, ALLEGRO_FONT *font, char *message)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   double start_time;
   long frames = 0;
   double fps = 0;
   char second_line[100];
   bool quit = false;

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());

   start_time = al_current_time();

   for (;;) {
      if (al_get_next_event(queue, &event)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
               break;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               quit = true;
               break;
            }
         }
      }

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO,
         al_map_rgb(255, 255, 255));

      /* Clear the backbuffer with red so we can tell if the bitmap does not
       * cover the entire backbuffer.
       */
      al_clear_to_color(al_map_rgb(255, 0, 0));

      al_draw_scaled_bitmap(bitmap, 0, 0,
         al_get_bitmap_width(bitmap),
         al_get_bitmap_height(bitmap),
         0, 0,
         al_get_display_width(),
         al_get_display_height(),
         0);

      print(font, message, 0, 0);
      sprintf(second_line, "%.1f FPS", fps);
      print(font, second_line, 0, al_get_font_line_height(font)+5);

      al_flip_display();

      frames++;
      fps = (double)frames / (al_current_time() - start_time);
   }

   al_destroy_event_queue(queue);

   return quit;
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *accelfont;
   ALLEGRO_FONT *memfont;
   ALLEGRO_BITMAP *accelbmp;
   ALLEGRO_BITMAP *membmp;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_init_font_addon();

   display = al_create_display(640, 400);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   accelfont = al_load_font("data/font.tga", 0, 0);
   if (!accelfont) {
      TRACE("font.tga not found\n");
      return 1;
   }
   accelbmp = al_load_image("data/mysha.pcx");
   if (!accelbmp) {
      TRACE("mysha.pcx not found\n");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   memfont = al_load_font("data/font.tga", 0, 0);
   membmp = al_load_image("data/mysha.pcx");

   for (;;) {
      if (test(membmp, memfont, "Memory bitmap (press SPACE key)"))
         break;
      if (test(accelbmp, accelfont, "Accelerated bitmap (press SPACE key)"))
         break;
   }

   return 0;
}
END_OF_MAIN()
