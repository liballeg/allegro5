/* alpha.c - alpha transparency test for loadpng
 *
 * Peter Wang <tjaden@users.sf.net>
 */

#include <png.h>
#include <allegro.h>
#include "loadpng.h"


static void toilet(BITMAP *b)
{
    int c1, c2, c;
    int x, y;

    c1 = makecol(0x00, 0x00, 0xe0);
    c2 = makecol(0xe0, 0xe0, 0xf0);

    for (y = 0; y < b->h; y += 16) {
	c = (y % 32) ? c1 : c2;
	for (x = 0; x < b->w; x += 16) {
	    rectfill(b, x, y, x+15, y+15, c);
	    c = (c == c1) ? c2 : c1;
	}
    }
}


int main(int argc, char *argv[])
{
    BITMAP *fg;
    int depth = 16;
    const char *file;

    allegro_init();
    install_keyboard();

    switch (argc) {
	case 1:
	    file = "alpha.png";
	    break;
	case 2:
	    file = argv[1];
	    break;
	default:
	    file = argv[1];
	    depth = atoi(argv[2]);
	    break;
    }

    if (depth == 8) {
	allegro_message("Truecolour modes only.\n");
	return 1;
    }

    set_color_depth(depth);
    if ((set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) < 0) &&
        (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) < 0)) {
	allegro_message("Unable to set video mode (640x480x%d).\n", depth);
	return 1;
    }

    /* We can't lose the alpha channel when we load the image. */
    set_color_conversion(COLORCONV_NONE);

    fg = load_png(file, NULL);
    if (!fg) {
	set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	allegro_message("Unable to load alpha.png\n");
	return 1;
    }

    acquire_screen();

    toilet(screen);

    /* Enable alpha transparency, then draw onto the screen. */
    set_alpha_blender();
    draw_trans_sprite(screen, fg, (SCREEN_W - fg->w) / 2, (SCREEN_H - fg->h) / 2);

    release_screen();

    /* Ooh, ahh. */
    readkey();

    destroy_bitmap(fg);

    return 0;
}

END_OF_MAIN()
