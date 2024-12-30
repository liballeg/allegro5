#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"
#include <math.h>

#include "common.c"

int main(int argc, char **argv)
{
    const char *filename;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *buffer, *bitmap, *subbitmap, *buffer_subbitmap;
    ALLEGRO_BITMAP *overlay;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_TRANSFORM transform;
    bool software = false;
    bool redraw = false;
    bool blend = false;
    bool use_subbitmap = true;
    int w, h;
    ALLEGRO_FONT* font;
    ALLEGRO_FONT* soft_font;

    if (argc > 1) {
       filename = argv[1];
    }
    else {
       filename = "data/mysha.pcx";
    }

    if (!al_init()) {
        abort_example("Could not init Allegro.\n");
    }

    al_init_primitives_addon();
    al_install_mouse();
    al_install_keyboard();

    al_init_image_addon();
    al_init_font_addon();
    init_platform_specific();

    display = al_create_display(640, 480);
    if (!display) {
       abort_example("Error creating display\n");
    }

    subbitmap = al_create_sub_bitmap(al_get_backbuffer(display), 50, 50, 640 - 50, 480 - 50);
    overlay = al_create_sub_bitmap(al_get_backbuffer(display), 100, 100, 300, 50);

    al_set_window_title(display, filename);

    bitmap = al_load_bitmap(filename);
    if (!bitmap) {
       abort_example("%s not found or failed to load\n", filename);
    }
    font = al_load_font("data/bmpfont.tga", 0, 0);
    if (!font) {
       abort_example("data/bmpfont.tga not found or failed to load\n");
    }

    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    buffer = al_create_bitmap(640, 480);
    buffer_subbitmap = al_create_sub_bitmap(buffer, 50, 50, 640 - 50, 480 - 50);

    soft_font = al_load_font("data/bmpfont.tga", 0, 0);
    if (!soft_font) {
       abort_example("data/bmpfont.tga not found or failed to load\n");
    }

    timer = al_create_timer(1.0 / 60);
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    w = al_get_bitmap_width(bitmap);
    h = al_get_bitmap_height(bitmap);

    al_set_target_bitmap(overlay);
    al_identity_transform(&transform);
    al_rotate_transform(&transform, -0.06);
    al_use_transform(&transform);

    while (1) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;
        if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_S) {
               software = !software;
               if (software) {
                  /* Restore identity transform on display bitmap. */
                  ALLEGRO_TRANSFORM identity;
                  al_identity_transform(&identity);
                  al_use_transform(&identity);
               }
            } else if (event.keyboard.keycode == ALLEGRO_KEY_L) {
               blend = !blend;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_B) {
               use_subbitmap = !use_subbitmap;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               break;
            }
        }
        if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;

        if (redraw && al_is_event_queue_empty(queue)) {
            double t = 3.0 + al_get_time();
            ALLEGRO_COLOR tint;
            redraw = false;

            al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
            if(blend)
               tint = al_map_rgba_f(0.5, 0.5, 0.5, 0.5);
            else
               tint = al_map_rgba_f(1, 1, 1, 1);

            if(software) {
               if(use_subbitmap) {
                  al_set_target_bitmap(buffer);
                  al_clear_to_color(al_map_rgb_f(1, 0, 0));
                  al_set_target_bitmap(buffer_subbitmap);
               } else {
                  al_set_target_bitmap(buffer);
               }
            } else {
               if(use_subbitmap) {
                  al_set_target_backbuffer(display);
                  al_clear_to_color(al_map_rgb_f(1, 0, 0));
                  al_set_target_bitmap(subbitmap);
               } else {
                  al_set_target_backbuffer(display);
               }
            }

            /* Set the transformation on the target bitmap. */
            al_identity_transform(&transform);
            al_translate_transform(&transform, -640 / 2, -480 / 2);
            al_scale_transform(&transform, 0.15 + sin(t / 5), 0.15 + cos(t / 5));
            al_rotate_transform(&transform, t / 50);
            al_translate_transform(&transform, 640 / 2, 480 / 2);
            al_use_transform(&transform);

            /* Draw some stuff */
            al_clear_to_color(al_map_rgb_f(0, 0, 0));
            al_draw_tinted_bitmap(bitmap, tint, 0, 0, 0);
            al_draw_tinted_scaled_bitmap(bitmap, tint, w / 4, h / 4, w / 2, h / 2, w, 0, w / 2, h / 4, 0);//ALLEGRO_FLIP_HORIZONTAL);
            al_draw_tinted_bitmap_region(bitmap, tint, w / 4, h / 4, w / 2, h / 2, 0, h, ALLEGRO_FLIP_VERTICAL);
            al_draw_tinted_scaled_rotated_bitmap(bitmap, tint, w / 2, h / 2, w + w / 2, h + h / 2, 0.7, 0.7, 0.3, 0);
            al_draw_pixel(w + w / 2, h + h / 2, al_map_rgb_f(0, 1, 0));
            al_put_pixel(w + w / 2 + 2, h + h / 2 + 2, al_map_rgb_f(0, 1, 1));
            al_draw_circle(w, h, 50, al_map_rgb_f(1, 0.5, 0), 3);

            al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
            if(software) {
               al_draw_text(soft_font, al_map_rgba_f(1, 1, 1, 1),
                  640 / 2, 430, ALLEGRO_ALIGN_CENTRE, "Software Rendering");
               al_set_target_backbuffer(display);
               al_draw_bitmap(buffer, 0, 0, 0);
            } else {
               al_draw_text(font, al_map_rgba_f(1, 1, 1, 1),
                  640 / 2, 430, ALLEGRO_ALIGN_CENTRE, "Hardware Rendering");
            }

            /* Each target bitmap has its own transformation matrix, so this
             * overlay is unaffected by the transformations set earlier.
             */
            al_set_target_bitmap(overlay);
            al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
            al_draw_text(font, al_map_rgba_f(1, 1, 0, 1),
               0, 10, ALLEGRO_ALIGN_LEFT, "hello!");

            al_set_target_backbuffer(display);
            al_flip_display();
        }
    }

    al_destroy_bitmap(bitmap);

    return 0;
}

/* vim: set sts=4 sw=4 et: */
