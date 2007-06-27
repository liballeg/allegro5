#include "allegro.h"

static void redraw(void)
{
    AL_COLOR black, white;
    int i, w, h;
    al_map_rgba_f(al_get_backbuffer(), &white, 1, 1, 1, 1);
    al_map_rgba_f(al_get_backbuffer(), &black, 0, 0, 0, 1);

    al_clear(&white);
    w = al_get_display_width();
    h = al_get_display_height();
    al_draw_line(0, h, w / 2, 0, &black);
    al_draw_line(w / 2, 0, w, h, &black);
    al_draw_line(w / 4, h / 2, 3 * w / 4, h / 2, &black);
    al_flip_display();
}

int main(void)
{
    AL_DISPLAY *display;
    AL_EVENT_QUEUE *events;
    AL_EVENT event;
    AL_KEYBOARD *keyboard;

    long last_resize;
    int rs = 100;

    /* Initialize Allegro and create an event queue. */
    al_init();
    events = al_create_event_queue();

    /* Setup a display driver and register events from it. */
    al_set_new_display_flags(AL_RESIZABLE);
    display = al_create_display(rs, rs);
    al_register_event_source(events, (AL_EVENT_SOURCE *)display);

    /* Setup a keyboard driver and regsiter events from it. */
    al_install_keyboard();
    keyboard = al_get_keyboard();
    al_register_event_source(events, (AL_EVENT_SOURCE *)keyboard);

    /* Display a pulsating window until a key or the closebutton is pressed. */
    redraw();
    last_resize = 0;
    while (1)
    {
        if (al_get_next_event(events, &event))
        {
            if (event.type == AL_EVENT_DISPLAY_RESIZE) {
                AL_DISPLAY_EVENT *de = &event.display;
                al_acknowledge_resize();
                redraw();
            }
            if (event.type == AL_EVENT_DISPLAY_CLOSE) {
                break;
            }
            if (event.type == AL_EVENT_KEY_DOWN) {
                break;
            }
        }
        if (al_current_time() - last_resize > 100)
        {
            int s;
            last_resize = al_current_time();
            rs += 10;
            if (rs == 300) rs = 100;
            s = rs;
            if (s > 200) s = 400 - s;
            al_resize_display(s, s);
        }
    }
    return 0;
}
