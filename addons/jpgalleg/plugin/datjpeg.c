/*
 *         __   _____    ______   ______   ___    ___
 *        /\ \ /\  _ `\ /\  ___\ /\  _  \ /\_ \  /\_ \
 *        \ \ \\ \ \L\ \\ \ \__/ \ \ \L\ \\//\ \ \//\ \      __     __
 *      __ \ \ \\ \  __| \ \ \  __\ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\
 *     /\ \_\/ / \ \ \/   \ \ \L\ \\ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \
 *     \ \____//  \ \_\    \ \____/ \ \_\ \_\/\____\/\____\ \____\ \____ \
 *      \/____/    \/_/     \/___/   \/_/\/_/\/____/\/____/\/____/\/___L\ \
 *                                                                  /\____/
 *                                                                  \_/__/
 *
 *      Version 2.6, by Angelo Mottola, 2000-2006
 *
 *      See README file for instructions on using this package in your own
 *      programs.
 *
 *      Grabber plugin for managing JPEG image objects, derived from the
 *      Allegro datimage plugin.
 */

#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "datedit.h"

#include <jpgalleg.h>



static char quality_string[64] = "Quality: 75 (medium)";
static int quality_cb(void *dp3, int d2);

static DIALOG settings_dialog[] = {
/* proc               x    y    w    h    fg   bg   key  flags      d1   d2    dp    dp2   dp3 */
 { d_shadow_box_proc, 0,   0,   292, 190, 0,   0,   0,   0,         0,   0,    NULL, NULL, NULL },
 { d_text_proc,       70,  10,  260, 8,   0,   0,   0,   0,         0,   0,    "JPG import settings", NULL, NULL },
 { d_text_proc,       16,  30,  260, 8,   0,   0,   0,   0,         0,   0,    quality_string, NULL, NULL },
 { d_slider_proc,     16,  40,  260, 16,  0,   0,   0,   0,         99,  74,   NULL, quality_cb, NULL },
 { d_radio_proc,      16,  70,  260, 11,  0,   0,   0,   D_SELECTED,0,   0,    "No subsampling (444)", NULL, NULL },
 { d_radio_proc,      16,  85,  260, 11,  0,   0,   0,   0,         0,   0,    "Horizontal subsampling (422)", NULL, NULL },
 { d_radio_proc,      16,  100, 260, 11,  0,   0,   0,   0,         0,   0,    "Hor./Ver. subsampling (411)", NULL, NULL },
 { d_check_proc,      16,  127, 100, 11,  0,   0,   0,   0,         1,   0,    " Greyscale", NULL, NULL },
 { d_check_proc,      188, 127, 100, 11,  0,   0,   0,   0,         1,   0,    " Optimize", NULL, NULL },
 { d_button_proc,     106, 160, 81,  17,  0,   0,   0,   D_EXIT,    0,   0,    "Ok", NULL, NULL },
 { d_yield_proc,      0,   0,   0,   0,   0,   0,   0,   0,         0,   0,    NULL, NULL, NULL },
 { NULL,              0,   0,   0,   0,   0,   0,   0,   0,         0,   0,    NULL, NULL, NULL }
};



/* creates a new bitmap object */
static void *makenew_jpeg(long *size)
{
	BITMAP *bmp = create_bitmap_ex(8, 32, 32);
	int buffer_size = 32 * 32 * 3;
	char *buffer = (char *)_al_malloc(buffer_size);

	clear_bitmap(bmp);
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x040104
	textout_centre_ex(bmp, font, "JPG", 16, 12, 1, -1);
#else
	text_mode(-1);
	textout_centre(bmp, font, "JPG", 16, 12, 1);
#endif
	save_memory_jpg(buffer, &buffer_size, bmp, NULL);
	destroy_bitmap(bmp);

	*size = buffer_size;

	return buffer;
}



/* returns a bitmap description string */
static void get_jpeg_desc(AL_CONST DATAFILE *dat, char *s)
{
	BITMAP *bmp = (BITMAP *)load_memory_jpg(dat->dat, dat->size, NULL);

	sprintf(s, "JPG image (%dx%d truecolor, %ld KB)", bmp->w, bmp->h, dat->size / 1024);

	destroy_bitmap(bmp);
}



/* exports a bitmap into an external file */
static int export_jpeg(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
	PACKFILE *f = pack_fopen(filename, F_WRITE);

	if (f) {
		pack_fwrite(dat->dat, dat->size, f);
		pack_fclose(f);
	}

	return (errno == 0);
}



