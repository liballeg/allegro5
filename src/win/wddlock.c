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



/* gfx_directx_switch_out:
 *  Arranges for drawing requests to pause when we are in the background.
 */
static void gfx_directx_switch_out(void)
{ 
   _exit_gfx_critical();
   thread_switch_out();
   _enter_gfx_critical();
}



/* gfx_switch_out:
 *  Handles switching away from the graphics subsystem.
 */
void gfx_switch_out(void)
{
   if (gfx_driver) {
      if (gfx_driver->id == GFX_DIRECTX_WIN) {
	 if (get_display_switch_mode() == SWITCH_PAUSE)
	    wddwin_switch_out();
      }
      else if (gfx_driver->id == GFX_DIRECTX_OVL)
	 wddovl_switch_in();
   }
}



/* gfx_switch_in:
 *  Handles switching back to the graphics subsystem.
 */
void gfx_switch_in(void)
{
   if (gfx_driver) {
      if (gfx_driver->id == GFX_DIRECTX_WIN) {
	 if (get_display_switch_mode() == SWITCH_PAUSE)
	    wddwin_switch_in();
      }
      else if (gfx_driver->id == GFX_DIRECTX_OVL)
	 wddovl_switch_in();
   }
}



/* gfx_directx_lock:
 *  Locks the surface and prepares the lines array of the bitmap.
 */
