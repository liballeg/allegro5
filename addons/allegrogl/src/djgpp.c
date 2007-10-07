/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/*----------------------------------------------------------------
 * amesa.c -- Allegro-Mesa interfacing
 *----------------------------------------------------------------
 *  This is the interface module for use AllegroGL with Mesa using the AMesa driver.
 */
#include <string.h>

#include <GL/gl.h>
#include <GL/amesa.h>

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "alleggl.h"
#include "allglint.h"
#include "glvtable.h"


static void allegro_gl_amesa_exit(BITMAP *bmp);
static void __allegro_gl_init_texture_read_format(void);

#ifdef GFX_OPENGL_FULLSCREEN
static BITMAP *allegro_gl_amesa_fullscreen_init(int w, int h, int vw, int vh, int color_depth);

GFX_DRIVER gfx_allegro_gl_fullscreen =
{
   GFX_OPENGL_FULLSCREEN,
   empty_string,
   empty_string,
   "AllegroGL Fullscreen (AMesa)",
   allegro_gl_amesa_fullscreen_init,
   allegro_gl_amesa_exit,
   NULL,
   NULL,               //_xwin_vsync,
   NULL,
   NULL, NULL, NULL,
   allegro_gl_create_video_bitmap,
   allegro_gl_destroy_video_bitmap,
   NULL, NULL,					/* No show/request video bitmaps */
   NULL, NULL,
   allegro_gl_set_mouse_sprite,
   allegro_gl_show_mouse,
   allegro_gl_hide_mouse,
   allegro_gl_move_mouse,
   NULL,
   NULL, NULL,
   NULL,                        /* No fetch_mode_list */
   0, 0,
   0,
   0, 0,
   0,
   0,
   FALSE                        /* Windowed mode */
};
#endif /* GFX_OPENGL_FULLSCREEN */


#ifdef GFX_OPENGL_WINDOWED
static BITMAP *allegro_gl_amesa_windowed_init(int w, int h, int vw, int vh, int color_depth);

GFX_DRIVER gfx_allegro_gl_windowed =
{
   GFX_OPENGL_WINDOWED,
   empty_string,
   empty_string,
   "AllegroGL Windowed (AMesa)",
   allegro_gl_amesa_windowed_init,
   allegro_gl_amesa_exit,
   NULL,
   NULL,               //_xwin_vsync,
   NULL,
   NULL, NULL, NULL,
   allegro_gl_create_video_bitmap,
   allegro_gl_destroy_video_bitmap,
   NULL, NULL,					/* No show/request video bitmaps */
   NULL, NULL,
   allegro_gl_set_mouse_sprite,
   allegro_gl_show_mouse,
   allegro_gl_hide_mouse,
   allegro_gl_move_mouse,
   NULL,
   NULL, NULL,
   NULL,                        /* No fetch_mode_list */
   0, 0,
   0,
   0, 0,
   0,
   0,
   TRUE                         /* Windowed mode */
};
#endif /* GFX_OPENGL_WINDOWED */



static int allegro_gl_amesa_create_window (int fullscreen);
static BITMAP *allegro_gl_amesa_create_screen_bitmap (GFX_DRIVER *drv, int w, int h, int depth);

struct AMESA_DATA {
    int          fullscreen;
    AMesaVisual  visual;
    AMesaBuffer  buffer;
    AMesaContext context;
} AMESA_DATA;

static struct AMESA_DATA _amesa;
static struct allegro_gl_driver allegro_gl_amesa;

static BITMAP* subscreen = NULL;                 /* Sub_bitmap of the virtual screen */
static BITMAP* saved_screen = NULL;              /* Saves screen address */

GFX_DRIVER *amesa_gfx_driver = NULL;

static GFX_VTABLE allegro_gl_generic_vtable;
static GFX_VTABLE *old_vtable;



/* allegro_gl_amesa_create_screen:
 *  Creates screen bitmap.
 */
