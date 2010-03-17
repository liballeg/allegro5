#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include <math.h>

#include "common.c"

int main(int argc, const char *argv[])
{
    const char *filename;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *buffer, *bitmap;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_TRANSFORM transform;
    ALLEGRO_TRANSFORM identity;
    bool software = false;
    bool redraw = false;
    int w, h;
    ALLEGRO_FONT* font;

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
    al_init_font_addon();

    display = al_create_display(640, 480);
    if (!display) {
       abort_example("Error creating display\n");
    }
    
    al_set_window_title(filename);

    bitmap = al_load_bitmap(filename);
    if (!bitmap) {
       abort_example("%s not found or failed to load\n", filename);
    }
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    buffer = al_create_bitmap(640, 480);
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
    
    font = al_load_font("data/bmpfont.tga", 0, 0);
    if (!font) {
       abort_example("data/bmpfont.tga not found or failed to load\n");
    }

    timer = al_install_timer(1.0 / 60);
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);
    
    w = al_get_bitmap_width(bitmap);
    h = al_get_bitmap_height(bitmap);
    
    al_identity_transform(&identity);

    while (1) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;
        if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                software = !software;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               break;
            }
        }
        if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;
            
        if (redraw && al_event_queue_is_empty(queue)) {
            double t = al_current_time();
            redraw = false;
            
            al_identity_transform(&transform);
            al_translate_transform(&transform, -640 / 2, -480 / 2);
            al_scale_transform(&transform, 0.15 + sin(t / 5), 0.15 + cos(t / 5));
            al_rotate_transform(&transform, t / 50);
            al_translate_transform(&transform, 640 / 2, 480 / 2);
            
            al_use_transform(&transform);
            
            if(software)
               al_set_target_bitmap(buffer);

            al_clear_to_color(al_map_rgb_f(0, 0, 0));
            al_draw_bitmap(bitmap, 0, 0, 0);
            al_draw_scaled_bitmap(bitmap, w / 4, h / 4, w / 2, h / 2, w, 0, w / 2, h / 4, 0);//ALLEGRO_FLIP_HORIZONTAL);
            al_draw_bitmap_region(bitmap, w / 4, h / 4, w / 2, h / 2, 0, h, ALLEGRO_FLIP_VERTICAL);
            al_draw_rotated_scaled_bitmap(bitmap, w / 2, h / 2, w + w / 2, h + h / 2, 0.7, 0.7, 0.3, 0);
            al_draw_pixel(w + w / 2, h + h / 2, al_map_rgb_f(0, 1, 0));
                    
            if(software) {
               al_draw_text(font, 640 / 2, 430, ALLEGRO_ALIGN_CENTRE, "Software Rendering");
               al_use_transform(&identity);
               al_set_target_bitmap(al_get_backbuffer());
               al_draw_bitmap(buffer, 0, 0, 0);
               al_use_transform(&transform);
            } else {
               al_draw_text(font, 640 / 2, 430, ALLEGRO_ALIGN_CENTRE, "Hardware Rendering");
            }
            al_flip_display();
        }
    }

    al_destroy_bitmap(bitmap);

    return 0;
}

/* vim: set sts=4 sw=4 et: */
