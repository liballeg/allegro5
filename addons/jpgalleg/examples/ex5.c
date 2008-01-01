/*
 *	Example program for the JPGalleg library
 *
 *  Version 2.6, by Angelo Mottola, 2000-2006
 *
 *	This example demonstrates how to load/save JPGs in memory and how this
 *      could be used to make a preview of a JPG image for a given quality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include <jpgalleg.h>


#define ESTIMATED_MAX_JPG_SIZE(bmp)		(((bmp)->w * (bmp)->h * 3) + 1024)


static int sx = 0, sy = 0, old_sx = 0, old_sy = 0;
static char last_path[1024] = "";
static char quality_string[64] = "Quality: 75 (medium)";
static char size_string[64] = "Size: ?";
static int quality_cb(void *dp3, int d2);
static int quality_text_proc(int msg, DIALOG *d, int c);
static int preview_proc(int msg, DIALOG *d, int c);
static int load_proc(void);
static int save_proc(void);
static int mmx_proc(void);


#define QUALITY_TEXT		2
#define QUALITY_SLIDER		3
#define SIZE_TEXT		4
#define SS_444_RADIO		5
#define SS_422_RADIO		6
#define SS_411_RADIO		7
#define GREYSCALE_CHECK		8
#define OPTIMIZE_CHECK		9
#define PREVIEW			11


static DIALOG settings_dialog[] = {
/* proc               x    y    w    h    fg   bg   key  flags      d1   d2    dp    dp2   dp3 */
 { d_shadow_box_proc, 0,   0,   465, 190, 0,   0,   0,   0,         0,   0,    NULL, NULL, NULL },
 { d_text_proc,       98,  10,  96,  8,   0,   0,   0,   0,         0,   0,    "JPG settings", NULL, NULL },
 { quality_text_proc, 16,  30,  260, 8,   0,   0,   0,   0,         75,  0,    quality_string, NULL, NULL },
 { d_slider_proc,     16,  40,  260, 16,  0,   0,   0,   0,         99,  74,   NULL, quality_cb, NULL },
 { d_text_proc,       16,  58,  260, 8,   0,   0,   0,   0,         0,   0,    size_string, NULL, NULL },
 { d_radio_proc,      16,  80,  260, 11,  0,   0,   0,   D_SELECTED,0,   0,    "No subsampling (444)", NULL, NULL },
 { d_radio_proc,      16,  95,  260, 11,  0,   0,   0,   0,         0,   0,    "Horizontal subsampling (422)", NULL, NULL },
 { d_radio_proc,      16,  110, 260, 11,  0,   0,   0,   0,         0,   0,    "Hor./Ver. subsampling (411)", NULL, NULL },
 { d_check_proc,      16,  135, 100, 11,  0,   0,   0,   0,         1,   0,    " Greyscale", NULL, NULL },
 { d_check_proc,      188, 135, 100, 11,  0,   0,   0,   0,         1,   0,    " Optimize", NULL, NULL },
 { d_button_proc,     106, 160, 81,  17,  0,   0,   0,   D_EXIT,    0,   0,    "Save", NULL, NULL },
 { preview_proc,      290, 15,  160, 160, 0,   0,   0,   0,         0,   0,    NULL, NULL, NULL },
 { d_yield_proc,      0,   0,   0,   0,   0,   0,   0,   0,         0,   0,    NULL, NULL, NULL },
 { NULL,              0,   0,   0,   0,   0,   0,   0,   0,         0,   0,    NULL, NULL, NULL }
};

static MENU popup_menu[] = {
/* text           proc           child  flags        dp */
 { "Load image",  load_proc,     NULL,  0,           NULL },
 { "Save as JPG", save_proc,     NULL,  D_DISABLED,  NULL },
 { "Enable MMX",  mmx_proc,      NULL,  D_DISABLED,  NULL },
 { "Quit",        NULL,          NULL,  0,           NULL },
 { NULL,          NULL,          NULL,  0,           NULL }
};



static int
quality_text_proc(int msg, DIALOG *d, int c)
{
	if (msg == MSG_DRAW) {
		rectfill(screen, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1, gui_bg_color);
		sprintf(quality_string, "Quality: %d (%s)", d->d1,
			d->d1 < 20 ? "very low" :
			(d->d1 < 50 ? "low" :
			(d->d1 < 80 ? "medium" :
			(d->d1 < 95 ? "high" : "very high"))));
	}
	return d_text_proc(msg, d, c);
}


