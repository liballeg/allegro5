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


#include "allegro.h"
#include "allegro/aintunix.h"
#include "xwin.h"


static BITMAP *_xwin_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);
static void _xwin_gfxdrv_exit(BITMAP *bmp);


static GFX_DRIVER gfx_xwin =
{
   GFX_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows graphics",
   _xwin_gfxdrv_init,
   _xwin_gfxdrv_exit,
   _xwin_scroll_screen,
   _xwin_vsync,
   _xwin_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   320, 200,
   TRUE,
   0, 0,
   0x10000,
   0
};



#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
static BITMAP *_xdga_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);
static BITMAP *_xdfs_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);
static void _xdga_gfxdrv_exit(BITMAP *bmp);


static GFX_DRIVER gfx_xdga =
{
   GFX_XDGA,
   empty_string,
   empty_string,
   "DGA graphics",
   _xdga_gfxdrv_init,
   _xdga_gfxdrv_exit,
   _xdga_scroll_screen,
   _xwin_vsync,
   _xdga_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   320, 200,
   TRUE,
   0, 0,
   0x10000,
   0
};


static GFX_DRIVER gfx_xdfs =
{
   GFX_XDFS,
   empty_string,
   empty_string,
   "Fullscreen DGA",
   _xdfs_gfxdrv_init,
   _xdga_gfxdrv_exit,
   _xdga_scroll_screen,
   _xwin_vsync,
   _xdga_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   320, 200,
   TRUE,
   0, 0,
   0x10000,
   0
};
#endif



/* list the available drivers */
_DRIVER_INFO _xwin_gfx_driver_list[] =
{
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   {  GFX_XDFS,     &gfx_xdfs, TRUE  },
   {  GFX_XDGA,     &gfx_xdga, TRUE  },
#endif
   {  GFX_XWINDOWS, &gfx_xwin, TRUE  },
   {  0,            NULL,      0     }
};



/* _xwin_gfxdrv_init:
 *  Creates screen bitmap.
 */
static BITMAP *_xwin_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xwin_create_screen(&gfx_xwin, w, h, vw, vh, color_depth);
}



/* _xwin_gfxdrv_exit:
 *  Shuts down the X-Windows driver.
 */
static void _xwin_gfxdrv_exit(BITMAP *bmp)
{
   _xwin_destroy_screen();
}



#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
/* _xdga_gfxdrv_init:
 *  Creates screen bitmap.
 */
static BITMAP *_xdga_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xdga_create_screen(&gfx_xdga, w, h, vw, vh, color_depth, FALSE);
}

static BITMAP *_xdfs_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   /* attempt to use full screen */
   return _xdga_create_screen(&gfx_xdfs, w, h, vw, vh, color_depth, TRUE);
}

/* _xdga_gfxdrv_exit:
 *  Shuts down the DGA X-Windows driver.
 */
static void _xdga_gfxdrv_exit(BITMAP *bmp)
{
   _xdga_destroy_screen();
}
#endif

