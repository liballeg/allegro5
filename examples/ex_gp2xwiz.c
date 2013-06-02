#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>

#include "common.c"

const int W = 320, H = 240;
const int R = 240;
const int POINTS = 200;

int main(void)
{
	if (!al_init()) {
		abort_example("Could not init Allegro.\n");
	}
	al_init_primitives_addon();

	ALLEGRO_DISPLAY *d = al_create_display(W, H);
	if (!d) {
		abort_example("Error creating display\n");
	}

	ALLEGRO_VERTEX v[POINTS];
	ALLEGRO_COLOR c;

	v[0].x = 0;
	v[0].y = 0;
	c = al_map_rgb(rand()%256, rand()%256, rand()%256);
	v[0].color = al_get_prim_color(c);
	v[1].x = 0+R;
	v[1].y = 0;
	c = al_map_rgb(rand()%256, rand()%256, rand()%256);
	v[1].color = al_get_prim_color(c);

	float a = 0;
	float r = R;
	int i;

	for (i = 2; i < POINTS; i++) {
		v[i].x = 0+cos(a)*r;
		v[i].y = 0+sin(a)*r;
		a += 0.3f;
		r -= 1.5f;
		c = al_map_rgb(rand()%256, rand()%256, rand()%256);
		v[i].color = al_get_prim_color(c);
	}

	int frames = 0;
	ALLEGRO_TRANSFORM t;

	while (true) {
		al_clear_to_color(al_map_rgb(0, 0, 0));
		al_identity_transform(&t);
		al_rotate_transform(&t, frames*0.1f);
		al_translate_transform(&t, W/2, H/2);
		al_use_transform(&t);
		al_draw_prim(v, NULL, 0, POINTS, ALLEGRO_PRIM_TRIANGLE_FAN);
		al_flip_display();
		/* GP2X Wiz is locked to 60FPS using OpenGL */
		frames++;
		if (frames > 400)
			break;
	}

	al_uninstall_system();

	return 0;
}
