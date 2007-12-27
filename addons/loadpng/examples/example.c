/* example.c - annotated example program for loadpng
 *
 * Peter Wang <tjaden@users.sf.net>
 */

#include <png.h>
#include <allegro.h>
#include "loadpng.h"


int main(int argc, char *argv[])
{
    char *filename;
    BITMAP *bmp;
    PALETTE pal;
    int depth = 16;

    /* Initialise Allegro. */
    allegro_init();
    install_keyboard();

    /* Get filename from command-line. */
    if (argc < 2) {
	allegro_message("usage: %s filename.png [depth]\n", argv[0]);
	return 1;
    }

    filename = argv[1];
    if (argc >= 3)
	depth = atoi(argv[2]);

    /* Make Allegro aware of PNG file format. */
    register_png_file_type();

    /* Set a suitable graphics mode. */
    set_color_depth(depth);
    if ((set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) < 0) &&
        (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) < 0)) {
	allegro_message("Error setting video mode (640x480x%d).\n", depth);
	return 1;
    }

    /* Load the PNG into a BITMAP structure. */
    bmp = load_png(filename, pal);
    if (!bmp) {
	set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	allegro_message("Error loading file `%s'.\n", filename);
	return 1;
    }

    /* If we have a palette, set it. */
    if (bitmap_color_depth(bmp) == 8)
	set_palette(pal);

    /* Show it on the screen. */
    clear(screen);
    blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);

    /* Write the image onto disk. */
    save_bitmap("saved.png", bmp, pal);

    /* Press any key to continue... */
    readkey();

    /* The End. */
    destroy_bitmap(bmp);
    return 0;
}

END_OF_MAIN()
