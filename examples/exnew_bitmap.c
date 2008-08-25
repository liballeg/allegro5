#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"

int main(int argc, const char *argv[])
{
    const char *filename;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;

    if (argc > 1) {
       filename = argv[1];
    }
    else {
       filename = "mysha.pcx";
    }

    al_init();
    al_install_mouse();

    al_iio_init();

    display = al_create_display(320, 200);
    if (!display) {
       TRACE("Error creating display");
       return 1;
    }

    al_show_mouse_cursor();
    bitmap = al_iio_load(filename);
    if (!bitmap) {
       TRACE("mysha.pcx not found");
       return 1;
    }

    al_draw_bitmap(bitmap, 0, 0, 0);
    al_flip_display();
    al_rest(5.0);

    return 0;
}
END_OF_MAIN()
