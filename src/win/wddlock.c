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
 *      DirectDraw bitmap locking.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"


void (*ptr_gfx_directx_autolock) (BITMAP* bmp) = gfx_directx_autolock;
void (*ptr_gfx_directx_unlock) (BITMAP* bmp) = gfx_directx_unlock;



/* gfx_directx_switch_out:
 *  Arranges for drawing requests to pause when we are in the background.
 */
static void gfx_directx_switch_out(void)
{
   _exit_gfx_critical();

   if (GFX_CRITICAL_RELEASED)
      thread_switch_out();

   _enter_gfx_critical();
}



/* gfx_directx_restore:
 *  Restores all the video and system bitmaps.
 */
void gfx_directx_restore(void)
{
   BMP_EXTRA_INFO *item = directx_bmp_list;

   if (!item)
      return;

   _enter_gfx_critical();

   while (item) {
      IDirectDrawSurface2_Restore(item->surf);
      item = item->next;
   }

   _exit_gfx_critical();
}



/* gfx_directx_lock:
 *  Locks the surface and prepares the lines array of the bitmap.
 */
void gfx_directx_lock(BITMAP *bmp)
{
   LPDIRECTDRAWSURFACE2 surf;
   BMP_EXTRA_INFO *bmp_extra;
   BITMAP *parent;
   HRESULT hr;
   DDSURFACEDESC surf_desc;
   int pitch;
   unsigned char *data;
   int y;

   if (bmp->id & BMP_ID_SUB) {
      /* if it's a sub-bitmap, start by locking our parent */
      parent = (BITMAP *)bmp->extra;
      gfx_directx_lock(parent);
      bmp->id |= BMP_ID_LOCKED;

      /* update the line array if our parent has moved */
      pitch = (long)parent->line[1] - (long)parent->line[0];
      data = parent->line[0] +
             (bmp->y_ofs - parent->y_ofs) * pitch +
             (bmp->x_ofs - parent->x_ofs) * BYTES_PER_PIXEL(bitmap_color_depth(bmp));

      if (data != bmp->line[0]) {
	 for (y = 0; y < bmp->h; y++) {
	    bmp->line[y] = data;
	    data += pitch;
	 }
      }
   }
   else {
      /* require exclusive ownership of the bitmap */
      _enter_gfx_critical();

      /* handle display switch */
      if (!app_foreground)
         gfx_directx_switch_out();

      /* this is a real bitmap, so can be locked directly */
      bmp_extra = BMP_EXTRA(bmp);
      bmp_extra->lock_nesting++;

      if (!(bmp->id & BMP_ID_LOCKED)) {
	 bmp->id |= BMP_ID_LOCKED;
	 bmp_extra->flags &= ~BMP_FLAG_LOST;

	 /* try to lock surface */
	 surf = bmp_extra->surf;

	 surf_desc.dwSize = sizeof(surf_desc);
	 surf_desc.dwFlags = 0;

	 hr = IDirectDrawSurface2_Lock(surf, NULL, &surf_desc,
                                       DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL); 

	 if (FAILED(hr)) {
	    /* lock failed, use pseudo surface memory */
	    bmp_extra->flags |= BMP_FLAG_LOST;
	    data = pseudo_surf_mem;
	    pitch = 0;
	 } 
	 else {
	    data = surf_desc.lpSurface;
	    pitch = surf_desc.lPitch;
	 }

	 /* prepare line array */
	 if (data != bmp->line[0]) {
	    for (y = 0; y < bmp->h; y++) {
	       bmp->line[y] = data;
	       data += pitch;
	    }
         }
      }
   }
}



/* gfx_directx_autolock:
 *  Locks the surface and prepares the lines array of the bitmap.
 *  This version is used directly by the bank switch functions, ie.
 *  it handles the autolocking mode, rather than being called directly.
 */
