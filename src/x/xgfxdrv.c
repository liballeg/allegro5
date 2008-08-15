#error This file is no longer used.

/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Video driver for X-Windows.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/platform/aintunix.h"
#include "xwin.h"

#if 0

static BITMAP *_xwin_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);
static void _xwin_gfxdrv_exit(BITMAP *bmp);


static GFX_DRIVER gfx_xwin =
{
   GFX_XWINDOWS,
   empty_string,
   empty_string,
   "X11 window",
   _xwin_gfxdrv_init,
   _xwin_gfxdrv_exit,
   _xwin_scroll_screen,
   _xwin_vsync,
   _xwin_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,	/* old hardware cursor methods */
   _xwin_drawing_mode,
   NULL, NULL,
   NULL,    // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,
   320, 200,
   TRUE,
   0, 0,
   0x10000,
   0,
   TRUE,
   /* new_api_branch additions */
#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   _al_xwin_create_mouse_cursor,
   _al_xwin_destroy_mouse_cursor,
   _al_xwin_set_mouse_cursor,
   _al_xwin_set_system_mouse_cursor,
   _al_xwin_show_mouse_cursor,
   _al_xwin_hide_mouse_cursor
#else
   NULL, NULL, NULL, NULL, NULL, NULL
#endif
};



static BITMAP *_xwin_fullscreen_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);


static GFX_DRIVER gfx_xwin_fullscreen =
{
   GFX_XWINDOWS_FULLSCREEN,
   empty_string,
   empty_string,
   "X11 fullscreen",
   _xwin_fullscreen_gfxdrv_init,
   _xwin_gfxdrv_exit,
   _xwin_scroll_screen,
   _xwin_vsync,
   _xwin_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,	/* old hardware cursor methods */
   _xwin_drawing_mode,
   NULL, NULL,
   NULL,
   _xwin_fetch_mode_list,
   320, 200,
   TRUE,
   0, 0,
   0x10000,
   0,
   FALSE,
   /* new_api_branch additions */
#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   _al_xwin_create_mouse_cursor,
   _al_xwin_destroy_mouse_cursor,
   _al_xwin_set_mouse_cursor,
   _al_xwin_set_system_mouse_cursor,
   _al_xwin_show_mouse_cursor,
   _al_xwin_hide_mouse_cursor
#else
   NULL, NULL, NULL, NULL, NULL, NULL
#endif
};



/* list the available drivers */
_DRIVER_INFO _xwin_gfx_driver_list[] =
{
#if (defined ALLEGRO_XWINDOWS_WITH_XF86DGA2) && (!defined ALLEGRO_WITH_MODULES)
   {  GFX_XDGA2,               &gfx_xdga2,           FALSE  },
   {  GFX_XDGA2_SOFT,          &gfx_xdga2_soft,      FALSE  },
#endif
   {  GFX_XWINDOWS_FULLSCREEN, &gfx_xwin_fullscreen, TRUE  },
   {  GFX_XWINDOWS,            &gfx_xwin,            TRUE  },
   {  0,                       NULL,                 0     }
};



/* _xwin_gfxdrv_init:
 *  Creates screen bitmap.
 */
static BITMAP *_xwin_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xwin_create_screen(&gfx_xwin, w, h, vw, vh, color_depth, FALSE);
}



/* _xwin_gfxdrv_exit:
 *  Shuts down the X-Windows driver.
 */
static void _xwin_gfxdrv_exit(BITMAP *bmp)
{
   _xwin_destroy_screen();
}



/* _xwin_fullscreen_gfxdrv_init:
 *  Creates screen bitmap (with video mode extension).
 */
static BITMAP *_xwin_fullscreen_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xwin_create_screen(&gfx_xwin_fullscreen, w, h, vw, vh, color_depth, TRUE);
}

#endif
