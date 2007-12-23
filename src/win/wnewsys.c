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
 *      New Windows system driver
 *
 *      Based on the X11 OpenGL driver by Elias Pschernig.
 *      Heavily modified by Trent Gamblin.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/platform/alplatf.h"

#include "winalleg.h"

#include "win_new.h"

static ALLEGRO_SYSTEM_INTERFACE *vt = 0;

ALLEGRO_SYSTEM_WIN *_al_win_system;

/* Create a new system object for the dummy D3D driver. */
static ALLEGRO_SYSTEM *win_initialize(int flags)
{
   _al_win_system = _AL_MALLOC(sizeof *_al_win_system);
   memset(_al_win_system, 0, sizeof *_al_win_system);

   _al_win_init_window();

   _al_vector_init(&_al_win_system->system.displays, sizeof (ALLEGRO_SYSTEM_WIN *));

   _al_win_system->system.vt = vt;

#if defined ALLEGRO_D3D
   if (_al_d3d_init_display() != true)
      return NULL;
#endif
   
   return &_al_win_system->system;
}

/* FIXME: autodetect a driver */
ALLEGRO_DISPLAY_INTERFACE *win_get_display_driver(void)
{
#if defined ALLEGRO_D3D
   return _al_display_d3d_driver();
#else
   return NULL;
#endif
}

/* FIXME: use the list */
ALLEGRO_KEYBOARD_DRIVER *win_get_keyboard_driver(void)
{
   return _al_keyboard_driver_list[0].driver;
}

int win_get_num_display_modes(void)
{
   int format = al_get_new_display_format();
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

   if (!(flags & ALLEGRO_FULLSCREEN))
      return -1;

#if defined ALLEGRO_D3D
   if (flags & ALLEGRO_DIRECT3D) {
      return _al_d3d_get_num_display_modes(format, refresh_rate, flags);
   }
#endif

   if (flags & ALLEGRO_OPENGL) {
      /* FIXME */
   }

   return 0;
}

ALLEGRO_DISPLAY_MODE *win_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode)
{
   int format = al_get_new_display_format();
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

   if (!(flags & ALLEGRO_FULLSCREEN))
      return NULL;

#if defined ALLEGRO_D3D
   if (flags & ALLEGRO_DIRECT3D) {
      return _al_d3d_get_display_mode(index, format, refresh_rate, flags, mode);
   }
#endif

   if (flags & ALLEGRO_OPENGL) {
      /* FIXME */
   }

   return NULL;
}

ALLEGRO_SYSTEM_INTERFACE *_al_system_win_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->initialize = win_initialize;
   vt->get_display_driver = win_get_display_driver;
   vt->get_keyboard_driver = win_get_keyboard_driver;
   vt->get_num_display_modes = win_get_num_display_modes;
   vt->get_display_mode = win_get_display_mode;
   vt->shutdown_system = NULL; /* might need something here */

   TRACE("ALLEGRO_SYSTEM_INTERFACE created.\n");

   return vt;
}

void _al_register_system_interfaces()
{
   ALLEGRO_SYSTEM_INTERFACE **add;

#if defined ALLEGRO_D3D
   /* This is the only system driver right now */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_win_driver();
#endif
}


static HICON win_create_icon_from_old_bitmap(struct BITMAP *sprite)
{
   int mask_color;
   int x, y;
   HDC h_dc;
   HDC h_and_dc;
   HDC h_xor_dc;
   ICONINFO iconinfo;
   HBITMAP and_mask;
   HBITMAP xor_mask;
   HBITMAP hOldAndMaskBitmap;
   HBITMAP hOldXorMaskBitmap;
   HICON hicon;
   HWND allegro_wnd = win_get_window();

   /* Create bitmap */
   h_dc = GetDC(allegro_wnd);
   h_xor_dc = CreateCompatibleDC(h_dc);
   h_and_dc = CreateCompatibleDC(h_dc);

   int width = sprite->w;
   int height = sprite->h;

   /* Prepare AND (monochrome) and XOR (colour) mask */
   and_mask = CreateBitmap(width, height, 1, 1, NULL);
   xor_mask = CreateCompatibleBitmap(h_dc, width, height);
   hOldAndMaskBitmap = SelectObject(h_and_dc, and_mask);
   hOldXorMaskBitmap = SelectObject(h_xor_dc, xor_mask);

   /* Create transparent cursor */
   for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
	 SetPixel(h_and_dc, x, y, WINDOWS_RGB(255, 255, 255));
	 SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
      }
   }
   draw_to_hdc(h_xor_dc, sprite, 0, 0);
   mask_color = bitmap_mask_color(sprite);

   /* Make cursor background transparent */
   for (y = 0; y < sprite->h; y++) {
      for (x = 0; x < sprite->w; x++) {
	 if (getpixel(sprite, x, y) != mask_color) {
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
   ReleaseDC(allegro_wnd, h_dc);

   iconinfo.fIcon = FALSE;
   iconinfo.hbmMask = and_mask;
   iconinfo.hbmColor = xor_mask;

   hicon = CreateIconIndirect(&iconinfo);

   DeleteObject(and_mask);
   DeleteObject(xor_mask);

   return hicon;
}


void _al_win_set_display_icon(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bmp)
{
   /* Convert to BITMAP */
   BITMAP *oldbmp = create_bitmap_ex(32,
      al_get_bitmap_width(bmp), al_get_bitmap_height(bmp));
   
   BITMAP *scaled_bmp = create_bitmap_ex(32, 16, 16);

   int x, y;
   for (y = 0; y < oldbmp->h; y++) {
      for (x = 0; x < oldbmp->w; x++) {
         ALLEGRO_COLOR color;
         al_get_pixel(bmp, x, y, &color);
         unsigned char r, g, b, a;
         al_unmap_rgba_ex(al_get_bitmap_format(bmp), &color, &r, &g, &b, &a);
         int oldcolor;
         if (a == 0)
            oldcolor = makecol32(255, 0, 255);
         else
            oldcolor = makecol32(r, g, b);
         putpixel(oldbmp, x, y, oldcolor);
      }
   }

   stretch_blit(oldbmp, scaled_bmp, 0, 0, oldbmp->w, oldbmp->h,
   	0, 0, scaled_bmp->w, scaled_bmp->h);

   HICON scaled_icon = win_create_icon_from_old_bitmap(scaled_bmp);

   HWND hwnd = win_get_window();

   SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)scaled_icon);
   SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)scaled_icon);

   destroy_bitmap(oldbmp);
   destroy_bitmap(scaled_bmp);
}