void gfx_directx_autolock(BITMAP *bmp)
{
   BMP_EXTRA_INFO *bmp_extra;
   BITMAP *parent;
   int pitch;
   unsigned char *data;
   int y;

   if (bmp->id & BMP_ID_SUB) {
      /* if it's a sub-bitmap, start by locking our parent */
      parent = (BITMAP *)bmp->extra;
      gfx_directx_autolock(parent);
      bmp->id |= BMP_ID_LOCKED;
      if (parent->id & BMP_ID_AUTOLOCK) {
	 bmp->id |= BMP_ID_AUTOLOCK;
	 parent->id &= ~BMP_ID_AUTOLOCK;
      }

      /* update the line array if our parent has moved */
      pitch = (long)parent->line[1] - (long)parent->line[0];
      data = parent->line[0] +
             (bmp->y_ofs - parent->y_ofs) * pitch +
             (bmp->x_ofs - parent->x_ofs) * BYTES_PER_PIXEL(bitmap_color_depth(bmp));

      if (data != bmp->line[0]) {
	 for (y = 0; y < bmp->h; y++) {
	    bmp->line[y] = data;
	    data += pitch;
	 }
      }
   }
   else {
      /* this is a real bitmap, so can be locked directly */
      bmp_extra = BMP_EXTRA(bmp);

      if (bmp_extra->lock_nesting) {
	 /* re-locking after a hwaccel, so don't change nesting state */
	 gfx_directx_lock(bmp);
	 bmp_extra->lock_nesting--;
         _exit_gfx_critical();
      }
      else {
	 /* locking for the first time */
	 gfx_directx_lock(bmp);
	 bmp->id |= BMP_ID_AUTOLOCK;
      }
   }
}



/* gfx_directx_unlock:
 *  Unlocks the surface.
 */
void gfx_directx_unlock(BITMAP *bmp)
{
   BMP_EXTRA_INFO *bmp_extra;
   BITMAP *parent;

   if (bmp->id & BMP_ID_SUB) {
      /* recurse when unlocking sub-bitmaps */
      parent = (BITMAP *)bmp->extra;
      gfx_directx_unlock(parent);
      if (!(parent->id & BMP_ID_LOCKED))
	 bmp->id &= ~BMP_ID_LOCKED;
   }
   else {
      /* regular bitmaps can be unlocked directly */
      bmp_extra = BMP_EXTRA(bmp);

      if (bmp_extra->lock_nesting > 0) {
	 bmp_extra->lock_nesting--;

         if ((!bmp_extra->lock_nesting) && (bmp->id & BMP_ID_LOCKED)) {
            if (!(bmp_extra->flags & BMP_FLAG_LOST)) {
	       /* only unlock if it doesn't use pseudo video memory */
	       IDirectDrawSurface2_Unlock(bmp_extra->surf, NULL);
            }

            bmp->id &= ~BMP_ID_LOCKED;
         }

         /* release bitmap for other threads */
         _exit_gfx_critical();
      }
   }
}



/* gfx_directx_release_lock:
 *  Releases the surface lock, for hardware accelerated drawing.
 */
void gfx_directx_release_lock(BITMAP *bmp)
{
   BMP_EXTRA_INFO *bmp_extra;

   /* handle display switch */
   if (!app_foreground)
      gfx_directx_switch_out();

   /* find parent */
   while (bmp->id & BMP_ID_SUB) {
      bmp->id &= ~BMP_ID_LOCKED;
      bmp = (BITMAP *)bmp->extra;
   }

   if (bmp->id & BMP_ID_LOCKED) {
      bmp_extra = (BMP_EXTRA_INFO *)bmp->extra;

      if (!(bmp_extra->flags & BMP_FLAG_LOST)) {
	 /* only unlock if it doesn't use pseudo video memory */
	 IDirectDrawSurface2_Unlock(bmp_extra->surf, NULL);
      }

      bmp->id &= ~BMP_ID_LOCKED;
   }
}