static int
quality_cb(void *dp3, int d2)
{
	(void)dp3;

	settings_dialog[QUALITY_TEXT].d1 = d2 + 1;
	object_message(&settings_dialog[QUALITY_TEXT], MSG_DRAW, 0);
	return D_O_K;
}


static int
preview_proc(int msg, DIALOG *d, int c)
{
	BITMAP *bmp;
	static int quality = 75, flags = JPG_SAMPLING_444;
	int new_quality, new_flags = 0;
	int size;

	(void)c;

	new_quality = settings_dialog[QUALITY_SLIDER].d2 + 1;
	if (settings_dialog[SS_444_RADIO].flags & D_SELECTED)
		new_flags = JPG_SAMPLING_444;
	if (settings_dialog[SS_422_RADIO].flags & D_SELECTED)
		new_flags = JPG_SAMPLING_422;
	if (settings_dialog[SS_411_RADIO].flags & D_SELECTED)
		new_flags = JPG_SAMPLING_411;
	if (settings_dialog[GREYSCALE_CHECK].flags & D_SELECTED)
		new_flags |= JPG_GREYSCALE;
	if (settings_dialog[OPTIMIZE_CHECK].flags & D_SELECTED)
		new_flags |= JPG_OPTIMIZE;
	
	switch (msg) {
		case MSG_START:
			d->dp2 = (void *)create_bitmap(d->w, d->h);
			bmp = (BITMAP *)d->dp;
			d->dp3 = malloc(ESTIMATED_MAX_JPG_SIZE(bmp));
			clear_to_color((BITMAP *)d->dp2, gui_mg_color);
			d->d2 = 20;
			sprintf(size_string, "Size: ?");
			break;

		case MSG_END:
			destroy_bitmap((BITMAP *)d->dp2);
			free(d->dp3);
			break;

		case MSG_DRAW:
			clear_to_color((BITMAP *)d->dp2, gui_mg_color);
			bmp = (BITMAP *)d->dp;
			bmp = create_sub_bitmap(bmp, MAX(0, (bmp->w - d->w) / 2), MAX(0, (bmp->h - d->h) / 2), d->w, d->h);
			if (!bmp)
				goto preview_error;
			size = ESTIMATED_MAX_JPG_SIZE(bmp);
			if (save_memory_jpg_ex(d->dp3, &size, bmp, NULL, quality, flags, NULL))
				goto preview_error;
			size = ESTIMATED_MAX_JPG_SIZE(bmp);
			destroy_bitmap(bmp);
			bmp = load_memory_jpg(d->dp3, size, NULL);
			if (!bmp)
				goto preview_error;
			blit(bmp, (BITMAP *)d->dp2, 0, 0, 0, 0, bmp->w, bmp->h);
			rect(screen, d->x - 1, d->y - 1, d->x + d->w, d->y + d->h, gui_fg_color);
			blit((BITMAP *)d->dp2, screen, 0, 0, d->x, d->y, d->w, d->h);
			destroy_bitmap(bmp);
			object_message(&settings_dialog[QUALITY_TEXT], MSG_DRAW, 0);
			break;

		case MSG_IDLE:
			if (d->d2 > 0) {
				d->d2--;
				if (d->d2 == 0) {
					quality = new_quality;
					flags = new_flags;
					bmp = (BITMAP *)d->dp;
					size = ESTIMATED_MAX_JPG_SIZE(bmp);
					if (save_memory_jpg_ex(d->dp3, &size, bmp, NULL, quality, flags, NULL))
						goto preview_error;
					sprintf(size_string, "Size: %dx%d, %.1f Kbytes  ", bmp->w, bmp->h, (double)size / 1024.0);
					object_message(&settings_dialog[SIZE_TEXT], MSG_DRAW, 0);
					return D_REDRAWME;
				}
			}
			if ((new_quality != quality) || (new_flags != flags)) {
				quality = new_quality;
				flags = new_flags;
				d->d2 = 20;
				if (flags & JPG_GREYSCALE) {
					settings_dialog[SS_444_RADIO].flags |= D_DISABLED;
					settings_dialog[SS_422_RADIO].flags |= D_DISABLED;
					settings_dialog[SS_411_RADIO].flags |= D_DISABLED;
				}
				else {
					settings_dialog[SS_444_RADIO].flags &= ~D_DISABLED;
					settings_dialog[SS_422_RADIO].flags &= ~D_DISABLED;
					settings_dialog[SS_411_RADIO].flags &= ~D_DISABLED;
				}
				settings_dialog[SS_444_RADIO].fg = (flags & JPG_GREYSCALE ? gui_mg_color : gui_fg_color);
				settings_dialog[SS_422_RADIO].fg = (flags & JPG_GREYSCALE ? gui_mg_color : gui_fg_color);
				settings_dialog[SS_411_RADIO].fg = (flags & JPG_GREYSCALE ? gui_mg_color : gui_fg_color);
				object_message(&settings_dialog[SS_444_RADIO], MSG_DRAW, 0);
				object_message(&settings_dialog[SS_422_RADIO], MSG_DRAW, 0);
				object_message(&settings_dialog[SS_411_RADIO], MSG_DRAW, 0);
				return D_REDRAWME;
			}
			break;
	}
	return D_O_K;

preview_error:
	line((BITMAP *)d->dp2, 0, 0, d->w - 1, d->h - 1, 0);
	line((BITMAP *)d->dp2, d->w - 1, 0, 0, d->h - 1, 0);
	blit((BITMAP *)d->dp2, screen, 0, 0, d->x, d->y, d->w, d->h);
	return D_O_K;
}


