/*
 *	Example program for the JPGalleg library
 *
 *  Version 2.6, by Angelo Mottola, 2000-2006
 *
 *	This example shows how to display JPGs stored in a datafile.
 */

#include <allegro.h>
#include <jpgalleg.h>

#include "datafile.h"


int
main(void)
{
	DATAFILE *data;
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
	
	data = load_datafile("datafile.dat");
	if (!data) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Error loading datafile.dat\n");
		return -1;
	}
	
	clear(screen);
	bmp = (BITMAP *)data[JPGALLEG_LOGO].dat;
	blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
	readkey();
	
	unload_datafile(data);
	
	return 0;
}
END_OF_MAIN();
