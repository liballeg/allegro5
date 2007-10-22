#include "allegro.h"

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;
    al_init();
    display = al_create_display(320, 200);
    bitmap = al_load_bitmap("mysha.pcx");
    ALLEGRO_COLOR color;
    al_map_rgb(&color, 255, 255, 255);
    al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, &color);
    al_draw_rotated_bitmap(bitmap, 160, 100, 160, 100, 45, 0);
    al_draw_rotated_bitmap(bitmap, 160, 100, 160, 100, 45, 0);
    al_flip_display();
    al_rest(1000);
    return 0;
}
END_OF_MAIN()