static void
progress_cb(int percentage)
{
	rect(screen, 219, 235, 421, 244, 0);
	rectfill(screen, 220, 236, 220 + (percentage * 2), 243, makecol(255, 0, 0));
	rectfill(screen, 221 + (percentage * 2), 236, 421, 243, 0);
}


static int
load_proc(void)
{
	char buffer[256], *p;
	BITMAP *bmp;
	PALETTE palette;

	if (file_select_ex("Load image", last_path, "bmp;pcx;lbm;tga;jpg;jpeg", 1024, (SCREEN_W * 2) / 3, (SCREEN_H * 2) / 3)) {
		bmp = load_jpg_ex(last_path, palette, progress_cb);
		if (!bmp)
			bmp = load_bitmap(last_path, palette);
		settings_dialog[PREVIEW].dp = bmp;
		if (!bmp) {
			sprintf(buffer, "Error loading image (JPG error code: %d)", jpgalleg_error);
			alert(buffer, jpgalleg_error_string(), "", "Ok", NULL, 0, 0);
			return 0;
		}
		for (p = last_path + strlen(last_path); (p >= last_path) && (*p != '/') && (*p != '\\'); p--)
			;
		*(p + 1) = '\0';
		sx = sy = old_sx = old_sy = 0;
		set_palette(palette);
		popup_menu[1].flags = 0;
	}
	return 0;
}


static int
save_proc(void)
{
	char filename[1024];
	char buffer[256], *p;
	BITMAP *bmp = NULL;
	int quality = 0, flags = 0;

	if (file_select_ex("Save as JPG", last_path, "jpg", 1024, (SCREEN_W * 2) / 3, (SCREEN_H * 2) / 3)) {
		strcpy(filename, last_path);
		for (p = last_path + strlen(last_path); (p >= last_path) && (*p != '/') && (*p != '\\'); p--)
			;
		*(p + 1) = '\0';
		set_dialog_color(settings_dialog, gui_fg_color, gui_bg_color);
		centre_dialog(settings_dialog);
		if (popup_dialog(settings_dialog, -1) < 0)
			return 0;
		quality = settings_dialog[QUALITY_SLIDER].d2 + 1;
		if (settings_dialog[SS_444_RADIO].flags & D_SELECTED)
			flags = JPG_SAMPLING_444;
		if (settings_dialog[SS_422_RADIO].flags & D_SELECTED)
			flags = JPG_SAMPLING_422;
		if (settings_dialog[SS_411_RADIO].flags & D_SELECTED)
			flags = JPG_SAMPLING_411;
		if (settings_dialog[GREYSCALE_CHECK].flags & D_SELECTED)
			flags |= JPG_GREYSCALE;
		if (settings_dialog[OPTIMIZE_CHECK].flags & D_SELECTED)
			flags |= JPG_OPTIMIZE;
		bmp = settings_dialog[PREVIEW].dp;

		if (save_jpg_ex(filename, bmp, NULL, quality, flags, progress_cb)) {
			sprintf(buffer, "Error saving JPG image (error code %d)", jpgalleg_error);
			alert(buffer, jpgalleg_error_string(), "", "Ok", NULL, 0, 0);
		}
	}
	return 0;
}


