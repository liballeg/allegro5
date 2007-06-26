#include "allegro.h"

int main(void)
{
    AL_DISPLAY *display;
    AL_BITMAP *bitmap;
    AL_LOCKED_REGION locked;
    int i;
    al_init();
    display = al_create_display(100, 100);
    bitmap = al_create_bitmap(100, 100);
    al_lock_bitmap(bitmap, &locked, 0);
    for (i = 0; i < 100 * locked.pitch; i++)
        ((char *)locked.data)[i] = (i / locked.pitch) * 255 / 99;
    al_unlock_bitmap(bitmap);
    al_draw_bitmap(bitmap, 0, 0, 0);
    al_flip_display();
    al_rest(5000);
    return 0;
}
END_OF_MAIN()
