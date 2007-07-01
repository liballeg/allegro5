#include "allegro.h"

void redraw(AL_BITMAP *picture)
{
   AL_COLOR color;
   al_map_rgb(al_get_backbuffer(), &color, rand()%255, rand()%255, rand()%255);
   al_clear(&color);
   al_map_rgb(al_get_backbuffer(), &color, 255, 0, 0);
   al_draw_line(0, 0, 640, 480, &color, 0);
   al_draw_line(0, 480, 640, 0, &color, 0);
   al_draw_bitmap(picture, 0, 0, 0);
   al_flip_display();
}

int main(void)
{
   AL_DISPLAY *display;
   AL_BITMAP *picture;

   al_init();

   al_set_new_display_flags(AL_FULLSCREEN);
   display = al_create_display(640, 480);

   picture = al_load_bitmap("mysha.pcx");

   redraw(picture);
   al_rest(2000);
   al_resize_display(800, 600);
   redraw(picture);
   al_rest(2000);

   return 0;
}
END_OF_MAIN()