static BITMAP *allegro_gl_amesa_create_screen(int w, int h, int vw, int vh, int depth, int fullscreen)
{
	int _keyboard_was_installed = FALSE;
	int _mouse_was_installed = FALSE;
	GFX_VTABLE vtable, *pvtable;

	pvtable = &vtable;
	
	if (keyboard_driver) {
		_keyboard_was_installed = TRUE;
		remove_keyboard();
		TRACE("* Note * amesa_create_screen: Removing Keyboard...\n");
	}
	
	if (mouse_driver) {
		_mouse_was_installed = TRUE;
		remove_mouse();
		TRACE("* Note * amesa_create_screen: Removing Mouse...\n");
	}
	
	if ((w == 0) && (h == 0)) {
		w = 640;
		h = 480;
	}

	if ((vw > w) || (vh > h)) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
		     get_config_text ("OpenGL drivers do not support virtual screens"));
		return NULL;
	}

	allegro_gl_display_info.w = w;
	allegro_gl_display_info.h = h;

   	if (allegro_gl_amesa_create_window(fullscreen)) {
		if (fullscreen) {
			ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
			          get_config_text ("Unable to switch in AMesa fullscreen"));
		}
		else {
			ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
			          get_config_text ("Unable to create AMesa window"));
		}
		return NULL;
   	}

	/* If pixel format is Allegro compatible, set up Allegro correctly. */

	if (fullscreen) {
#ifdef GFX_OPENGL_FULLSCREEN
		allegro_gl_screen = allegro_gl_amesa_create_screen_bitmap (&gfx_allegro_gl_fullscreen, w, h, allegro_gl_display_info.colour_depth);
#endif
    }
	else {
#ifdef GFX_OPENGL_WINDOWED
		allegro_gl_screen = allegro_gl_amesa_create_screen_bitmap (&gfx_allegro_gl_windowed, w, h, allegro_gl_display_info.colour_depth);
#endif
    }

	if (!allegro_gl_screen) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
		          get_config_text ("Error creating screen bitmap"));
		return NULL;
	}
	
	__allegro_gl_valid_context = TRUE;
	__allegro_gl_driver = &allegro_gl_amesa;
	LOCK_DATA(&_amesa, sizeof(AMESA_DATA));
	LOCK_DATA(__allegro_gl_driver, sizeof(struct allegro_gl_driver));

	/* save the old vtable and create a copy */
	old_vtable = allegro_gl_screen->vtable;
	memcpy(&allegro_gl_generic_vtable, allegro_gl_screen->vtable,
	       sizeof(GFX_VTABLE));
	allegro_gl_screen->vtable = &allegro_gl_generic_vtable;

	/* The generic driver does not use the glvtable (since it already uses
	 * 'pure' Allegro gfx functions. However it needs the AGL-specific
	 * draw_glyph method. 
	 * Hence this hack : we call __allegro_gl__glvtable_update_vtable with
	 * 'pvtable' as a parameter since we do not want the regular
	 * allegro_gl_screen->vtable to be crushed.
	 */
	__allegro_gl__glvtable_update_vtable(&pvtable);
	allegro_gl_screen->vtable->draw_glyph = pvtable->draw_glyph;
	memcpy(&_screen_vtable, allegro_gl_screen->vtable, sizeof(GFX_VTABLE));
	allegro_gl_screen->vtable = &_screen_vtable;
	__allegro_gl_init_screen_mode();

	/* Print out OpenGL version info */
	TRACE("\n\nOpenGL Version: %s\nVendor: %s\nRenderer: %s\n",
	      (const char*)glGetString(GL_VERSION),
	      (const char*)glGetString(GL_VENDOR),
	      (const char*)glGetString(GL_RENDERER));
	
	/* Prints out OpenGL extensions info and activates needed extensions */
	__allegro_gl_manage_extensions();
	__allegro_gl_init_texture_read_format();	

	if (_keyboard_was_installed) {
		install_keyboard();
		TRACE("* Note * amesa_create_screen: Installing Keyboard...\n");
	}

	if (_mouse_was_installed) {
		install_mouse();
		TRACE("* Note * amesa_create_screen: Installing Mouse...\n");
	}
	
	/* XXX <rohannessian> Maybe we should leave this for autodetection? */
	gfx_capabilities |= GFX_HW_CURSOR;

	allegro_gl_info.is_mesa_driver = TRUE;
	_amesa.fullscreen = fullscreen;
	return allegro_gl_screen;
}



