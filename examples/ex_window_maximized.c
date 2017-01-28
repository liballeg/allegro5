/*
 * Creating the maximized, resizable window.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"


#define DISPLAY_W 640
#define DISPLAY_H 480


extern int
main(int argc, char **argv)
{
	ALLEGRO_DISPLAY *display;
	ALLEGRO_EVENT_QUEUE *queue;
	bool done = false;
	bool redraw = true;

	(void)argc;
	(void)argv;

	if (!al_init()) {
		abort_example("Failed to init Allegro.\n");
	}

	if (!al_init_primitives_addon()) {
		abort_example("Failed to init primitives addon.\n");
	}

	al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE
		| ALLEGRO_MAXIMIZED);

	display = al_create_display(DISPLAY_W, DISPLAY_H);
	if (!display) {
		abort_example("Error creating display.\n");
	}

	/* this is should not be an issue */
	ALLEGRO_MONITOR_INFO mi;
	al_get_monitor_info(0, &mi);
	int max_w = mi.x2 - mi.x1;
	int max_h = mi.y2 - mi.y1;
	al_set_window_constraints(display, DISPLAY_W, DISPLAY_H, max_w, max_h);

	if (!al_install_keyboard()) {
		abort_example("Error installing keyboard.\n");
	}

	queue = al_create_event_queue();
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_display_event_source(display));


	while (!done) {
		ALLEGRO_EVENT event;

		if (redraw && al_is_event_queue_empty(queue)) {
			al_clear_to_color(al_map_rgb_f(0, 0, 0));
			int x2 = al_get_display_width(display) - 10;
			int y2 = al_get_display_height(display) - 10;
			al_draw_filled_rectangle(10, 10, x2, y2, al_map_rgb(255, 127, 66));
			al_draw_filled_circle(5, 5, 30, al_map_rgb(127, 66, 255));
			al_draw_filled_circle(x2 + 5, y2 + 5, 30, al_map_rgb(127, 66, 255));
			al_flip_display();
			redraw = false;
		}

		al_wait_for_event(queue, &event);
		switch (event.type) {
		case ALLEGRO_EVENT_KEY_DOWN:
			if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
				done = true;
			break;

		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			redraw = true;
			al_acknowledge_resize(display);
			break;

		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			done = true;
			break;
		}
	}

	return 0;
}
