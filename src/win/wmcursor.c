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


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/platform/aintwin.h"


static void local_stretch_blit_to_hdc(ALLEGRO_BITMAP *bitmap, HDC dc,
   int src_x, int src_y, int src_w, int src_h,
   int dest_x, int dest_y, int dest_w, int dest_h);
static void local_draw_to_hdc(HDC dc, ALLEGRO_BITMAP *bitmap, int x, int y);


HICON _al_win_create_icon(HWND wnd,
   ALLEGRO_BITMAP *sprite, int xfocus, int yfocus, bool is_cursor, bool resize)
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
   HICON icon;
   bool was_locked;
   ALLEGRO_BITMAP *tmp = sprite;

   if (resize) {
      if (is_cursor) {
         /* Get allowed cursor size - Windows can't make cursors of arbitrary size */
         sys_sm_cx = GetSystemMetrics(SM_CXCURSOR);
         sys_sm_cy = GetSystemMetrics(SM_CYCURSOR);
      }
      else {
         sys_sm_cx = GetSystemMetrics(SM_CXICON);
         sys_sm_cy = GetSystemMetrics(SM_CYICON);
      }

      if ((tmp->w > sys_sm_cx) || (tmp->h > sys_sm_cy)) {
         float ratio = tmp->w / (float)tmp->h;
         int w, h;
         ALLEGRO_STATE state;
         if (ratio > 1) {
            w = sys_sm_cx;
            h = sys_sm_cy / ratio;
         }
         else {
            w = sys_sm_cx * ratio;
            h = sys_sm_cy;
         }
         al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP |
            ALLEGRO_STATE_NEW_BITMAP_PARAMETERS |
            ALLEGRO_STATE_BLENDER);
         al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
         al_set_new_bitmap_format(al_get_bitmap_format(tmp));
         tmp = al_create_bitmap(w, h);
         al_set_target_bitmap(tmp);
         al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
         al_draw_scaled_bitmap(
            sprite,
            0, 0, sprite->w, sprite->h,
            0, 0, w, h,
            0
         );
         al_restore_state(&state);
      }
   }
   else {
      sys_sm_cx = al_get_bitmap_width(tmp);
      sys_sm_cy = al_get_bitmap_height(tmp);
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

   /* Lock sprite to speed up repeated get pixel calls. */
   was_locked = al_is_bitmap_locked(tmp);
   if (!was_locked) {
      al_lock_bitmap(tmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
   }

   local_draw_to_hdc(h_xor_dc, tmp, 0, 0);

   /* Make cursor background transparent */
   for (y = 0; y < tmp->h; y++) {
      for (x = 0; x < tmp->w; x++) {
         ALLEGRO_COLOR c;
         unsigned char r, g, b, a;

         c = al_get_pixel(tmp, x, y);
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

   if (!was_locked) {
      al_unlock_bitmap(tmp);
   }

   SelectObject(h_and_dc, hOldAndMaskBitmap);
   SelectObject(h_xor_dc, hOldXorMaskBitmap);
   DeleteDC(h_and_dc);
   DeleteDC(h_xor_dc);
   ReleaseDC(wnd, h_dc);

   iconinfo.fIcon = is_cursor ? false : true;
   iconinfo.xHotspot = xfocus;
   iconinfo.yHotspot = yfocus;
   iconinfo.hbmMask = and_mask;
   iconinfo.hbmColor = xor_mask;

   icon = CreateIconIndirect(&iconinfo);

   DeleteObject(and_mask);
   DeleteObject(xor_mask);

   if (sprite != tmp) {
      al_destroy_bitmap(tmp);
   }

   return icon;
}


ALLEGRO_MOUSE_CURSOR *_al_win_create_mouse_cursor(ALLEGRO_BITMAP *sprite,
   int xfocus, int yfocus)
{
   HWND wnd;
   HCURSOR hcursor;
   ALLEGRO_MOUSE_CURSOR_WIN *win_cursor;

   /* A null HWND retrieves the DC for the entire screen. */
   wnd = NULL;

   hcursor = (HCURSOR)_al_win_create_icon(wnd, sprite, xfocus, yfocus, true, true);
   if (!hcursor) {
      return NULL;
   }

   win_cursor = al_malloc(sizeof *win_cursor);
   if (!win_cursor) {
      DestroyIcon(hcursor);
      return NULL;
   }

   win_cursor->hcursor = hcursor;
   return (ALLEGRO_MOUSE_CURSOR *)win_cursor;
}



void _al_win_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_MOUSE_CURSOR_WIN *win_cursor = (ALLEGRO_MOUSE_CURSOR_WIN *) cursor;
   ALLEGRO_SYSTEM *sys = al_get_system_driver();
   unsigned i;

   ASSERT(win_cursor);

   /* XXX not at all thread safe */

   for (i = 0; i < _al_vector_size(&sys->displays); i++) {
      ALLEGRO_DISPLAY_WIN **slot = _al_vector_ref(&sys->displays, i);
      ALLEGRO_DISPLAY_WIN *win_display = *slot;

      if (win_cursor->hcursor == win_display->mouse_selected_hcursor) {
         _al_win_set_system_mouse_cursor((ALLEGRO_DISPLAY *)win_display,
            ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW);
      }
   }

   DestroyIcon(win_cursor->hcursor);
   al_free(win_cursor);
}



bool _al_win_set_mouse_cursor(ALLEGRO_DISPLAY *display,
                              ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   ALLEGRO_MOUSE_CURSOR_WIN *win_cursor = (ALLEGRO_MOUSE_CURSOR_WIN *) cursor;

   ASSERT(win_cursor);
   ASSERT(win_cursor->hcursor);

   win_display->mouse_selected_hcursor = win_cursor->hcursor;

   if (win_display->mouse_cursor_shown) {
      POINT p;
   
      SetCursor(win_cursor->hcursor);

      /* Windows is too stupid to actually display the mouse pointer when we
       * change it and waits until it is moved, so we have to generate a fake
       * mouse move to actually show the cursor.
       */
      GetCursorPos(&p);
      SetCursorPos(p.x, p.y);
   }

   return true;
}


bool _al_win_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   ALLEGRO_MOUSE_CURSOR_WIN tmp_cursor;
   ALLEGRO_MOUSE_CURSOR_WIN *tmp_cursor_ptr = &tmp_cursor;

   /* XXX do we need this? */
   if (!win_display->mouse_selected_hcursor) {
      _al_win_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW);
   }

   tmp_cursor.hcursor = win_display->mouse_selected_hcursor;
   win_display->mouse_cursor_shown = true;
   _al_win_set_mouse_cursor(display, (ALLEGRO_MOUSE_CURSOR *)tmp_cursor_ptr);

   return true;
}

