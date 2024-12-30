#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"

#include "common.c"

static void print(ALLEGRO_FONT *myfont, char *message, int x, int y)
{
   al_draw_text(myfont, al_map_rgb(0, 0, 0), x+2, y+2, 0, message);
   al_draw_text(myfont, al_map_rgb(255, 255, 255), x, y, 0, message);
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
   al_register_event_source(queue, al_get_keyboard_event_source());

   start_time = al_get_time();

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

      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      /* Clear the backbuffer with red so we can tell if the bitmap does not
       * cover the entire backbuffer.
       */
      al_clear_to_color(al_map_rgb(255, 0, 0));

      al_draw_scaled_bitmap(bitmap, 0, 0,
         al_get_bitmap_width(bitmap),
         al_get_bitmap_height(bitmap),
         0, 0,
         al_get_bitmap_width(al_get_target_bitmap()),
         al_get_bitmap_height(al_get_target_bitmap()),
         0);

      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

      /* Note this makes the memory buffer case much slower due to repeated
       * locking of the backbuffer.  Officially you can't use al_lock_bitmap
       * to solve the problem either.
       */
      print(font, message, 0, 0);
      sprintf(second_line, "%.1f FPS", fps);
      print(font, second_line, 0, al_get_font_line_height(font)+5);

      al_flip_display();

      frames++;
      fps = (double)frames / (al_get_time() - start_time);
   }

   al_destroy_event_queue(queue);

   return quit;
}

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *accelfont;
   ALLEGRO_FONT *memfont;
   ALLEGRO_BITMAP *accelbmp;
   ALLEGRO_BITMAP *membmp;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();
   al_init_image_addon();
   al_init_font_addon();
   init_platform_specific();

   display = al_create_display(640, 400);
   if (!display) {
      abort_example("Error creating display\n");
   }

   accelfont = al_load_font("data/font.tga", 0, 0);
   if (!accelfont) {
      abort_example("font.tga not found\n");
   }
   accelbmp = al_load_bitmap("data/mysha.pcx");
   if (!accelbmp) {
      abort_example("mysha.pcx not found\n");
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   memfont = al_load_font("data/font.tga", 0, 0);
   membmp = al_load_bitmap("data/mysha.pcx");

   for (;;) {
      if (test(membmp, memfont, "Memory bitmap (press SPACE key)"))
         break;
      if (test(accelbmp, accelfont, "Accelerated bitmap (press SPACE key)"))
         break;
   }

   return 0;
}
