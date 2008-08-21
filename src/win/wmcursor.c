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
 *      Windows mouse cursors.
 *
 *      By Evert Glebbeek.
 *
 *      Adapted for Allegro 4.9 by Peter Wang.
 *
 *      GDI code adapted from src/win/gdi.c.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "win_new.h"


static void local_stretch_blit_to_hdc(ALLEGRO_BITMAP *bitmap, HDC dc,
   int src_x, int src_y, int src_w, int src_h,
   int dest_x, int dest_y, int dest_w, int dest_h);
static void local_draw_to_hdc(HDC dc, ALLEGRO_BITMAP *bitmap, int x, int y);



static HCURSOR _al_win_create_mouse_hcursor(HWND wnd,
   ALLEGRO_BITMAP *sprite, int xfocus, int yfocus)
{
   int x, y;
   int sys_sm_cx, sys_sm_cy;
   HDC h_dc;
   HDC h_and_dc;
   HDC h_xor_dc;
   ICONINFO iconinfo;
   HBITMAP and_mask;
   HBITMAP xor_mask;
   HBITMAP hOldAndMaskBitmap;
   HBITMAP hOldXorMaskBitmap;
   HCURSOR hcursor;

   /* Get allowed cursor size - Windows can't make cursors of arbitrary size */
   sys_sm_cx = GetSystemMetrics(SM_CXCURSOR);
   sys_sm_cy = GetSystemMetrics(SM_CYCURSOR);

   if ((sprite->w > sys_sm_cx) || (sprite->h > sys_sm_cy)) {
      return NULL;
   }

   /* Create bitmap */
   h_dc = GetDC(wnd);
   h_xor_dc = CreateCompatibleDC(h_dc);
   h_and_dc = CreateCompatibleDC(h_dc);

   /* Prepare AND (monochrome) and XOR (colour) mask */
   and_mask = CreateBitmap(sys_sm_cx, sys_sm_cy, 1, 1, NULL);
   xor_mask = CreateCompatibleBitmap(h_dc, sys_sm_cx, sys_sm_cy);
   hOldAndMaskBitmap = (HBITMAP) SelectObject(h_and_dc, and_mask);
   hOldXorMaskBitmap = (HBITMAP) SelectObject(h_xor_dc, xor_mask);

   /* Create transparent cursor */
   for (y = 0; y < sys_sm_cy; y++) {
      for (x = 0; x < sys_sm_cx; x++) {
	 SetPixel(h_and_dc, x, y, WINDOWS_RGB(255, 255, 255));
	 SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
      }
   }
   local_draw_to_hdc(h_xor_dc, sprite, 0, 0);

   /* Make cursor background transparent */
   for (y = 0; y < sprite->h; y++) {
      for (x = 0; x < sprite->w; x++) {
         ALLEGRO_COLOR c;
         unsigned char r, g, b, a;

         c = al_get_pixel(sprite, x, y);
         al_unmap_rgba(c, &r, &g, &b, &a);
         if (a != 0) {
	    /* Don't touch XOR value */
	    SetPixel(h_and_dc, x, y, 0);
	 }
	 else {
	    /* No need to touch AND value */
	    SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
	 }
      }
   }

   SelectObject(h_and_dc, hOldAndMaskBitmap);
   SelectObject(h_xor_dc, hOldXorMaskBitmap);
   DeleteDC(h_and_dc);
   DeleteDC(h_xor_dc);
   ReleaseDC(wnd, h_dc);

   iconinfo.fIcon = FALSE;
   iconinfo.xHotspot = xfocus;
   iconinfo.yHotspot = yfocus;
   iconinfo.hbmMask = and_mask;
   iconinfo.hbmColor = xor_mask;

   hcursor = CreateIconIndirect(&iconinfo);

   DeleteObject(and_mask);
   DeleteObject(xor_mask);

   return hcursor;
}



ALLEGRO_MOUSE_CURSOR_WIN *_al_win_create_mouse_cursor(HWND wnd,
   ALLEGRO_BITMAP *sprite, int xfocus, int yfocus)
{
   HCURSOR hcursor;
   ALLEGRO_MOUSE_CURSOR_WIN *win_cursor;

   hcursor = _al_win_create_mouse_hcursor(wnd, sprite, xfocus, yfocus);
   if (!hcursor) {
      return NULL;
   }

   win_cursor = _AL_MALLOC(sizeof *win_cursor);
   if (!win_cursor) {
      DestroyIcon(hcursor);
      return NULL;
   }

   win_cursor->hcursor = hcursor;
   return win_cursor;
}



void _al_win_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR_WIN *win_cursor)
{
   ASSERT(win_cursor);
   ASSERT(win_cursor->hcursor);

   DestroyIcon(win_cursor->hcursor);
   _AL_FREE(win_cursor);
}



void _al_win_set_mouse_hcursor(HCURSOR hcursor)
{
   POINT p;
   
   SetCursor(hcursor);

   /* Windows is too stupid to actually display the mouse pointer when we
    * change it and waits until it is moved, so we have to generate a fake
    * mouse move to actually show the cursor.
    */
   GetCursorPos(&p);
   SetCursorPos(p.x, p.y);
}