/* draws a bitmap onto the grabber object view window */
static void plot_jpeg(AL_CONST DATAFILE *dat, int x, int y)
{
	BITMAP *b = load_memory_jpg(dat->dat, dat->size, NULL);
	int w = b->w;
	int h = b->h;
	fixed scale;

	if (w > SCREEN_W-x-8) {
		scale = itofix(SCREEN_W-x-8) / w;
		w = (w * scale) >> 16;
		h = (h * scale) >> 16;
	}

	if (h > SCREEN_H-y-40) {
		scale = itofix(SCREEN_H-y-40) / h;
		w = (w * scale) >> 16;
		h = (h * scale) >> 16;
	}

	rect(screen, x, y+32, x+w+1, y+32+h+1, gui_fg_color);

	if (bitmap_color_depth(screen) == 8) {
		if ((w != b->w) || (h != b->h))
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x040104
			textout_ex(screen, font, "<reduced color scaled preview>", x, y+18, gui_fg_color, -1);
#else
			textout(screen, font, "<reduced color scaled preview>", x, y+18, gui_fg_color);
#endif
		else
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x040104
			textout_ex(screen, font, "<reduced color preview>", x, y+18, gui_fg_color, -1);
#else
			textout(screen, font, "<reduced color preview>", x, y+18, gui_fg_color);
#endif
	}
	else if ((w != b->w) || (h != b->h))
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x040104
		textout_ex(screen, font, "<scaled preview>", x, y+18, gui_fg_color, -1);
#else
		textout(screen, font, "<scaled preview>", x, y+18, gui_fg_color);
#endif

	scare_mouse();
	if ((w != b->w) || (h != b->h)) {
		if (bitmap_color_depth(b) != bitmap_color_depth(screen)) {
			PALETTE pal;
			generate_332_palette(pal);
			b = _fixup_loaded_bitmap(b, pal, bitmap_color_depth(screen));
		}
		if (b)
			stretch_blit(b, screen, 0, 0, b->w, b->h, x+1, y+33, w, h);
	}
	else
		blit(b, screen, 0, 0, x+1, y+33, b->w, b->h);
	unscare_mouse();
	destroy_bitmap(b);
}



/* handles double-clicking on a bitmap in the grabber */
static int view_jpeg(DATAFILE *dat)
{
	BITMAP *b = load_memory_jpg(dat->dat, dat->size, NULL);
	fixed scale = itofix(1);
	fixed prevscale = itofix(1);
	int x = 0;
	int y = 0;
	int prevx = 0;
	int prevy = 0;
	BITMAP *bc = NULL;
	int done = FALSE;
	int c;

	show_mouse(NULL);
	clear_to_color(screen, gui_mg_color);
	blit(b, screen, 0, 0, 0, 0, b->w, b->h);

	clear_keybuf();
	do {
		poll_mouse();
	} while (mouse_b);

	do {
		if ((x != prevx) || (y != prevy) || (scale != prevscale)) {
			prevx = itofix(SCREEN_W) / scale;
			prevy = itofix(SCREEN_H) / scale;

			if ((b->w >= prevx) && (x+prevx > b->w))
				x = b->w-prevx;
			else if ((b->w < prevx) && (x > 0))
				x = 0;

			if ((b->h >= prevy) && (y+prevy > b->h))
				y = b->h-prevy;
			else if ((b->h < prevy) && (y > 0))
				y = 0;

			if (x < 0)
				x = 0;
			if (y < 0)
				y = 0;

			if (scale != prevscale)
				clear_to_color(screen, gui_mg_color);

			if (!bc) {
				bc = create_bitmap(b->w, b->h);
				blit(b, bc, 0, 0, 0, 0, b->w, b->h);
			}

			stretch_blit(bc, screen, x, y, b->w-x, b->h-y, 0, 0, ((b->w-x)*scale)>>16, ((b->h-y)*scale)>>16);

			prevx = x;
			prevy = y;
			prevscale = scale;
		}

		while (keypressed()) {
			c = readkey();

			switch (c >> 8) {
			
				case KEY_DOWN:
					y += 4;
					break;

				case KEY_RIGHT:
					x += 4;
					break;

				case KEY_UP:
					y -= 4;
					break;

				case KEY_LEFT:
					x -= 4;
					break;

				case KEY_HOME:
					x = 0;
					y = 0;
					break;

				case KEY_END:
					x = 65536;
					y = 65536;
					break;

				case KEY_PGUP:
					if (scale > itofix(1)/16)
						scale /= 2;
					break;

				case KEY_PGDN:
					if (scale < itofix(16))
						scale *= 2;
					break;

				default:
					switch (c & 0xFF) {

						case '+':
							if (scale < itofix(16))
								scale *= 2;
							break;

						case '-':
							if (scale > itofix(1)/16)
								scale /= 2;
							break;

						default:
							done = TRUE;
							break;
					}
					break;
			}
		}

		poll_mouse();

		if (mouse_b)
			done = TRUE;

	} while (!done);

	if (bc)
		destroy_bitmap(bc);
	destroy_bitmap(b);

	clear_keybuf();

	do {
		poll_mouse();
	} while (mouse_b);

	clear_bitmap(screen);
	show_mouse(screen);
	
	return D_REDRAW;
}



