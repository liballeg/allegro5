#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;
    al_init();
    display = al_create_display(320, 200);
    bitmap = al_load_bitmap("mysha.pcx");
    al_draw_bitmap(bitmap, 0, 0, 0);

    a5font_init();
    A5FONT_FONT *f = a5font_load_font("bmpfont.tga", NULL);

    /* Draw red text */
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(255, 0, 0));
    a5font_textout(f, "red", 10, 10);

    /* Draw green text */
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(0, 255, 0));
    a5font_textout(f, "green", 10, 50);

    al_flip_display();
    al_rest(3000);
    return 0;
}
END_OF_MAIN()
