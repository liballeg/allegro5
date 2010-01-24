#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"

#include "common.c"

int main(int argc, const char *argv[])
{
    const char *filename;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *membitmap, *bitmap;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT_QUEUE *queue;
    bool redraw = true;
    double zoom = 1;
    double t0;
    double t1;

    if (argc > 1) {
       filename = argv[1];
    }
    else {
       filename = "data/mysha.pcx";
    }

    if (!al_init()) {
        abort_example("Could not init Allegro.\n");
        return 1;
    }

    al_install_mouse();
    al_install_keyboard();

    al_init_image_addon();

    display = al_create_display(640, 480);
    if (!display) {
       abort_example("Error creating display\n");
    }
    
    al_set_window_title(filename);
    
    /* We load the bitmap into a memory bitmap, because creating a
     * display bitmap could fail if the bitmap is too big to fit into a
     * single texture.
     * FIXME: Or should A5 automatically created multiple display bitmaps?
     */
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    t0 = al_current_time();
    membitmap = al_load_bitmap(filename);
    t1 = al_current_time();
    if (!membitmap) {
       abort_example("%s not found or failed to load\n", filename);
    }
    al_set_new_bitmap_flags(0);

    printf("Loading took %.4f seconds\n", t1 - t0);
    
    // FIXME: 
    // Now try to split the memory bitmap into display bitmaps?
    bitmap = al_clone_bitmap(membitmap);
    if (!bitmap)
        bitmap = membitmap;

    timer = al_install_timer(1.0 / 30);
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    while (1) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;
        if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                break;
            if (event.keyboard.unichar == '1')
                zoom = 1;
            if (event.keyboard.unichar == '+')
                zoom *= 1.1;
            if (event.keyboard.unichar == '-')
                zoom /= 1.1;
            if (event.keyboard.unichar == 'f')
                zoom = (double)al_get_display_width() /
                    al_get_bitmap_width(bitmap);
        }
        if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;
            
        if (redraw && al_event_queue_is_empty(queue)) {
            redraw = false;
            al_clear_to_color(al_map_rgb_f(0, 0, 0));
            if (zoom == 1)
                al_draw_bitmap(bitmap, 0, 0, 0);
            else
                al_draw_rotated_scaled_bitmap(
                    bitmap, 0, 0, 0, 0, zoom, zoom, 0, 0);
            al_flip_display();
        }
    }

    al_destroy_bitmap(bitmap);

    return 0;
}

/* vim: set sts=4 sw=4 et: */
