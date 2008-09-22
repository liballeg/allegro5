#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"

int main(int argc, const char *argv[])
{
    const char *filename;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT_QUEUE *queue;
    bool redraw = true;

    if (argc > 1) {
       filename = argv[1];
    }
    else {
       filename = "data/mysha.pcx";
    }

    if (!al_init()) {
        TRACE("Could not init Allegro.\n");
        return 1;
    }

    al_install_mouse();
    al_install_keyboard();

    al_iio_init();

    display = al_create_display(320, 200);
    if (!display) {
       TRACE("Error creating display\n");
       return 1;
    }
    
    al_set_window_title(filename);

    bitmap = al_iio_load(filename);
    if (!bitmap) {
       TRACE("%s not found or failed to load", filename);
       return 1;
    }
    
    timer = al_install_timer(1.0 / 30);
    queue = al_create_event_queue();
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)timer);
    al_start_timer(timer);

    while (1) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;
        if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
                event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            break;
        }
        if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;
            
        if (redraw && al_event_queue_is_empty(queue)) {
            redraw = false;
            al_draw_bitmap(bitmap, 0, 0, 0);
            al_flip_display();
        }
    }

    return 0;
}
END_OF_MAIN()

/* vim: set sts=4 sw=4 et: */
