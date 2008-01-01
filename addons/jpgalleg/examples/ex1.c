/*
 *	Example program for the JPGalleg library
 *
 *  Version 2.6, by Angelo Mottola, 2000-2006
 *
 *	This example shows how to load JPG image files from disk.
 */

#include <allegro.h>
#include <jpgalleg.h>


int
main(void)
{
	BITMAP *bmp;

	allegro_init();
	install_keyboard();
	
	jpgalleg_init();

	set_color_depth(32);
	if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0)) {
		set_color_depth(16);
		if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0)) {
			set_color_depth(15);
			if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0)) {
				allegro_message("Unable to init truecolor 640x480 gfx mode: %s", allegro_error);
				return -1;
			}
		}
	}
	clear(screen);
	bmp = load_jpg("jpgalleg.jpg", NULL);
	if (!bmp) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Error loading jpgalleg.jpg (error code %d)\n", jpgalleg_error);
		return -1;
	}
	blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
	destroy_bitmap(bmp);
	readkey();
	
	return 0;
}
END_OF_MAIN();
