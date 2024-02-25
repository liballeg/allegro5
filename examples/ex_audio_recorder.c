/*
 *    Example program for the Allegro library.
 *
 *    Demonstrate 'simple' audio interface.
 */

#define ALLEGRO_UNSTABLE
#include <math.h>
#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

int main(int argc, const char* argv[])
{
    ALLEGRO_DISPLAY* display = NULL;
    ALLEGRO_EVENT_QUEUE* event_queue;
    ALLEGRO_EVENT event;
    ALLEGRO_SAMPLE_ID sample_id;
    ALLEGRO_TIMER* timer;
    ALLEGRO_FONT* font;

    if (!al_init()) {
        abort_example("Could not init Allegro.\n");
    }
    if (!al_init_font_addon()) {
        abort_example("Could not init the font addon.\n");
    }

    open_log();
    al_install_keyboard();

    display = al_create_display(640, 480);
    if (!display) {
        abort_example("Could not create display\n");
    }
    font = al_create_builtin_font();


    al_init_acodec_addon();
    al_init_primitives_addon();

    if (!al_install_audio()) {
        abort_example("Could not init sound!\n");
    }

    int sample_count = 640;
    int freq = 44100;

    ALLEGRO_AUDIO_RECORDER* recorder = al_create_audio_recorder(1, sample_count, freq,
        ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);
    if (!recorder) {
        abort_example("Count not open recorder\n");
    }

    timer = al_create_timer(1 / 60.0);
    al_start_timer(timer);
    event_queue = al_create_event_queue();
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_audio_recorder_event_source(recorder));

    if (!al_start_audio_recorder(recorder)) {
        abort_example("Count not start recorder\n");
    }

    uint8_t* fragment = 0;
    int window_w = 640;
    int window_h = 480;

    while (true) {
        al_wait_for_event(event_queue, &event);
        if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                break;
            }
        }
        else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            window_w = event.display.width;
            window_h = event.display.height;
        }
        else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }
        else if (event.type == ALLEGRO_EVENT_AUDIO_RECORDER_FRAGMENT) {
            ALLEGRO_AUDIO_RECORDER_EVENT* ev = al_get_audio_recorder_event(&event);
            fragment = (uint8_t*)ev->buffer;
        }
        else if (event.type == ALLEGRO_EVENT_TIMER) {

            al_clear_to_color(al_map_rgb_f(0., 0., 0.));
            if (fragment != NULL) {
                int dx = 0;
                int dy = window_h / 2;

                float bar_width = window_w / sample_count;
                for (int i = 0; i < sample_count; i++) {
                    int value_cursor = i;

                    char value = (UINT8_MAX / 2) - fragment[value_cursor];
                    float bx = dx + i * bar_width;
                    al_draw_line(bx, dy, bx, dy + value, al_map_rgb_f(200, 0., 0.), bar_width);
                }
            }

            al_flip_display();
        }
    }

    al_stop_audio_recorder(recorder);
    al_destroy_audio_recorder(recorder);
    al_uninstall_audio();

    al_destroy_display(display);
    close_log(true);
    return 0;
}

/* vim: set sts=3 sw=3 et: */