#ifdef GFX_OPENGL_WINDOWED
/* allegro_gl_amesa_windowed_init:
 *  Creates screen bitmap for windowed driver.
 */
static BITMAP *allegro_gl_amesa_windowed_init(int w, int h, int vw, int vh, int depth)
{
	return allegro_gl_amesa_create_screen(w, h, vw, vh, depth, FALSE);
}
#endif



#ifdef GFX_OPENGL_FULLSCREEN
/* allegro_gl_amesa_fullscreen_init:
 *  Creates screen bitmap for fullscreen driver.
 */
static BITMAP *allegro_gl_amesa_fullscreen_init(int w, int h, int vw, int vh, int depth)
{
	return allegro_gl_amesa_create_screen(w, h, vw, vh, depth, TRUE);
}
#endif



/* allegro_gl_amesa_exit:
 *  Shuts down the driver (shared between windowed and full-screen)
 */
static void allegro_gl_amesa_exit(BITMAP *bmp)
{
	/* Restore the screen to its intial value */
    screen = saved_screen;
    if (subscreen)
       destroy_bitmap(subscreen);

	amesa_gfx_driver->exit(screen);
   
	AMesaMakeCurrent(_amesa.context, NULL);
	AMesaDestroyVisual(_amesa.visual);
	AMesaDestroyBuffer(_amesa.buffer);
	AMesaDestroyContext(_amesa.context);
	
	__allegro_gl_valid_context = FALSE;
}



static void amesa_choose_gfx_mode(_DRIVER_INFO *driver_info, int *w, int *h,
                                  int *colour_depth)
{
	GFX_MODE_LIST *mode_list;
	GFX_MODE *mode;
	int i;

	TRACE("* Note * amesa_choose_gfx_mode: GFX driver : %s\n",
	      ((GFX_DRIVER*)driver_info->driver)->ascii_name);

	/* Try to find a mode which resolution and color depth are higher or
	 * equal to those requested
	 */
	mode_list = get_gfx_mode_list(driver_info->id);

	if (mode_list) {
		TRACE("* Note * amesa_choose_gfx_mode: %i modes\n",
		      mode_list->num_modes);

		mode = mode_list->mode;

		for (i = 0; i < mode_list->num_modes; i++) {
			TRACE("Mode %i : %ix%i %i bpp\n", i, mode->width, mode->height,
			       mode->bpp);
			if ((mode->width >= *w) && (mode->height >= *h) &&
			    (mode->bpp >= *colour_depth)) {
				break;
			}
			if (mode->width) {
				mode++;
			}
		}
		if ((mode->width) && (mode->height) && (mode->bpp)) {
			allegro_gl_display_info.w = *w = mode->width;
			allegro_gl_display_info.h = *h = mode->height;
			allegro_gl_display_info.colour_depth = *colour_depth = mode->bpp;
		}
		TRACE("Best Mode : %ix%i %i bpp\n", *w, *h, *colour_depth);
		destroy_gfx_mode_list(mode_list);
	}
	else {
		TRACE("** Warning ** amesa_choose_gfx_mode: Can not list modes...\n"
		      "Trying %ix%i %i bpp anyway\n", *w, *h, *colour_depth);
	}
}



/* amesa_set_gfx_mode :
 * A light version of set_gfx_mode since when this function is reached we are
 * ALREADY in set_gfx_mode. No need to initialize some parameters.
 * Moreover we must choose our driver in saved_gfx_drivers and set the real
 * gfx driver (i.e. the GFX_OPENGL one) the right way.
 */