static int
mmx_proc(void)
{
	if (popup_menu[2].flags & D_SELECTED) {
		popup_menu[2].flags &= ~D_SELECTED;
		cpu_capabilities &= ~CPU_MMX;
	}
	else {
		popup_menu[2].flags |= D_SELECTED;
		cpu_capabilities |= CPU_MMX;
	}
	return 0;
}


static void
draw_image(BITMAP *bmp, int x, int y)
{
	scare_mouse();
	if (bmp) {
		blit(bmp, screen, x, y, 0, 0, bmp->w, bmp->h);
		x = bmp->w - x;
		y = bmp->h - y;
		if (x < SCREEN_W)
			rectfill(screen, x, 0, SCREEN_W, SCREEN_H, gui_mg_color);
		if (y < SCREEN_H)
			rectfill(screen, 0, y, x, SCREEN_H, gui_mg_color);
	}
	else
		clear_to_color(screen, gui_mg_color);
	unscare_mouse();
}


int
main(int argc, char **argv)
{
	BITMAP *bmp = NULL;
	int x, y, mx, my;
	int result, mode = GFX_AUTODETECT;

	allegro_init();
	install_keyboard();

	if (install_mouse() < 0) {
		allegro_message("This example requires a mouse to run!");
		return -1;
	}

	jpgalleg_init();

	if (cpu_capabilities & CPU_MMX)
		popup_menu[2].flags = D_SELECTED;

	if ((argc > 1) && (!strcmp(argv[1], "-window")))
		mode = GFX_AUTODETECT_WINDOWED;

	set_color_depth(32);
	if (set_gfx_mode(mode, 640, 480, 0, 0)) {
		set_color_depth(16);
		if (set_gfx_mode(mode, 640, 480, 0, 0)) {
			set_color_depth(15);
			if (set_gfx_mode(mode, 640, 480, 0, 0)) {
				allegro_message("Unable to init truecolor 640x480 gfx mode: %s", allegro_error);
				return -1;
			}
		}
	}

	clear_to_color(screen, gui_mg_color);
	show_mouse(screen);
	
	alert("JPG image load/save example for " JPGALLEG_VERSION_STRING, NULL, "Press right mouse button for contextual menu, ESC to quit.", "Ok", NULL, 0, 0);

	while (!key[KEY_ESC]) {
		x = mouse_x;
		y = mouse_y;
		get_mouse_mickeys(&mx, &my);
		mx /= 2;
		my /= 2;

		bmp = settings_dialog[PREVIEW].dp;

		if (bmp) {
			old_sx = sx;
			old_sy = sy;
			if (bmp->w > SCREEN_W) {
				if ((x == 0) && (mx < 0) && (sx > 0))
					sx = MAX(0, sx + mx);
				if ((x == SCREEN_W - 1) && (mx > 0) && (sx < bmp->w - SCREEN_W - 1))
					sx = MIN(sx + mx, bmp->w - SCREEN_W - 1);
				if (key[KEY_LEFT] && (sx > 0))
					sx = MAX(0, sx - 8);
				if (key[KEY_RIGHT] && (sx < bmp->w - SCREEN_W - 1))
					sx = MIN(sx + 8, bmp->w - SCREEN_W - 1);
			}
			if (bmp->h > SCREEN_H) {
				if ((y == 0) && (my < 0) && (sy > 0))
					sy = MAX(0, sy + my);
				if ((y == SCREEN_H - 1) && (my > 0) && (sy < bmp->h - SCREEN_H - 1))
					sy = MIN(sy + my, bmp->h - SCREEN_H - 1);
				if (key[KEY_UP] && (sy > 0))
					sy = MAX(0, sy - 8);
				if (key[KEY_DOWN] && (sy < bmp->h - SCREEN_H - 1))
					sy = MIN(sy + 8, bmp->h - SCREEN_H - 1);
			}
			if ((sx != old_sx) || (sy != old_sy))
				draw_image(bmp, sx, sy);
		}
		if (mouse_b & 0x2) {
			result = do_menu(popup_menu, x, y);
			while(key[KEY_ESC])
				;
			if (result == 3)
				break;
			draw_image((BITMAP *)settings_dialog[PREVIEW].dp, sx, sy);
		}
	}

	if (bmp)
		destroy_bitmap(bmp);

	return 0;
}
END_OF_MAIN();
