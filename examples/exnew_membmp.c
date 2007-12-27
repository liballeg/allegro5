#include <stdio.h>

#include "allegro5.h"
#include "a5_font.h"

static ALLEGRO_KBDSTATE kbdstate;
static A5FONT_FONT *myfont;

static bool key_down(void)
{
	al_get_keyboard_state(&kbdstate);

	int i;

	for (i = 0; i < ALLEGRO_KEY_MAX; i++) {
		if (al_key_down(&kbdstate, i)) {
			return true;
		}
	}

	return false;
}

static void print(char *message, int x, int y)
{
	ALLEGRO_COLOR color;

	al_map_rgb(&color, 0, 0, 0);
	al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &color);
	a5font_textout(myfont, message, x+2, y+2);

	al_map_rgb(&color, 255, 255, 255);
	al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &color);
	a5font_textout(myfont, message, x, y);
}

static void test(ALLEGRO_BITMAP *bitmap, char *message)
{
	long frames = 0;
	long start_time = al_current_time();
	int fps = 0;
	ALLEGRO_COLOR color;

	for (;;) {
		if (key_down()) {
			while (key_down())
				;
			break;
		}

		al_map_rgb(&color, 255, 255, 255);
		al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, &color);

		al_draw_scaled_bitmap(bitmap, 0, 0,
			al_get_bitmap_width(bitmap),
			al_get_bitmap_height(bitmap),
			0, 0,
			al_get_display_width(),
			al_get_display_height(),
			0);

		print(message, 0, 0);
		char second_line[100];
		sprintf(second_line, "%d FPS", fps);
		print(second_line, 0, a5font_text_height(myfont)+5);

		al_flip_display();

		frames++;
		long now = al_current_time();
		long seconds = (now - start_time) / 1000;
		fps = seconds > 0 ? frames / seconds : 0;
	}
}

int main(void)
{
	al_init();

	al_install_keyboard();

	ALLEGRO_DISPLAY *display = al_create_display(640, 400);

	myfont = a5font_load_font("font.tga", 0);

	ALLEGRO_BITMAP *accelbmp = al_load_bitmap("mysha.pcx");

	al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
	ALLEGRO_BITMAP *membmp = al_load_bitmap("mysha.pcx");

	test(membmp, "Memory bitmap");

	test(accelbmp, "Accelerated bitmap");
}
END_OF_MAIN()

