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
 *      DirectDraw acceleration.
 *
 *      By Stefan Schimanski. Bugfixes by Isaac Cruz. Acceleration for
 *      rectfill() and hline() added by Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



/* software version pointers */
static void (*_orig_hline) (BITMAP * bmp, int x1, int y, int x2, int color);
static void (*_orig_rectfill) (BITMAP * bmp, int x1, int y1, int x2, int y2, int color);
static void (*_orig_draw_sprite) (BITMAP * bmp, BITMAP * sprite, int x, int y);
static void (*_orig_masked_blit) (BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);



/* ddraw_blit_to_self:
 *  Accelerated vram -> vram blitting routine.
 */
static void ddraw_blit_to_self(BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   RECT src_rect = {
      source_x + source->x_ofs,
      source_y + source->y_ofs,
      source_x + source->x_ofs + width,
      source_y + source->y_ofs + height
   };

   BITMAP *dest_parent;
   BITMAP *source_parent;

   /* find parents */
   dest_parent = dest;
   while (dest_parent->id & BMP_ID_SUB)
      dest_parent = (BITMAP *)dest_parent->extra;

   source_parent = source;
   while (source_parent->id & BMP_ID_SUB)
      source_parent = (BITMAP *)source_parent->extra;

   _enter_gfx_critical();
   gfx_directx_release_lock(dest);
   gfx_directx_release_lock(source);

   IDirectDrawSurface_BltFast(BMP_EXTRA(dest_parent)->surf,
			      dest_x + dest->x_ofs, dest_y + dest->y_ofs,
			      BMP_EXTRA(source_parent)->surf, &src_rect, DDBLTFAST_WAIT);

   _exit_gfx_critical();
}


/* ddraw_masked_blit:
 *  Accelerated masked blitting routine.
 */
static void ddraw_masked_blit(BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   RECT dest_rect = {
      dest_x + dest->x_ofs,
      dest_y + dest->y_ofs,
      dest_x + dest->x_ofs + width,
      dest_y + dest->y_ofs + height
   };

   RECT source_rect = {
      source_x + source->x_ofs,
      source_y + source->y_ofs,
      source_x + source->x_ofs + width,
      source_y + source->y_ofs + height
   };

   DDCOLORKEY src_key = {
      source->vtable->mask_color,
      source->vtable->mask_color
   };

   HRESULT hr;
   BITMAP *dest_parent;
   BITMAP *source_parent;

   /* find parents */
   dest_parent = dest;
   while (dest_parent->id & BMP_ID_SUB)
      dest_parent = (BITMAP *)dest_parent->extra;

   source_parent = source;
   while (source_parent->id & BMP_ID_SUB)
      source_parent = (BITMAP *)source_parent->extra;

   if (source->vtable == &_screen_vtable) {
      _enter_gfx_critical();
      gfx_directx_release_lock(dest);
      gfx_directx_release_lock(source);

      IDirectDrawSurface_SetColorKey(BMP_EXTRA(source_parent)->surf,
				     DDCKEY_SRCBLT, &src_key);

      hr = IDirectDrawSurface_Blt(BMP_EXTRA(dest_parent)->surf, &dest_rect,
				  BMP_EXTRA(source_parent)->surf, &source_rect,
				  DDBLT_KEYSRC | DDBLT_WAIT, NULL);

      _exit_gfx_critical();

      if (FAILED(hr))
	 _TRACE("Blt failed (%x)\n", hr);
   }
   else {
      /* have to use the original software version */
      _orig_masked_blit(source, dest, source_x, source_y, dest_x, dest_y, width, height);
   }
}



/* ddraw_draw_sprite:
 *  Accelerated sprite drawing routine.
 */
static void ddraw_draw_sprite(BITMAP * bmp, BITMAP * sprite, int x, int y)
{
   int sx, sy, w, h;

   if (sprite->vtable == &_screen_vtable) {
      sx = sprite->x_ofs;
      sy = sprite->y_ofs;
      w = sprite->w;
      h = sprite->h;

      if (bmp->clip) {
	 if (x < bmp->cl) {
	    sx += bmp->cl - x;
	    w -= bmp->cl - x;
	    x = bmp->cl;
	 }

	 if (y < bmp->ct) {
	    sy += bmp->ct - y;
	    h -= bmp->ct - y;
	    y = bmp->ct;
	 }

	 if (x + w > bmp->cr)
	    w = bmp->cr - x;

	 if (w <= 0)
	    return;

	 if (y + h > bmp->cb)
	    h = bmp->cb - y;

	 if (h <= 0)
	    return;
      }

      ddraw_masked_blit(sprite, bmp, sx, sy, x, y, w, h);
   }
   else {
      /* have to use the original software version */
      _orig_draw_sprite(bmp, sprite, x, y);
   }
}



/* ddraw_clear_to_color:
 *  Accelerated screen clear routine.
 */
static void ddraw_clear_to_color(BITMAP * bitmap, int color)
{
   RECT dest_rect = {
      bitmap->cl + bitmap->x_ofs,
      bitmap->ct + bitmap->y_ofs,
      bitmap->x_ofs + bitmap->cr,
      bitmap->y_ofs + bitmap->cb
   };
   HRESULT hr;
   DDBLTFX blt_fx;
   BITMAP *parent;

   /* find parent */
   parent = bitmap;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_release_lock(bitmap);

   hr = IDirectDrawSurface_Blt(BMP_EXTRA(parent)->surf, &dest_rect, NULL, NULL,
			       DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);

   _exit_gfx_critical();

   if (FAILED(hr))
      _TRACE("Blt failed (%x)\n", hr);
}



