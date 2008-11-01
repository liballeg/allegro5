#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include <stdio.h>

int main(void)
{
	ALLEGRO_DISPLAY *display;
	ALLEGRO_FONT *font;
	ALLEGRO_EVENT_QUEUE *events;
	ALLEGRO_EVENT event;
	bool done = false;
	bool active = true;

	al_init();
	al_install_keyboard();
	al_font_init();

	al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);

	display = al_create_display(200, 32);
	font = al_font_load_font("data/font.tga", 0);

	if (!font) {
		printf("Error loading font\n");
		return 1;
	}

	events = al_create_event_queue();
	al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
	/* For expose events */
	al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display);

	do {
		al_clear(al_map_rgb(0, 0, 0));
		al_font_textprintf(font, 0, 0, "Screen saver: %s", active ? "Normal" : "Inhibited");
		al_flip_display();
		al_wait_for_event(events, &event);
		switch (event.type) {
			case ALLEGRO_EVENT_KEY_DOWN:
				if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
					done = true;
				else if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
					active = !active;
					al_inhibit_screensaver(!active);
				}
				break;
		}
	} while (!done);

	al_font_destroy_font(font);
	al_destroy_event_queue(events);

	return 0;
}
END_OF_MAIN()

