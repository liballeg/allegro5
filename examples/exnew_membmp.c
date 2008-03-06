#include <stdio.h>

#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"


static bool space_key_down(void)
{
   ALLEGRO_KBDSTATE kbdstate;

   al_get_keyboard_state(&kbdstate);

   if (al_key_down(&kbdstate, ALLEGRO_KEY_SPACE)) {
      return true;
   }

   return false;
}

static void print(A5FONT_FONT *myfont, char *message, int x, int y)
{
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(0, 0, 0));
   a5font_textout(myfont, message, x+2, y+2);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(255, 255, 255));
   a5font_textout(myfont, message, x, y);
}

static void test(ALLEGRO_BITMAP *bitmap, A5FONT_FONT *font, char *message)
{
   double start_time;
   long frames = 0;
   double fps = 0;
   char second_line[100];

   start_time = al_current_time();

   for (;;) {
      if (space_key_down()) {
         while (space_key_down())
            ;
         break;
      }

      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO,
         al_map_rgb(255, 255, 255));

      al_draw_scaled_bitmap(bitmap, 0, 0,
         al_get_bitmap_width(bitmap),
         al_get_bitmap_height(bitmap),
         0, 0,
         al_get_display_width(),
         al_get_display_height(),
         0);

      print(font, message, 0, 0);
      sprintf(second_line, "%.1f FPS", fps);
      print(font, second_line, 0, a5font_text_height(font)+5);

      al_flip_display();

      frames++;
      fps = (double)frames / (al_current_time() - start_time);
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   A5FONT_FONT *accelfont;
   A5FONT_FONT *memfont;
   ALLEGRO_BITMAP *accelbmp;
   ALLEGRO_BITMAP *membmp;

   al_init();
   al_install_keyboard();

   display = al_create_display(640, 400);
   if (!display) {
      allegro_message("Error creating display");
      return 1;
   }

   accelfont = a5font_load_font("font.tga", 0);
   if (!accelfont) {
      allegro_message("font.tga not found");
      return 1;
   }
   accelbmp = al_load_bitmap("mysha.pcx");
   if (!accelbmp) {
      allegro_message("mysha.pcx not found");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   memfont = a5font_load_font("font.tga", 0);
   membmp = al_load_bitmap("mysha.pcx");

   test(membmp, memfont, "Memory bitmap (press SPACE key)");

   test(accelbmp, accelfont, "Accelerated bitmap (press SPACE key)");

   return 0;
}
END_OF_MAIN()
