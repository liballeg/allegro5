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
 *      Windows header file for the Allegro library.
 *      This should be included by everyone and everything.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef WIN_ALLEGRO_H
#define WIN_ALLEGRO_H

#ifdef __cplusplus
   extern "C" {
#endif

#ifndef ALLEGRO_H
#error Please include allegro.h before winalleg.h!
#endif

#define BITMAP WINDOWS_BITMAP
#define WIN32_LEAN_AND_MEAN
#define NO_STRICT

#if (!defined SCAN_EXPORT) && (!defined SCAN_DEPEND)
#include <windows.h>
#endif

#undef BITMAP
#undef RGB


AL_FUNC(void, win_set_window, (HWND wnd));
AL_FUNC(HWND, win_get_window, (void));
AL_FUNC(void, win_set_wnd_create_proc, (HWND));

AL_FUNC(HDC, win_get_dc, (BITMAP *bmp));
AL_FUNC(void, win_release_dc, (BITMAP *bmp, HDC dc));

AL_FUNC(void, set_gdi_color_format, (void));
AL_FUNC(void, set_palette_to_hdc, (HDC dc, PALETTE pal));
AL_FUNC(HPALETTE, convert_palette_to_hpalette, (PALETTE pal));
AL_FUNC(void, convert_hpalette_to_palette, (HPALETTE hpal, PALETTE pal));
AL_FUNC(HBITMAP, convert_bitmap_to_hbitmap, (BITMAP *bitmap));
AL_FUNC(BITMAP *, convert_hbitmap_to_bitmap, (HBITMAP bitmap));
AL_FUNC(void, draw_to_hdc, (HDC dc, BITMAP *bitmap, int x, int y));
AL_FUNC(void, blit_to_hdc, (BITMAP *bitmap, HDC dc, int src_x, int src_y, int dest_x, int dest_y, int w, int h));
AL_FUNC(void, stretch_blit_to_hdc, (BITMAP *bitmap, HDC dc, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h));
AL_FUNC(void, blit_from_hdc, (HDC dc, BITMAP *bitmap, int src_x, int src_y, int dest_x, int dest_y, int w, int h));
AL_FUNC(void, stretch_blit_from_hdc, (HDC hdc, BITMAP *bitmap, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h));



#ifdef __cplusplus
   }
#endif

#endif          /* ifndef WIN_ALLEGRO_H */


