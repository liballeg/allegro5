#include "allegro.h"
#include "a5_font.h"

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
    a5font_textout(f, "text", 10, 10);

    al_flip_display();
    al_rest(3000);
    return 0;
}
END_OF_MAIN()
