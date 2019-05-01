/* This example displays a picture on the screen, with support for
 * command-line parameters and turn on/off screensaver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#include "common.c"

int main(int argc, char **argv)
{
    const char *filename;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_FONT *font;
    bool redraw = true;
    int ss_inhibe = 0;

    /* The first commandline argument can optionally specify an 
     * image to display instead of the default. Allegro's image
     * addon supports BMP, DDS, PCX, TGA and can be compiled with
     * PNG and JPG support on all platforms. Additional formats
     * are supported by platform specific libraries and support for
     * image formats can also be added at runtime.
     */
    if (argc > 1) {
       filename = argv[1];
    }
    else {
       filename = "data/mysha.pcx";
    }

    if (!al_init()) {
       abort_example("Could not init Allegro.\n");
    }

    /* The second parameter to the process can optionally specify what 
     * adapter to use.
     */
    if (argc > 2) {
       al_set_new_display_adapter(atoi(argv[2]));
    }

    /* Allegro requires installing drivers for all input devices before
     * they can be used.
     */
    al_install_mouse();
    al_install_keyboard();

    /* Initialize the image and font addon. Requires the allegro_image
     * and allegro_font addon library.
     */
    al_init_image_addon();
    al_init_font_addon();

    // Helper functions from common.c.
    init_platform_specific();

    // Create a new display that we can render the image to.
    display = al_create_display(640, 480);
    if (!display) {
       abort_example("Error creating display\n");
    }
    
    al_set_window_title(display, filename);

    al_inhibit_screensaver(ss_inhibe);
    
    // Load the image and time how long it took for the log.
    bitmap = al_load_bitmap(filename);
    if (!bitmap) {
       abort_example("%s not found or failed to load\n", filename);
    }

    // Load a font from datas of examples.
    font = al_load_font("data/fixed_font.tga", 0, 0);
    if (!font) {
      abort_example("Error loading data/fixed_font.tga\n");
    }

    // Create a timer that fires 30 times a second.
    timer = al_create_timer(1.0 / 30); 
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source()); 
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer); // Start the timer

    // Primary 'game' loop.
    while (1) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event); // Wait for and get an event.
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;
        /* Use keyboard to zoom image in and out.
         * ' ': Set Inhibe or not Inhibe the screensaver.
         */
        if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                break; // Break the loop and quite on escape key.

            if (event.keyboard.unichar == ' '){
                ss_inhibe ^= 1;
                al_inhibit_screensaver(ss_inhibe);
            }
        }

        // Trigger a redraw on the timer event
        if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;
            
        // Redraw, but only if the event queue is empty
        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = false;
            // Clear so we don't get trippy artifacts left after zoom.
            al_clear_to_color(al_map_rgb_f(0, 0, 0));
            al_draw_bitmap(bitmap, 0, 0, 0);
            al_draw_textf(font, al_map_rgb(255, 255, 0), 0, 0, 0,
               "Inhibe screensaver: %d", ss_inhibe);
            al_flip_display();
        }
    }

    al_destroy_font(font);
    al_destroy_bitmap(bitmap);

    return 0;
}
