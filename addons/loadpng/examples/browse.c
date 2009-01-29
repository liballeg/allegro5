/* browse.c
 *
 * This is not really an example but a program to help me test loadpng
 * on a large set of images.  It's not really well written.
 *
 * Usage: browse [-bpp] *.png
 *
 * Keys:
 *	Escape - exit
 *	Space - next image
 *	Backspace - previous image
 *	Plus - increase screen gamma
 *	Minus - decrease screen gamma
 *
 * Peter Wang <tjaden@users.sourceforge.net>
 */

#include <allegro.h>
#include "loadpng.h"

static void
checkerboard(BITMAP *b)
{
    AL_CONST int depth = bitmap_color_depth(b);
    int c1, c2, c;
    int x, y;

    c1 = makecol_depth(depth, 0x80, 0x80, 0x80);
    c2 = makecol_depth(depth, 0xe0, 0xe0, 0xf0);

    for (y = 0; y < b->h; y += 16) {
	c = (y % 32) ? c1 : c2;
	for (x = 0; x < b->w; x += 16) {
	    rectfill(b, x, y, x+15, y+15, c);
	    c = (c == c1) ? c2 : c1;
	}
    }
}

static void
load_and_blit(AL_CONST char *filename)
{
    BITMAP *dbuf;
    BITMAP *bmp;
    PALETTE pal;
    int depth;
    RGB_MAP rgb_table;

    bmp = load_png(filename, pal);
    if (!bmp) {
        textprintf_ex(screen , font , 0 , SCREEN_H/2 , makecol(255,0,0) , -1 , "Error loading %s" , filename);
        return;
    }

    depth = bitmap_color_depth(bmp);

    dbuf = create_bitmap_ex(depth, SCREEN_W, SCREEN_H);

    if (depth == 8) {
	set_palette(pal);
    } else if (bitmap_color_depth(screen) == 8) {
	AL_CONST signed char rsvd[256] = {0,};
	generate_optimized_palette(bmp, pal, rsvd);
	create_rgb_table(&rgb_table, pal, NULL);
	rgb_map = &rgb_table;
	set_palette(pal);
    }

    checkerboard(dbuf);

    if (depth == 32) {
	set_alpha_blender();
	draw_trans_sprite(dbuf, bmp, 100, 100);
    } else {
#if 0
	blit(bmp, dbuf, 0, 0, 0, 0, bmp->w, bmp->h);
#else
	draw_sprite(dbuf, bmp, 100, 100);
#endif
    }

    acquire_screen();
    blit(dbuf, screen, 0, 0, 0, 0, dbuf->w, dbuf->h);
    textprintf_ex(screen , font , 0 , SCREEN_H - 8 , makecol(255,255,255) , makecol(0,0,0) , 
                    "%s (%dx%dx%d) gamma=%f" , filename , bmp->w , bmp->h , depth , _png_screen_gamma);
    release_screen();

    destroy_bitmap(bmp);
    destroy_bitmap(dbuf);
}

static int eqf(double x, double y)
{
    AL_CONST double epsilon = 0.01;
    return (x > y-epsilon) && (x < y+epsilon);
}

static void
run(int nfiles, AL_CONST char *filenames[])
{
    int n = 0;
    int k;

  Redraw:

    load_and_blit(filenames[n]);

    while ((k = readkey())) {
	switch (k>>8) {

	    case KEY_ESC:
		return;

	    case KEY_SPACE:
		if (n < nfiles-1) {
		    n++;
		    goto Redraw;
		}
		break;

	    case KEY_BACKSPACE:
		if (n > 0) {
		    n--;
		    goto Redraw;
		}
		break;

	    case KEY_EQUALS:
		if (eqf(_png_screen_gamma, -1.0))
		    _png_screen_gamma = 0.0;
		else if (eqf(_png_screen_gamma, 0.0))
		    _png_screen_gamma = 0.1;
		else
		    _png_screen_gamma += 0.1;
		goto Redraw;

	    case KEY_MINUS:
		if (eqf(_png_screen_gamma, -1.0))
		    ;
		else if (eqf(_png_screen_gamma, 0.0))
		    _png_screen_gamma = -1.0;
		else
		    _png_screen_gamma -= 0.1;
		goto Redraw;

	    default:
		break;
	}
    }
}

int
main(int argc, AL_CONST char *argv[])
{
    int nfiles;
    AL_CONST char **files;
    int depth;

    if (argc < 2) {
	allegro_message("Need filename arguments\n");
	return 1;
    }

    nfiles = argc-1;
    files = argv+1;

    if (argc >= 2 && argv[1][0] == '-') {
	depth = atoi(&argv[1][1]);
	nfiles--;
	files++;
    } else {
	depth = 32;
    }

    allegro_init();
    install_keyboard();
    set_color_depth(depth);
    if (set_gfx_mode(GFX_AUTODETECT, 800, 600, 0, 0) != 0) {
	allegro_message("Error setting video mode\n");
	return 1;
    }
    set_color_conversion(COLORCONV_NONE);

    run(nfiles, files);

    allegro_exit();
    return 0;
}

END_OF_MAIN()