static int amesa_set_gfx_mode(int fullscreen)
{
	extern void blit_end();
	_DRIVER_INFO *driver_list;
	int tried = FALSE;
	char buf[512], tmp[64];
	int c, n;
	int card = GFX_AUTODETECT;
	int w = allegro_gl_display_info.w;
	int h = allegro_gl_display_info.h;
	int check_mode = TRUE, require_window = FALSE;

	driver_list = saved_gfx_drivers();

	if (!fullscreen)
		require_window = TRUE;

	/* try the drivers that are listed in the config file */
	for (n=-2; n<255; n++) {
		switch (n) {

			case -2:
				/* example: gfx_card_640x480x16 = */
				usprintf(buf, uconvert_ascii("gfx_card_%dx%dx%d", tmp), w, h,
				         _color_depth);
				break;

			case -1:
				/* example: gfx_card_24bpp = */
				usprintf(buf, uconvert_ascii("gfx_card_%dbpp", tmp),
				         _color_depth);
				break;

			case 0:
				/* example: gfx_card = */
				ustrcpy(buf, uconvert_ascii("gfx_card", tmp));
				break;

			default:
				/* example: gfx_card1 = */
				usprintf(buf, uconvert_ascii("gfx_card%d", tmp), n);
			break;
		}
		card = get_config_id(uconvert_ascii("graphics", tmp), buf,
		                     GFX_AUTODETECT);

		if (card != GFX_AUTODETECT) {

			for (c=0; driver_list[c].driver; c++) {
				if (driver_list[c].id == card) {
					amesa_gfx_driver = driver_list[c].driver;
					
					if (check_mode) {
						if ( ((require_window) && (!amesa_gfx_driver->windowed))
						 || ((!require_window)
						  && (amesa_gfx_driver->windowed))) {

							amesa_gfx_driver = NULL;
							continue;
						}
					}
					break;
				}
			}

			if (amesa_gfx_driver) {
				tried = TRUE;
				amesa_gfx_driver->name = amesa_gfx_driver->desc
				                = get_config_text(amesa_gfx_driver->ascii_name);
				
				amesa_choose_gfx_mode(&driver_list[c], &w, &h, &_color_depth);
				
				screen = amesa_gfx_driver->init(w, h, 0, 0, _color_depth);
				if (screen) {
					break;
				}
				else {
					amesa_gfx_driver = NULL;
				}
			}
		}
		else {
			if (n > 1) {
				break;
			}
		}
	}

	if (!tried) {
		for (c=0; driver_list[c].driver; c++) {

			if (driver_list[c].autodetect) {
				
				amesa_gfx_driver = driver_list[c].driver;
				if (check_mode) {
					if (((require_window) && (!amesa_gfx_driver->windowed)) ||
					   ((!require_window) && (amesa_gfx_driver->windowed)))
						continue;
				}

				amesa_gfx_driver->name = amesa_gfx_driver->desc
				                = get_config_text(amesa_gfx_driver->ascii_name);

				amesa_choose_gfx_mode(&driver_list[c], &w, &h, &_color_depth);

				screen = amesa_gfx_driver->init(w, h, 0, 0, _color_depth);

				if (screen) {
					break;
				}
			}
		}
	}

	if (!screen) {
		amesa_gfx_driver = NULL;
		return -1;
	}

	LOCK_DATA(amesa_gfx_driver, sizeof(GFX_DRIVER));
	_register_switch_bitmap(screen, NULL);

	return 0;
}



/* create_window:
 *  Based on Bernhard Tschirren AMesa GLUT code.
 */
static int allegro_gl_amesa_create_window (int fullscreen)
{
	if (!allegro_gl_display_info.colour_depth)
		allegro_gl_display_info.colour_depth = _color_depth;

	set_color_depth(allegro_gl_display_info.colour_depth);

	if (amesa_set_gfx_mode(fullscreen)) {
		TRACE("** ERROR ** amesa_create_window: Unable to set a gfx mode!\n");
		return 1;
	}

	_amesa.visual  = AMesaCreateVisual(allegro_gl_display_info.doublebuffered,
					allegro_gl_display_info.colour_depth,
					GL_TRUE,				/* RGBA Mode */
					allegro_gl_display_info.depth_size,
					allegro_gl_display_info.stencil_size,
					allegro_gl_display_info.accum_size.rgba.r,
					allegro_gl_display_info.accum_size.rgba.g,
					allegro_gl_display_info.accum_size.rgba.b,
					allegro_gl_display_info.accum_size.rgba.a
					);
	if (!_amesa.visual) {
		TRACE("** ERROR ** amesa_create_window: Unable to create AMesa "
		      "Visual\n");
		return 1;
	}

	_amesa.context = AMesaCreateContext(_amesa.visual, NULL);
	if (!_amesa.context) {
		TRACE("** ERROR ** amesa_create_window: Unable to create AMesa "
		      "Context\n");
		AMesaDestroyVisual(_amesa.visual);
		return 1;
	}

	if ((screen->w != allegro_gl_display_info.w)
	 || (screen->h != allegro_gl_display_info.h)) {

		subscreen = create_sub_bitmap(screen, 0, 0,
		                  allegro_gl_display_info.w, allegro_gl_display_info.h);

		_amesa.buffer = AMesaCreateBuffer(_amesa.visual, subscreen);

		TRACE("** Note ** amesa_create_window: Screen : %ix%i %i bpp\n",
		      ubscreen->w, subscreen->h, bitmap_color_depth(subscreen));
	}
	else {
		_amesa.buffer = AMesaCreateBuffer(_amesa.visual, screen);
	}

	if (!_amesa.buffer) {
		AMesaDestroyContext(_amesa.context);
		AMesaDestroyVisual(_amesa.visual);
		TRACE("** ERROR ** amesa_create_window: Unable to create AMesa "
		      "Buffer\n");
		return 1;
	}

	if (!AMesaMakeCurrent(_amesa.context, _amesa.buffer)) {
		AMesaDestroyContext(_amesa.context);
		AMesaDestroyVisual(_amesa.visual);
		AMesaDestroyBuffer(_amesa.buffer);
		TRACE("** ERROR ** amesa_create_window: Unable to make context "
		      "current\n");
		return 1;
	}

	saved_screen = screen;
	return 0;
}