/* reads a bitmap from an external file */
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x04010d
static DATAFILE *grab_jpeg(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
	int x = datedit_numprop(prop, DAT_XPOS);
	int y = datedit_numprop(prop, DAT_YPOS);
	int w = datedit_numprop(prop, DAT_XSIZ);
	int h = datedit_numprop(prop, DAT_YSIZ);
#else
static void *grab_jpeg(AL_CONST char *filename, long *size, int x, int y, int w, int h, int depth)
{
#endif
	PACKFILE *f = NULL;
	BITMAP *bmp;
	char *buffer;
	int buffer_size;
	int quality, flags = 0;

	if (depth > 0) {
		int oldcolordepth = _color_depth;

		_color_depth = depth;
		set_color_conversion(COLORCONV_TOTAL);

		bmp = load_bitmap(filename, datedit_last_read_pal);

		_color_depth = oldcolordepth;
		set_color_conversion(COLORCONV_NONE);
	}
	else
		bmp = load_bitmap(filename, datedit_last_read_pal);

	if (!bmp) {
		bmp = load_jpg(filename, datedit_last_read_pal);
		if (!bmp)
			return NULL;
		if ((x < 0) || (y < 0) || (w < 0) || (h < 0)) {
			buffer_size = file_size_ex(filename);
			buffer = (char *)_al_malloc(buffer_size);
			if (!buffer) {
				destroy_bitmap(bmp);
				return NULL;
			}
			f = pack_fopen(filename, F_READ);
			pack_fread(buffer, buffer_size, f);
			pack_fclose(f);
			destroy_bitmap(bmp);
         
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x04010d
			return datedit_construct(type, buffer, buffer_size, prop);
#else
			*size = buffer_size;
			return buffer;
#endif
		}
	}

	if ((x >= 0) && (y >= 0) && (w >= 0) && (h >= 0)) {
		BITMAP *b2 = create_bitmap_ex(bitmap_color_depth(bmp), w, h);
		clear_to_color(b2, bitmap_mask_color(b2));
		blit(bmp, b2, x, y, 0, 0, w, h);
		destroy_bitmap(bmp);
		bmp = b2;
	}
	
	buffer_size = (bmp->w * bmp->h * 3) + 1024;
	buffer = (char *)_al_malloc(buffer_size);
	if (!buffer) {
		destroy_bitmap(bmp);
		return NULL;
	}
	
	quality = settings_dialog[3].d2 + 1;
	if (settings_dialog[4].flags & D_SELECTED)
		flags = JPG_SAMPLING_444;
	if (settings_dialog[5].flags & D_SELECTED)
		flags = JPG_SAMPLING_422;
	if (settings_dialog[6].flags & D_SELECTED)
		flags = JPG_SAMPLING_411;
	if (settings_dialog[7].flags & D_SELECTED)
		flags |= JPG_GREYSCALE;
	if (settings_dialog[8].flags & D_SELECTED)
		flags |= JPG_OPTIMIZE;
	
	if (save_memory_jpg_ex(buffer, &buffer_size, bmp, datedit_last_read_pal, quality, flags, NULL)) {
		free(buffer);
		destroy_bitmap(bmp);
		return NULL;
	}
	destroy_bitmap(bmp);

#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x04010d
	return datedit_construct(type, buffer, buffer_size, prop);
#else
	*size = buffer_size;
	return buffer;
#endif
}



/* saves a bitmap into the datafile format */
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x040104
static int save_datafile_jpeg(DATAFILE *dat, AL_CONST int *fixed_prop, int packed, int packkids, int strip, int sort, int verbose, int extra, PACKFILE *f)
#else
static void save_datafile_jpeg(DATAFILE *dat, int packed, int packkids, int strip, int verbose, int extra, PACKFILE *f)
#endif
{
	pack_fwrite(dat->dat, dat->size, f);
#if ALLEGRO_VERSION*0x10000+ALLEGRO_SUB_VERSION*0x100+ALLEGRO_WIP_VERSION>=0x040104
	return TRUE;
#else
	return;
#endif
}



static int quality_cb(void *dp3, int d2)
{
	int quality = d2 + 1;
	
	sprintf(quality_string, "Quality: %d (%s)       ", quality,
		quality < 20 ? "very low" :
		(quality < 50 ? "low" :
		(quality < 80 ? "medium" :
		(quality < 95 ? "high" : "very high"))));
	object_message(&settings_dialog[2], MSG_DRAW, 0);
	return D_O_K;
}



static int jpeg_settings_proc(void)
{
	set_dialog_color(settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(settings_dialog);
	popup_dialog(settings_dialog, -1);
	return D_REDRAW;
}



static MENU jpeg_settings_menu =
{
	"JPG import settings",
	jpeg_settings_proc,
	NULL,
	0,
	0
};



/* plugin interface header */
DATEDIT_OBJECT_INFO datjpeg_info =
{
	DAT_JPEG, 
	"JPG image", 
	get_jpeg_desc,
	makenew_jpeg,
	save_datafile_jpeg,
	plot_jpeg,
	view_jpeg,
	NULL
};



DATEDIT_GRABBER_INFO datjpeg_grabber =
{
	DAT_JPEG, 
	"jpg;jpeg;bmp;lbm;pcx;tga",
	"jpg;jpeg",
	grab_jpeg,
	export_jpeg,
	NULL
};


DATEDIT_MENU_INFO datjpeg_menu =
{
	&jpeg_settings_menu,
	NULL,
	DATEDIT_MENU_FILE,
	0,
	NULL
};