void gfx_directx_lock(BITMAP *bmp)
{
   LPDIRECTDRAWSURFACE surf;
   BMP_EXTRA_INFO *bmp_extra;
   BMP_EXTRA_INFO *item;
   BITMAP *parent;
   HRESULT hr;
   DDSURFACEDESC surf_desc;
   int pitch;
   char *data;
   int y, h;

   /* pause other threads */
   _enter_gfx_critical();

   /* handle display switch */
   if (!app_foreground)
      gfx_directx_switch_out();

   if (bmp->id & BMP_ID_SUB) {
      /* if it's a sub-bitmap, start by locking our parent */
      parent = (BITMAP *)bmp->extra;
      gfx_directx_lock(parent);
      bmp->id |= BMP_ID_LOCKED;

      /* update the line array if our parent has moved */
      pitch = (long)parent->line[1] - (long)parent->line[0];
      data = parent->line[0] + pitch * bmp->y_ofs + bmp->x_ofs * ((_color_depth+7) >> 3);
      h = bmp->h;

      if (data != bmp->line[0]) {
	 for (y = 0; y < h; y++) {
	    bmp->line[y] = data;
	    data += pitch;
	 }
      }
   }
   else {
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

	 hr = IDirectDrawSurface_Lock(surf, NULL, &surf_desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL); 

	 if (FAILED(hr)) {
	    /* lost bitmap, try to restore all surfaces */
	    item = directx_bmp_list;
	    while (item) {
	       /* if restoration fails, stop restoring */
	       if (FAILED(IDirectDrawSurface_Restore(item->surf)))
		  break;
	       item = item->next;
	    }

	    /* try again to lock */
	    surf_desc.dwSize = sizeof(surf_desc);
	    surf_desc.dwFlags = 0;

	    hr = IDirectDrawSurface_Lock(surf, NULL, &surf_desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL); 

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
	 } 
	 else {
	    data = surf_desc.lpSurface;
	    pitch = surf_desc.lPitch;
	 }

	 /* prepare line array */
	 h = bmp->h;

	 if (data != bmp->line[0]) {
	    for (y = 0; y < h; y++) {
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
static void gfx_directx_autolock(BITMAP *bmp)
{
   BMP_EXTRA_INFO *bmp_extra;
   BITMAP *parent;
   int pitch;
   char *data;
   int y, h;

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
      data = parent->line[0] + pitch * bmp->y_ofs + bmp->x_ofs * ((_color_depth+7) >> 3);
      h = bmp->h;

      if (data != bmp->line[0]) {
	 for (y = 0; y < h; y++) {
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
	 _exit_gfx_critical();
	 gfx_directx_lock(bmp);
	 bmp_extra->lock_nesting--;
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

      if (bmp_extra->lock_nesting > 0)
	 bmp_extra->lock_nesting--;

      if ((!bmp_extra->lock_nesting) && (bmp->id & BMP_ID_LOCKED)) {
	 if (!(bmp_extra->flags & BMP_FLAG_LOST)) {
	    /* only unlock if it doesn't use pseudo video memory */
	    IDirectDrawSurface_Unlock(bmp_extra->surf, bmp->line[0]);
	 }

	 bmp->id &= ~BMP_ID_LOCKED;
      }
   }

   /* release bitmap for other threads */
   _exit_gfx_critical();
}



/* gfx_directx_release_lock:
 *  Temporarily releases the surface lock, for hardware accelerated drawing.
 */
void gfx_directx_release_lock(BITMAP *bmp)
{
   BMP_EXTRA_INFO *bmp_extra;

   /* find parent */
   while (bmp->id & BMP_ID_SUB) {
      bmp->id &= ~BMP_ID_LOCKED;
      bmp = (BITMAP *)bmp->extra;
   }

   if (bmp->id & BMP_ID_LOCKED) {
      bmp_extra = (BMP_EXTRA_INFO *)bmp->extra;

      if (!(bmp_extra->flags & BMP_FLAG_LOST)) {
	 /* only unlock if it doesn't use pseudo video memory */
	 IDirectDrawSurface_Unlock(bmp_extra->surf, bmp->line[0]);
      }

      bmp->id &= ~BMP_ID_LOCKED;
   }
}



#ifdef __MINGW32__


/* gcc-style locking code for the Mingw32 build, by Henrik Stokseth.
 *
 * Note from Shawn: I don't like having two versions of the same thing.
 * At some point it would be nice to move this into an external .s file,
 * so we can share the same code between MSVC and Mingw32 builds.
 */



static unsigned long gfx_directx_write_bank_internal(BITMAP *bitmap_p, unsigned long line)
{
   if (!(bitmap_p->id & BMP_ID_LOCKED)) {    /* check if locked already */
      gfx_directx_lock(bitmap_p);            /* lock the surface once */
      bitmap_p->id |= BMP_ID_AUTOLOCK;       /* set Autolock flag */
   }

   return ((unsigned long *)(bitmap_p->line))[line];
}



/* gfx_directx_bank_switch:
 *  edx = bitmap_p
 *  eax = line
 */
__asm__(".align 4 \n
.globl _gfx_directx_write_bank \n
	.def _gfx_directx_write_bank; .scl 2; .type 32; .endef \n
_gfx_directx_write_bank: \n
	pushl %ebp \n
	movl %esp, %ebp \n
	addl $-12, %esp \n
	pushl %ebx \n
	pushl %ecx \n

	pushl %eax \n
	pushl %edx \n

	call _gfx_directx_write_bank_internal \n

	addl $8, %esp \n

	popl %ecx \n
	popl %ebx \n
	addl $12, %esp \n
	leave \n
	ret");



static void gfx_directx_unwrite_bank_internal(BITMAP *bitmap_p)
{
   if(bitmap_p->id & BMP_ID_AUTOLOCK)  {     /* bitmap autolock flag set? */
      gfx_directx_unlock(bitmap_p); 
      bitmap_p->id &= ~BMP_ID_AUTOLOCK;      /* clear autolock flag */
   }
}



/* gfx_directx_unbank_switch:
 *  edx = bitmap_p
 */
__asm__(".align 4 \n
.globl _gfx_directx_unwrite_bank \n
	.def _gfx_directx_unwrite_bank; .scl 2; .type 32; .endef \n
_gfx_directx_unwrite_bank: \n
	pushl %ebp \n
	movl %esp, %ebp \n
	addl $-12, %esp \n
	pushl %eax \n
	pushl %ebx \n
	pushl %ecx \n

	pushl %edx \n

	call _gfx_directx_unwrite_bank_internal \n

	addl $4,%esp \n

	popl %ecx \n
	popl %ebx \n
	popl %eax \n
	addl $12, %esp \n
	leave \n
	ret");



#else          /* not Mingw32, so use MSVC style inline asm */



/* gfx_directx_bank_switch:
 *  edx = bitmap
 *  eax = line
 */
void gfx_directx_write_bank(void)
{
   _asm
   {
       ; check whether is is locked already
       test [edx]BITMAP.id, BMP_ID_LOCKED
       jnz Locked

       ; lock the surface
       pushad
       push edx
       call gfx_directx_autolock
       pop edx
       popad

   Locked:
       ; get pointer to the video memory
       mov eax, [edx+eax*4]BITMAP.line
   }
}



/* gfx_directx_unbank_switch:
 *  edx = bmp
 */
void gfx_directx_unwrite_bank(void)
{
   _asm
   {
       ; only unlock bitmaps that were autolocked
       test [edx]BITMAP.id, BMP_ID_AUTOLOCK
       jz NoUnlock

       ; unlock surface
       pushad
       push edx
       call gfx_directx_unlock
       pop edx
       popad

       ; clear the autolock flag
       and [edx]BITMAP.id, ~BMP_ID_AUTOLOCK

   NoUnlock:
   }
} 



#endif         /* MSVC vs. Mingw32 */