static BITMAP *allegro_gl_amesa_create_screen_bitmap (GFX_DRIVER *drv,
                                                      int w, int h, int depth)
{
	drv->w = w;
	drv->h = h;
	drv->linear        = amesa_gfx_driver->linear;
	drv->bank_size     = amesa_gfx_driver->bank_size;
	drv->bank_gran     = amesa_gfx_driver->bank_gran;
	drv->vid_mem       = amesa_gfx_driver->vid_mem;
	drv->vid_phys_base = amesa_gfx_driver->vid_phys_base;

	return AMesaGetColorBuffer(_amesa.buffer, AMESA_ACTIVE);
}



static void __allegro_gl_init_texture_read_format(void)
{
	/* 8 bpp (true color mode) */
	__allegro_gl_texture_read_format[0] = GL_UNSIGNED_BYTE_3_3_2;

	/* 15 bpp */
	if (_rgb_r_shift_15 > _rgb_b_shift_15) {
		__allegro_gl_texture_read_format[1] = GL_UNSIGNED_SHORT_5_5_5_1;
		if (_rgb_r_shift_15 == 10) {
			__allegro_gl_texture_components[1] = GL_BGRA;
		}
	}
	else {
		__allegro_gl_texture_read_format[1] = GL_UNSIGNED_SHORT_1_5_5_5_REV;
	}

	/* 16 bpp */
	if (_rgb_r_shift_16 > _rgb_b_shift_16) {
		__allegro_gl_texture_read_format[2] = GL_UNSIGNED_SHORT_5_6_5;
	}
	else {
		__allegro_gl_texture_read_format[2] = GL_UNSIGNED_SHORT_5_6_5_REV;
	}

	/* 24 bpp */
	__allegro_gl_texture_read_format[3] = GL_UNSIGNED_BYTE;

	/* 32 bpp */
	if (_rgb_r_shift_32 > _rgb_b_shift_32) {
		__allegro_gl_texture_read_format[4] = GL_UNSIGNED_INT_8_8_8_8_REV;
		if (_rgb_r_shift_32 == 16) {
			__allegro_gl_texture_components[4] = GL_BGRA;
		}
	}
	else {
		__allegro_gl_texture_read_format[4] = GL_UNSIGNED_BYTE;
	}
}



/******************************/
/* AllegroGL driver functions */
/******************************/

/* flip:
 *  Does a page flip / double buffer copy / whatever it really is.
 */
static void amesa_flip (void)
{
	AMesaSwapBuffers (_amesa.buffer);
}



/* gl_on, gl_off:
 *  Switches to/from GL mode.
 */
static void amesa_gl_on (void)
{
}



static void amesa_gl_off (void)
{
}



/*****************/
/* Driver struct */
/*****************/

static struct allegro_gl_driver allegro_gl_amesa = {
	amesa_flip,
	amesa_gl_on,
	amesa_gl_off
};

