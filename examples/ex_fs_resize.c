#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"

void redraw(ALLEGRO_BITMAP *picture)
{
   ALLEGRO_COLOR color;
   int w = al_get_display_width();
   int h = al_get_display_height();

   color = al_map_rgb(rand() % 255, rand() % 255, rand() % 255);
   al_clear(color);

   color = al_map_rgb(255, 0, 0);
   al_draw_line(0, 0, w, h, color);
   al_draw_line(0, h, w, 0, color);

   al_draw_bitmap(picture, 0, 0, 0);
   al_flip_display();
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *picture;

   al_init();
   al_iio_init();

   al_set_new_display_flags(ALLEGRO_FULLSCREEN);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   picture = al_iio_load("data/mysha.pcx");
   if (!picture) {
      TRACE("mysha.pcx not found\n");
      return 1;
   }

   redraw(picture);
   al_rest(2.5);

   if (al_resize_display(800, 600)) {
      redraw(picture);
      al_rest(2.5);
   }
   else {
      return 1;
   }

   return 0;
}
END_OF_MAIN()