HCURSOR _al_win_system_cursor_to_hcursor(ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   switch (cursor_id) {
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW:
         return LoadCursor(NULL, IDC_ARROW);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY:
         return LoadCursor(NULL, IDC_WAIT);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION:
         return LoadCursor(NULL, IDC_HELP);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT:
         return LoadCursor(NULL, IDC_IBEAM);
      default:
         return NULL;
   }
}



/* GDI stuff - this really belongs elsewhere */


/* get_bitmap_info:
 *  Returns a BITMAPINFO structure suited to an ALLEGRO_BITMAP.
 *  You have to free the memory allocated by this function.
 */
static BITMAPINFO *get_bitmap_info(ALLEGRO_BITMAP *bitmap)
{
   BITMAPINFO *bi;
   int bpp, i;

   bi = (BITMAPINFO *) _AL_MALLOC(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);

   bpp = al_get_pixel_format_bits(al_get_bitmap_format(bitmap));
   if (bpp == 15)
      bpp = 16;

   ZeroMemory(&bi->bmiHeader, sizeof(BITMAPINFOHEADER));

   bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bi->bmiHeader.biBitCount = bpp;
   bi->bmiHeader.biPlanes = 1;
   bi->bmiHeader.biWidth = bitmap->w;
   bi->bmiHeader.biHeight = -bitmap->h;
   bi->bmiHeader.biClrUsed = 256;
   bi->bmiHeader.biCompression = BI_RGB;

   /*
   if (pal) {
      for (i = 0; i < 256; i++) {
	 bi->bmiColors[i].rgbRed = _rgb_scale_6[pal[i].r];
	 bi->bmiColors[i].rgbGreen = _rgb_scale_6[pal[i].g];
	 bi->bmiColors[i].rgbBlue = _rgb_scale_6[pal[i].b];
	 bi->bmiColors[i].rgbReserved = 0;
      }
   }
   */

   return bi;
}



/* get_dib_from_bitmap_32:
 *  Creates a Windows device-independent bitmap (DIB) from an Allegro BITMAP.
 *  You have to free the memory allocated by this function.
 *
 *  This version always creates a 32-bit DIB.
 */
static BYTE *get_dib_from_bitmap_32(ALLEGRO_BITMAP *bitmap)
{
   int w, h;
   int x, y;
   int pitch;
   BYTE *pixels;
   BYTE *dst;

   w = al_get_bitmap_width(bitmap);
   h = al_get_bitmap_height(bitmap);
   pitch = w * 4;

   pixels = (BYTE *) _AL_MALLOC_ATOMIC(h * pitch);
   if (!pixels)
      return NULL;

   for (y = 0; y < h; y++) {
      dst = pixels + y * pitch;

      for (x = 0; x < w; x++) {
         ALLEGRO_COLOR col;
         unsigned char r, g, b, a;

         col = al_get_pixel(bitmap, x, y);
         al_unmap_rgba(col, &r, &g, &b, &a);

         dst[0] = r;
         dst[1] = g;
         dst[2] = b;
         dst[3] = a;

         dst += 4;
      }
   }

   return pixels;
}



/* draw_to_hdc:
 *  Draws an entire Allegro BITMAP to a Windows DC. Has a syntax similar to
 *  draw_sprite().
 */
static void local_draw_to_hdc(HDC dc, ALLEGRO_BITMAP *bitmap, int x, int y)
{
   int w = al_get_bitmap_width(bitmap);
   int h = al_get_bitmap_height(bitmap);
   local_stretch_blit_to_hdc(bitmap, dc, 0, 0, w, h, x, y, w, h);
}



/* stretch_blit_to_hdc:
 *  Blits an Allegro BITMAP to a Windows DC. Has a syntax similar to
 *  stretch_blit().
 */
static void local_stretch_blit_to_hdc(ALLEGRO_BITMAP *bitmap, HDC dc,
   int src_x, int src_y, int src_w, int src_h,
   int dest_x, int dest_y, int dest_w, int dest_h)
{
   const int bitmap_w = al_get_bitmap_width(bitmap);
   const int bitmap_h = al_get_bitmap_height(bitmap);
   const int bottom_up_src_y = bitmap_h - src_y - src_h;
   BYTE *pixels;
   BITMAPINFO *bi;

   bi = get_bitmap_info(bitmap);
   pixels = get_dib_from_bitmap_32(bitmap);

   /* Windows treats all source bitmaps as bottom-up when using StretchDIBits
    * unless the source (x,y) is (0,0).  To work around this buggy behavior, we
    * can use negative heights to reverse the direction of the blits.
    *
    * See <http://wiki.allegro.cc/StretchDIBits> for a detailed explanation.
    */
   if (bottom_up_src_y == 0 && src_x == 0 && src_h != bitmap_h) {
      StretchDIBits(dc, dest_x, dest_h+dest_y-1, dest_w, -dest_h,
	 src_x, bitmap_h - src_y + 1, src_w, -src_h, pixels, bi,
	 DIB_RGB_COLORS, SRCCOPY);
   }
   else {
      StretchDIBits(dc, dest_x, dest_y, dest_w, dest_h,
	 src_x, bottom_up_src_y, src_w, src_h, pixels, bi,
	 DIB_RGB_COLORS, SRCCOPY);
   }

   _AL_FREE(pixels);
   _AL_FREE(bi);
}



/* vim: set sts=3 sw=3 et: */
