#include "allegro5/allegro5.h"

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;
    ALLEGRO_LOCKED_REGION locked;
    int i;
    al_init();
    al_set_new_display_format(ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA);
    display = al_create_display(100, 100);
    bitmap = al_get_backbuffer();
    al_lock_bitmap(bitmap, &locked, 0);
    for (i = 0; i < 100 * locked.pitch; i++)
        ((char *)locked.data)[i] = (i / locked.pitch) * 255 / 99;
    al_unlock_bitmap(bitmap);
    al_flip_display();
    al_rest(5.0);
    return 0;
}
END_OF_MAIN()