/* ddraw_rectfill:
 *  Accelerated rectangle fill routine.
 */
static void ddraw_rectfill(BITMAP *bitmap, int x1, int y1, int x2, int y2, int color)
{
   RECT dest_rect;
   HRESULT hr;
   DDBLTFX blt_fx;
   BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_rectfill(bitmap, x1, y1, x2, y2, color);
      return;
   }

   if (x2 < x1) {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (y2 < y1) {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bitmap->clip) {
      if (x1 < bitmap->cl)
	 x1 = bitmap->cl;

      if (x2 >= bitmap->cr)
	 x2 = bitmap->cr-1;

      if (x2 < x1)
	 return;

      if (y1 < bitmap->ct)
	 y1 = bitmap->ct;

      if (y2 >= bitmap->cb)
	 y2 = bitmap->cb-1;

      if (y2 < y1)
	 return;
   }

   dest_rect.left = x1 + bitmap->x_ofs;
   dest_rect.top = y1 + bitmap->y_ofs;
   dest_rect.right = x2 + bitmap->x_ofs + 1;
   dest_rect.bottom = y2 + bitmap->y_ofs + 1;

   /* find parent */
   parent = bitmap;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_release_lock(bitmap);

   hr = IDirectDrawSurface_Blt(BMP_EXTRA(parent)->surf, &dest_rect, NULL, NULL,
			       DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);

   _exit_gfx_critical();

   if (FAILED(hr))
      _TRACE("Blt failed (%x)\n", hr);
}



/* ddraw_hline:
 *  Accelerated scanline fill routine.
 */
static void ddraw_hline(BITMAP *bitmap, int x1, int y, int x2, int color)
{
   RECT dest_rect;
   HRESULT hr;
   DDBLTFX blt_fx;
   BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_hline(bitmap, x1, y, x2, color);
      return;
   }

   if (x1 > x2) {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (bitmap->clip) {
      if ((y < bitmap->ct) || (y >= bitmap->cb))
	 return;

      if (x1 < bitmap->cl)
	 x1 = bitmap->cl;

      if (x2 >= bitmap->cr)
	 x2 = bitmap->cr-1;

      if (x2 < x1)
	 return;
   }

   dest_rect.left = x1 + bitmap->x_ofs;
   dest_rect.top = y + bitmap->y_ofs;
   dest_rect.right = x2 + bitmap->x_ofs + 1;
   dest_rect.bottom = y + bitmap->y_ofs + 1;

   /* find parent */
   parent = bitmap;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_release_lock(bitmap);

   hr = IDirectDrawSurface_Blt(BMP_EXTRA(parent)->surf, &dest_rect, NULL, NULL,
			       DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);

   _exit_gfx_critical();

   if (FAILED(hr))
      _TRACE("Blt failed (%x)\n", hr);
}



/* enable_acceleration:
 *  checks graphic driver for capabilities to accelerate Allegro
 */
int enable_acceleration(GFX_DRIVER * drv)
{
   HRESULT hr;

   /* safe pointer to software versions */
   _orig_hline = _screen_vtable.hline;
   _orig_rectfill = _screen_vtable.rectfill;
   _orig_draw_sprite = _screen_vtable.draw_sprite;
   _orig_masked_blit = _screen_vtable.masked_blit;

   /* accelerated video to video blits? */
   if (dd_caps.dwCaps & DDCAPS_BLT) {
      _screen_vtable.blit_to_self = ddraw_blit_to_self;
      _screen_vtable.blit_to_self_forward = ddraw_blit_to_self;
      _screen_vtable.blit_to_self_backward = ddraw_blit_to_self;
      _screen_vtable.blit_from_system = ddraw_blit_to_self;
      _screen_vtable.blit_to_system = ddraw_blit_to_self;

      gfx_capabilities |= (GFX_HW_VRAM_BLIT | GFX_HW_SYS_TO_VRAM_BLIT);
   }

   /* accelerated color fills? */
   if (dd_caps.dwCaps & DDCAPS_BLTCOLORFILL) {
      _screen_vtable.clear_to_color = ddraw_clear_to_color;
      _screen_vtable.rectfill = ddraw_rectfill;
      _screen_vtable.hline = ddraw_hline;

      gfx_capabilities |= GFX_HW_HLINE | GFX_HW_FILL;
   }

   /* accelerated color key blits? */
   if ((dd_caps.dwCaps & DDCAPS_COLORKEY) &&
       (dd_caps.dwCKeyCaps & DDCKEYCAPS_SRCBLT)) {
      _screen_vtable.masked_blit = ddraw_masked_blit;
      _screen_vtable.draw_sprite = ddraw_draw_sprite;

      if (_screen_vtable.color_depth == 8)
	 _screen_vtable.draw_256_sprite = ddraw_draw_sprite;

      gfx_capabilities |= (GFX_HW_VRAM_BLIT_MASKED | GFX_HW_SYS_TO_VRAM_BLIT_MASKED);
   }

   /* triple buffering? */
   hr = IDirectDrawSurface_GetFlipStatus(BMP_EXTRA(dd_frontbuffer)->surf, DDGFS_ISFLIPDONE);
   if (hr == DDERR_WASSTILLDRAWING || hr == DD_OK) {
      drv->poll_scroll = gfx_directx_poll_scroll;
      gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }

   return 0;
}
