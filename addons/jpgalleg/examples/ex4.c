/*
 *	Example program for the JPGalleg library
 *
 *  Version 2.6, by Angelo Mottola, 2000-2006
 *
 *	This example is the same as ex3, but also shows how you can specify
 *      custom quality and sampling settings when saving the JPG image.
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
	bmp = load_bitmap("cat.tga", NULL);
	if (!bmp) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Error loading cat.tga\n");
		return -1;
	}
	blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
	
	readkey();
	
	if (save_jpg_ex("savedcat.jpg", bmp, NULL, 65, JPG_SAMPLING_411 | JPG_OPTIMIZE, NULL)) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		allegro_message("Error saving savedcat.jpg (error code %d)\n", jpgalleg_error);
		return -1;
	}
	destroy_bitmap(bmp);
	
	return 0;
}
END_OF_MAIN();
