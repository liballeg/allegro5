#include "allegro5/allegro.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

static void redraw(void)
{
    ALLEGRO_COLOR black, white;
    int w, h;

    white = al_map_rgba_f(1, 1, 1, 1);
    black = al_map_rgba_f(0, 0, 0, 1);

    al_clear_to_color(white);
    w = al_get_bitmap_width(al_get_target_bitmap());
    h = al_get_bitmap_height(al_get_target_bitmap());
    al_draw_line(0, h, w / 2, 0, black, 0);
    al_draw_line(w / 2, 0, w, h, black, 0);
    al_draw_line(w / 4, h / 2, 3 * w / 4, h / 2, black, 0);
    al_flip_display();
}

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_EVENT_QUEUE *events;
    ALLEGRO_EVENT event;

    double last_resize;
    int rs = 100;

    /* Initialize Allegro and create an event queue. */
    if (!al_init()) {
        abort_example("Could not init Allegro.\n");
        return 1;
    }
    al_init_primitives_addon();
    events = al_create_event_queue();

    /* Setup a display driver and register events from it. */
    al_set_new_display_flags(ALLEGRO_RESIZABLE);
    display = al_create_display(rs, rs);
    if (!display) {
        abort_example("Could not create display.\n");
        return 1;
    }
    al_register_event_source(events, al_get_display_event_source(display));

    /* Setup a keyboard driver and regsiter events from it. */
    al_install_keyboard();
    al_register_event_source(events, al_get_keyboard_event_source());

    /* Display a pulsating window until a key or the closebutton is pressed. */
    redraw();
    last_resize = 0;
    while (true) {
        if (al_get_next_event(events, &event)) {
            if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
                ALLEGRO_DISPLAY_EVENT *de = &event.display;
                al_acknowledge_resize(de->source);
                redraw();
            }
            if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                break;
            }
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                break;
            }
        }
        if (al_get_time() - last_resize > 0.1) {
            int s;

            last_resize = al_get_time();
            rs += 10;
            if (rs == 300)
               rs = 100;
            s = rs;
            if (s > 200)
               s = 400 - s;
            al_resize_display(display, s, s);
        }
    }

    return 0;
}

/* vim: set sts=4 sw=4 et: */
