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
 *      DirectDraw gfx drivers header
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#ifndef WDDRAW_H_INCLUDED
#define WDDRAW_H_INCLUDED

#ifndef DIRECTDRAW_VERSION
   #define DIRECTDRAW_VERSION 0x0300
#endif

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"

#ifndef SCAN_DEPEND
   #include <ddraw.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

/* vtable routines */
AL_FUNC(void, gfx_directx_exit, (BITMAP *b));
AL_FUNC(void, gfx_directx_sync, (void));
AL_FUNC(void, gfx_directx_set_palette, (AL_CONST RGB *p, int from, int to, int vsync));
AL_FUNC(int, gfx_directx_poll_scroll, (void));
AL_FUNC(void, gfx_directx_created_sub_bitmap, (BITMAP *bmp, BITMAP *parent));
AL_FUNC(BITMAP *, gfx_directx_create_video_bitmap, (int width, int height));
AL_FUNC(void, gfx_directx_destroy_video_bitmap, (BITMAP *bitmap));
AL_FUNC(int, gfx_directx_show_video_bitmap, (BITMAP *bitmap));
AL_FUNC(int, gfx_directx_request_video_bitmap, (BITMAP *bitmap));
AL_FUNC(BITMAP *, gfx_directx_create_system_bitmap, (int width, int height));
AL_FUNC(void, gfx_directx_destroy_system_bitmap, (BITMAP *bitmap));
AL_FUNC(void, gfx_directx_destroy_surf, (LPDIRECTDRAWSURFACE surf));


/* driver initialisation and shutdown */
AL_FUNC(int, init_directx, (void));
AL_FUNC(int, set_video_mode, (int w, int h, int v_w, int v_h, int color_depth));
AL_FUNC(int, create_palette, (LPDIRECTDRAWSURFACE surf));
AL_FUNC(int, create_primary, (void));
AL_FUNC(int, create_clipper, (HWND hwnd));
AL_FUNC(int, setup_driver, (GFX_DRIVER * drv, int w, int h, int color_depth));
AL_FUNC(int, finalize_directx_init, (void));
AL_FUNC(int, gfx_directx_wnd_exit, (void));
AL_FUNC(void, gfx_directx_exit, (BITMAP *b));
AL_FUNC(int, enable_acceleration, (GFX_DRIVER * drv));


/* video mode setting */
AL_FUNC(int, gfx_directx_compare_color_depth, (int color_depth));
AL_FUNC(int, gfx_directx_update_color_format, (LPDIRECTDRAWSURFACE surf, int color_depth));

AL_VAR(int, desktop_depth);
AL_VAR(BOOL, same_color_depth);


/* bitmap locking */
AL_FUNC(void, gfx_directx_lock, (BITMAP *bmp));
AL_FUNC(void, gfx_directx_autolock, (BITMAP* bmp));
AL_FUNC(void, gfx_directx_unlock, (BITMAP *bmp));
AL_FUNC(void, gfx_directx_unlock_win, (BITMAP *bmp));
AL_FUNC(void, gfx_directx_release_lock, (BITMAP * bmp));
AL_FUNC(void, gfx_directx_write_bank, (void));
AL_FUNC(void, gfx_directx_unwrite_bank, (void));
AL_FUNC(void, gfx_directx_write_bank_win, (void));
AL_FUNC(void, gfx_directx_unwrite_bank_win, (void));
AL_FUNCPTR(void, ptr_gfx_directx_autolock, (BITMAP* bmp));
AL_FUNCPTR(void, ptr_gfx_directx_unlock, (BITMAP* bmp));


/* bitmap creation */
AL_FUNC(LPDIRECTDRAWSURFACE, gfx_directx_create_surface, (int w, int h, LPDDPIXELFORMAT pixel_format,
   int video, int primary, int overlay));
AL_FUNC(BITMAP *, make_directx_bitmap, (LPDIRECTDRAWSURFACE surf, int w, int h, int color_depth, int id));


/* video bitmap list */
AL_FUNC(void, register_directx_bitmap, (BITMAP *bmp));
AL_FUNC(void, unregister_directx_bitmap, (BITMAP *bmp));
AL_FUNC(void, unregister_all_directx_bitmaps, (void));
AL_FUNC(void, release_directx_bitmap, (struct BITMAP *bmp));

AL_VAR(BMP_EXTRA_INFO *, directx_bmp_list);


/* overlay */
AL_FUNC(void, hide_overlay, (void));
AL_FUNC(void, wddovl_switch_out, (void));
AL_FUNC(void, wddovl_switch_in, (void));

AL_VAR(int, wnd_x);
AL_VAR(int, wnd_y);
AL_VAR(int, wnd_width);
AL_VAR(int, wnd_height);
AL_VAR(int, wnd_sysmenu);


/* windowed mode */
AL_FUNC(void, handle_window_moving_win, (void));
AL_FUNC(void, handle_window_size_win, (void));
AL_FUNCPTR(void, update_window, (RECT* rect));
AL_FUNC(void, wddwin_switch_out, (void));
AL_FUNC(void, wddwin_switch_in, (void));

AL_FUNC(void,  _update_8_to_15, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_24_to_15, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_32_to_15, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void,  _update_8_to_16, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_24_to_16, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_32_to_16, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void,  _update_8_to_24, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_16_to_24, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_32_to_24, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void,  _update_8_to_32, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_16_to_32, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));
AL_FUNC(void, _update_24_to_32, (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc));

AL_VAR(BITMAP*, pseudo_screen);
AL_VAR(int*, allegro_palette);
AL_VAR(int*, rgb_scale_5335);
AL_VAR(int*, dirty_lines);


#endif

