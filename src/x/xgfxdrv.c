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
   "DGA 1.0",
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
   "Fullscreen DGA 1.0",
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



#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA2
static BITMAP *_xdga2_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);
static BITMAP *_xdga2_soft_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);


static GFX_DRIVER gfx_xdga2 =
{
   GFX_XDGA2,
   empty_string,
   empty_string,
   "DGA 2.0",
   _xdga2_gfxdrv_init,
   _xdga2_gfxdrv_exit,
   _xdga2_scroll_screen,
   _xdga2_vsync,
   _xdga2_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   640, 480,
   TRUE,
   0, 0,
   0,
   0
};


static GFX_DRIVER gfx_xdga2_soft =
{
   GFX_XDGA2_SOFT,
   empty_string,
   empty_string,
   "Software DGA 2.0",
   _xdga2_soft_gfxdrv_init,
   _xdga2_gfxdrv_exit,
   _xdga2_scroll_screen,
   _xdga2_vsync,
   _xdga2_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   640, 480,
   TRUE,
   0, 0,
   0,
   0
};

#endif



/* list the available drivers */
_DRIVER_INFO _xwin_gfx_driver_list[] =
{
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA2
   {  GFX_XDGA2,      &gfx_xdga2,      TRUE  },
   {  GFX_XDGA2_SOFT, &gfx_xdga2_soft, TRUE  },
#endif
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   {  GFX_XDFS,       &gfx_xdfs,       TRUE  },
   {  GFX_XDGA,       &gfx_xdga,       TRUE  },
#endif
   {  GFX_XWINDOWS,   &gfx_xwin,       TRUE  },
   {  0,              NULL,            0     }
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



#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA2
/* _xdga2_gfxdrv_init:
 *  Creates screen bitmap.
 */
static BITMAP *_xdga2_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xdga2_gfxdrv_init_drv(&gfx_xdga2, w, h, vw, vh, color_depth, TRUE);
}



/* _xdga2_soft_gfxdrv_init:
 *  Creates screen bitmap (software only mode).
 */
static BITMAP *_xdga2_soft_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xdga2_gfxdrv_init_drv(&gfx_xdga2_soft, w, h, vw, vh, color_depth, FALSE);
}
#endif
