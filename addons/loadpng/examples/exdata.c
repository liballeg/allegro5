/* exdata.c - loading a PNG from a datafile (exdata.dat)
 *
 * Ceniza & Peter Wang
 */

#include <png.h>
#include <allegro.h>
#include "loadpng.h"

#define DAT_PNG  DAT_ID('P','N','G',' ')

int main(int argc, char *argv[])
{
    BITMAP *bmp;
    DATAFILE *data;
    int depth = 16;

    allegro_init();
    install_keyboard();

    /* Tell Allegro that datafile items with the "PNG" type id should
     * be treated as PNG files.
     */
    register_png_datafile_object(DAT_PNG);

    if (argc > 1)
	depth = atoi(argv[1]);

    if (depth == 8) {
	allegro_message("Sorry, this program doesn't work in 8 bpp modes,\n"
			"because we don't have a palette for our image.\n");
	return 1;
    }

    set_color_depth(depth);
    if ((set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) < 0) &&
        (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) < 0)) {
	allegro_message("Unable to set video mode (640x480x%d).\n", depth);
	return 1;
    }
    clear_bitmap(screen);

    data = load_datafile("exdata.dat");
    if (!data) {
	allegro_message("Unable to load exdata.dat\n");
	return 1;
    }

    /* Once loaded into memory, PNG datafile objects are just bitmaps. */
    bmp = (BITMAP *)data[0].dat;
    draw_sprite(screen, bmp, (SCREEN_W - bmp->w) / 2, (SCREEN_H - bmp->h) / 2);

    readkey();

    unload_datafile(data);

    return 0;
}

END_OF_MAIN()
