#include <allegro.h>

int main(void)
{
	ALLEGRO_DISPLAY *display;
	ALLEGRO_BITMAP *cursor;
	ALLEGRO_COLOR black;
	ALLEGRO_EVENT_QUEUE *queue;
	ALLEGRO_EVENT event;
	int mx = 0;
	int my = 0;

	al_init();

	al_install_mouse();
	al_install_keyboard();

	display = al_create_display(640, 480);

	al_hide_mouse_cursor();

	cursor = al_load_bitmap("cursor.tga");

	al_map_rgb(&black, 0, 0, 0);

	queue = al_create_event_queue();
	al_register_event_source(queue, (void *)al_get_mouse());
	al_register_event_source(queue, (void *)al_get_keyboard());

	do {
		if (!al_event_queue_is_empty(queue)) {
			while (al_get_next_event(queue, &event)) {
				switch (event.type) {
					case ALLEGRO_EVENT_MOUSE_AXES:
						mx = event.mouse.x;
						my = event.mouse.y;
						break;
					case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
						goto done;
						break;
					case ALLEGRO_EVENT_KEY_DOWN:
						if (event.keyboard.keycode ==
								ALLEGRO_KEY_ESCAPE)
							goto done;
						break;
				}
			}
		}
		al_clear(&black);
		al_draw_bitmap(cursor, mx, my, 0);
		al_flip_display();
		al_rest(5);
	}
        while(1);
done:
	al_destroy_event_queue(queue);

	return 0;
}
END_OF_MAIN()

