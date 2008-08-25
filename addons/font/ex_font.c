#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;
    ALLEGRO_FONT *f;

    al_init();
    al_iio_init();
    al_font_init();

    display = al_create_display(320, 200);
    bitmap = al_iio_load("mysha.pcx");
    al_draw_bitmap(bitmap, 0, 0, 0);

    f = al_font_load_font("bmpfont.tga", NULL);

    /* Draw red text */
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(255, 0, 0));
    al_font_textout(f, "red", 10, 10);

    /* Draw green text */
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(0, 255, 0));
    al_font_textout(f, "green", 10, 50);

    al_flip_display();
    al_rest(3);
    return 0;
}
END_OF_MAIN()