bool _al_win_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   win_display->mouse_cursor_shown = false;
   PostMessage(win_display->window, WM_SETCURSOR, 0, 0);

   return true;
}


static HCURSOR system_cursor_to_hcursor(ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   switch (cursor_id) {
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW:
         return LoadCursor(NULL, IDC_ARROW);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY:
         return LoadCursor(NULL, IDC_WAIT);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION:
         return LoadCursor(NULL, IDC_HELP);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT:
         return LoadCursor(NULL, IDC_IBEAM);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE:
         return LoadCursor(NULL, IDC_SIZEALL);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_S:
         return LoadCursor(NULL, IDC_SIZENS);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_W:
         return LoadCursor(NULL, IDC_SIZEWE);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SW:
         return LoadCursor(NULL, IDC_SIZENESW);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SE:
         return LoadCursor(NULL, IDC_SIZENWSE);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_PROGRESS:
         return LoadCursor(NULL, IDC_APPSTARTING);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_PRECISION:
         return LoadCursor(NULL, IDC_CROSS);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_LINK:
         return LoadCursor(NULL, IDC_HAND);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ALT_SELECT:
         return LoadCursor(NULL, IDC_UPARROW);
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_UNAVAILABLE:
         return LoadCursor(NULL, IDC_NO);
      default:
         return NULL;
   }
}


bool _al_win_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   HCURSOR wc;

   wc = system_cursor_to_hcursor(cursor_id);
   if (!wc) {
      return false;
   }

   win_display->mouse_selected_hcursor = wc;

   if (win_display->mouse_cursor_shown) {
      /*
      MySetCursor(wc);
      PostMessage(wgl_display->window, WM_MOUSEMOVE, 0, 0);
      */
      ALLEGRO_MOUSE_CURSOR_WIN tmp_cursor;
      ALLEGRO_MOUSE_CURSOR_WIN *tmp_cursor_ptr = &tmp_cursor;
      tmp_cursor.hcursor = wc;
      _al_win_set_mouse_cursor(display, (ALLEGRO_MOUSE_CURSOR *)tmp_cursor_ptr);
   }
   return true;
}



/* GDI stuff - this really belongs elsewhere */


/* get_bitmap_info:
 *  Returns a BITMAPINFO structure suited to an ALLEGRO_BITMAP.
 *  You have to free the memory allocated by this function.
 *
 *  This version always returns a 32-bit BITMAPINFO.
 */
static BITMAPINFO *get_bitmap_info(ALLEGRO_BITMAP *bitmap)
{
   BITMAPINFO *bi;
   int i;

   bi = (BITMAPINFO *) al_malloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);

   ZeroMemory(&bi->bmiHeader, sizeof(BITMAPINFOHEADER));

   bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bi->bmiHeader.biBitCount = 32;
   bi->bmiHeader.biPlanes = 1;
   bi->bmiHeader.biWidth = al_get_bitmap_width(bitmap);
   bi->bmiHeader.biHeight = -al_get_bitmap_height(bitmap);
   bi->bmiHeader.biClrUsed = 256;
   bi->bmiHeader.biCompression = BI_RGB;

   for (i = 0; i < 256; i++) {
      bi->bmiColors[i].rgbRed = 0;
      bi->bmiColors[i].rgbGreen = 0;
      bi->bmiColors[i].rgbBlue = 0;
      bi->bmiColors[i].rgbReserved = 0;
   }

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

   pixels = (BYTE *) al_malloc(h * pitch);
   if (!pixels)
      return NULL;

   for (y = 0; y < h; y++) {
      dst = pixels + y * pitch;

      for (x = 0; x < w; x++) {
         ALLEGRO_COLOR col;
         unsigned char r, g, b, a;

         col = al_get_pixel(bitmap, x, y);
         al_unmap_rgba(col, &r, &g, &b, &a);

         /* BGR */
         dst[0] = b;
         dst[1] = g;
         dst[2] = r;
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

   al_free(pixels);
   al_free(bi);
}



/* vim: set sts=3 sw=3 et: */
