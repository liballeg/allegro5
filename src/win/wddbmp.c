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
 *      DirectDraw bitmap management routines.
 *
 *      By Stefan Schimanski.
 *
 *      Improved page flipping mechanism by Robin Burrows.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"


static LPDIRECTDRAWSURFACE2 primbuffersurf = NULL;
static LPDIRECTDRAWSURFACE2 backbuffersurf = NULL;
static LPDIRECTDRAWSURFACE2 tripbuffersurf = NULL;


/* we may have to recycle the screen surface as a video bitmap,
 * in order to be consistent with how other platforms behave
 */
static int reused_screen = 0;



/* get_surface2_int:
 *  Helper function for getting the DirectDrawSurface2 interface
 *  from a DirectDrawSurface object.
 */
static LPDIRECTDRAWSURFACE2 get_surface2_int(LPDIRECTDRAWSURFACE surf)
{
   LPDIRECTDRAWSURFACE2 surf2;
   HRESULT hr;

   hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirectDrawSurface2, (LPVOID *)&surf2);

   /* There is a bug in the COM part of DirectX 3:
    *  if we release the DirectSurface interface, the actual
    *  object is also released. It is fixed in DirectX 5.
    */
   if (_dx_ver >= 0x500)
      IDirectDrawSurface_Release(surf);

   if (FAILED(hr))
      return NULL;
   else
      return surf2;
} 



/* gfx_directx_create_surface:
 *  Creates a DirectDraw surface.
 */ 
LPDIRECTDRAWSURFACE2 gfx_directx_create_surface(int w, int h, LPDDPIXELFORMAT pixel_format, int type)
{
   DDSURFACEDESC surf_desc;
   LPDIRECTDRAWSURFACE _surf1;
   LPDIRECTDRAWSURFACE2 surf;
   DDSCAPS ddscaps;
   HRESULT hr;

   /* describe surface characteristics */
   memset (&surf_desc, 0, sizeof(DDSURFACEDESC));
   surf_desc.dwSize = sizeof(surf_desc);
   surf_desc.dwFlags = DDSD_CAPS;

   switch (type) {

      case SURF_PRIMARY_COMPLEX:
         surf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
         surf_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
         surf_desc.dwBackBufferCount = 2;
         break;

      case SURF_PRIMARY_SINGLE:
         surf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
         break;

      case SURF_OVERLAY:
         surf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
         surf_desc.dwFlags |= DDSD_BACKBUFFERCOUNT | DDSD_HEIGHT | DDSD_WIDTH;
         surf_desc.dwBackBufferCount = 2;
         surf_desc.dwHeight = h;
         surf_desc.dwWidth = w;

         if (pixel_format) {    /* use pixel format */
            surf_desc.dwFlags |= DDSD_PIXELFORMAT;
            surf_desc.ddpfPixelFormat = *pixel_format;
         }
         break;

      case SURF_VIDEO:
         surf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN;
         surf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         surf_desc.dwHeight = h;
         surf_desc.dwWidth = w;
         break;

      case SURF_SYSTEM:
         surf_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
         surf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         surf_desc.dwHeight = h;
         surf_desc.dwWidth = w;

         if (pixel_format) {    /* use pixel format */
            surf_desc.dwFlags |= DDSD_PIXELFORMAT;
            surf_desc.ddpfPixelFormat = *pixel_format;
         }
         break;

      default:
         _TRACE("Unknown surface type\n");
         return NULL;
   }

   /* create the surface with this properties */
   hr = IDirectDraw2_CreateSurface(directdraw, &surf_desc, &_surf1, NULL);

   if (FAILED(hr)) {
      if ((type == SURF_PRIMARY_COMPLEX) || (type == SURF_OVERLAY)) {
         /* lower the number of backbuffers */
         surf_desc.dwBackBufferCount = 1;
         hr = IDirectDraw2_CreateSurface(directdraw, &surf_desc, &_surf1, NULL);

         if (FAILED(hr)) {
            /* no backbuffer any more */
            surf_desc.dwBackBufferCount = 0;
            surf_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
            surf_desc.ddsCaps.dwCaps &= ~(DDSCAPS_FLIP | DDSCAPS_COMPLEX);

            hr = IDirectDraw2_CreateSurface(directdraw, &surf_desc, &_surf1, NULL);
            if (FAILED(hr))
               return NULL;
         }
      }
      else
         /* no other way for normal surfaces */
         return NULL;
   }

   /* create the IDirectDrawSurface2 interface */
   surf = get_surface2_int(_surf1);
   if (surf == NULL)
      return NULL;

   /* get attached backbuffers */
   if (surf_desc.dwBackBufferCount > 0) {
      ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
      IDirectDrawSurface2_GetAttachedSurface(surf, &ddscaps, &backbuffersurf);

      if (surf_desc.dwBackBufferCount > 1) {
         ddscaps.dwCaps = DDSCAPS_FLIP;
         IDirectDrawSurface2_GetAttachedSurface(backbuffersurf, &ddscaps, &tripbuffersurf);
      }

      primbuffersurf = surf;
   }

   return surf;
}



/* gfx_directx_destroy_surf:
 *  Destroys a DirectDraw surface.
 */
void gfx_directx_destroy_surf(LPDIRECTDRAWSURFACE2 surf)
{
   if (surf) {
      IDirectDrawSurface2_Release(surf);

      if (surf == primbuffersurf) {
         primbuffersurf = NULL;
         backbuffersurf = NULL;
         tripbuffersurf = NULL;

         reused_screen = 0;
      }
   }
}



/* _make_video_bitmap:
 *  Helper function for wrapping up video memory in a video bitmap.
 */
static BITMAP *_make_video_bitmap(int w, int h, unsigned long addr, struct GFX_VTABLE *vtable, int bpl)
{
   int i, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *) malloc(size);
   if (!b)
      return NULL;

   b->w = b->cr = w;
   b->h = b->cb = h;
   b->clip = TRUE;
   b->cl = b->ct = 0;
   b->vtable = vtable;
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->id = BMP_ID_VIDEO;
   b->extra = NULL;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _video_ds();

   b->line[0] = (char *)addr;

   for (i = 1; i < h; i++)
      b->line[i] = b->line[i - 1] + bpl;

   return b;
}



/* make_directx_bitmap:
 *  Connects a DirectDraw surface with an Allegro bitmap.
 */
BITMAP *make_directx_bitmap(LPDIRECTDRAWSURFACE2 surf, int w, int h, int id)
{
   struct BITMAP *bmp;

   /* create Allegro bitmap */
   bmp = _make_video_bitmap(w, h, (unsigned long)pseudo_surf_mem, &_screen_vtable, 0);
   if (!bmp)
      return NULL;

   bmp->id = id;
   bmp->write_bank = gfx_directx_write_bank;
   bmp->read_bank = gfx_directx_write_bank;

   /* setup surface info structure to store additional information */
   bmp->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
   if (!bmp->extra)
      return NULL;

   BMP_EXTRA(bmp)->surf = surf;
   BMP_EXTRA(bmp)->next = NULL;
   BMP_EXTRA(bmp)->prev = NULL;
   BMP_EXTRA(bmp)->flags = 0;
   BMP_EXTRA(bmp)->lock_nesting = 0;

   register_directx_bitmap(bmp);

   return bmp;
}



/* destroy_directx_bitmap:
 *  Destroys a video or system bitmap.
 */
void destroy_directx_bitmap(BITMAP *bmp)
{
   if (bmp) {
      unregister_directx_bitmap(bmp);
      free(bmp->extra);
      free(bmp);
   }
}



/* gfx_directx_created_sub_bitmap:
 */
void gfx_directx_created_sub_bitmap(BITMAP *bmp, BITMAP *parent)
{
   bmp->extra = parent;
}



/* gfx_directx_create_video_bitmap:
 */
BITMAP *gfx_directx_create_video_bitmap(int width, int height)
{
   LPDIRECTDRAWSURFACE2 surf;
   struct BITMAP *bmp;

   /* can we reuse the screen bitmap for this? */
   if ((screen->w == width) && (screen->h == height)) {
      switch (reused_screen) {

         case 0:  /* return the screen */
            reused_screen++;
            return screen;

         case 1:  /* return the first backbuffer if any */
            if (backbuffersurf) {
               bmp = make_directx_bitmap(backbuffersurf, width, height, BMP_ID_VIDEO);
               if (bmp) {
                  reused_screen++;
                  return bmp;
               }
            }
            break;

         case 2:  /* return the second backbuffer if any */
            if (tripbuffersurf) {
               bmp = make_directx_bitmap(tripbuffersurf, width, height, BMP_ID_VIDEO);
               if (bmp) {
                  reused_screen++;
                  return bmp;
               }
            }
            break;
      }
   }

   /* assume all flip surfaces have been allocated, so free unused */
   if (reused_screen < 3 && tripbuffersurf) {
      gfx_directx_destroy_surf(tripbuffersurf);
      tripbuffersurf = NULL;
   }

   if (reused_screen < 2 && backbuffersurf) {
      gfx_directx_destroy_surf(backbuffersurf);
      backbuffersurf = NULL;
   }

   /* create DirectDraw surface */
   if (dd_pixelformat)
      surf = gfx_directx_create_surface(width, height, dd_pixelformat, SURF_SYSTEM);
   else
      surf = gfx_directx_create_surface(width, height, NULL, SURF_VIDEO);

   if (!surf)
      return NULL;

   /* create Allegro bitmap for surface */
   return make_directx_bitmap(surf, width, height, BMP_ID_VIDEO);
}



/* gfx_directx_destroy_video_bitmap:
 */
void gfx_directx_destroy_video_bitmap(BITMAP *bmp)
{
   if (bmp == screen) {
      reused_screen--;
      return;
   }

   if (bmp == dd_frontbuffer) {
      /* in this case, 'bmp' points to the visible contents
       * but 'screen' doesn't, so we first invert that
       */
      gfx_directx_show_video_bitmap(screen);
   }

   if ((BMP_EXTRA(bmp)->surf == backbuffersurf) || (BMP_EXTRA(bmp)->surf == tripbuffersurf)) {
      /* don't destroy the surface belonging to the flip chain */
      reused_screen --;
   }
   else {
      /* destroy the surface */
      gfx_directx_destroy_surf(BMP_EXTRA(bmp)->surf);
   }

   destroy_directx_bitmap(bmp);
}



/* flip_directx_bitmap:
 *  Worker function for DirectDraw page flipping.
 */
static int flip_directx_bitmap(BITMAP *bmp, int wait)
{
   LPDIRECTDRAWSURFACE2 visible;
   LPDIRECTDRAWSURFACE2 invisible;
   HRESULT hr;

   /* flip only in the foreground */
   if (!app_foreground) {
      thread_switch_out();
      return 0;
   }

   /* retrieve underlying DirectDraw surfaces */
   visible = BMP_EXTRA(dd_frontbuffer)->surf;
   invisible = BMP_EXTRA(bmp)->surf;

   if (visible == invisible)
      return 0;

   if ((invisible == backbuffersurf) || (invisible == tripbuffersurf)) {
      /* flip already attached surfaces:
       *  - visible is guaranteed to be primbuffersurf,
       *  - invisible is either backbuffersurf or tripbuffersurf
       */
      hr = IDirectDrawSurface2_Flip(visible, invisible, wait ? DDFLIP_WAIT : 0);

      /* If the surface has been lost, try to restore all surfaces
       * and, on success, try again to flip the surfaces.
       */
      if (hr == DDERR_SURFACELOST) {
         if (gfx_directx_restore() == 0)
            hr = IDirectDrawSurface2_Flip(visible, invisible, wait ? DDFLIP_WAIT : 0);
      }
   }
   else {
      /* attach invisible surface to visible one */
      hr = IDirectDrawSurface2_AddAttachedSurface(visible, invisible);

      /* If the surface has been lost, try to restore all surfaces
       * and, on success, try again to attach the surface.
       */
      if (hr == DDERR_SURFACELOST) {
         if (gfx_directx_restore() == 0)
            hr = IDirectDrawSurface2_AddAttachedSurface(visible, invisible);
      }

      if (FAILED(hr)) {
         _TRACE("Can't attach surface (%x)\n", hr);
         return -1;
      }

      /* flip the surfaces */
      hr = IDirectDrawSurface2_Flip(visible, invisible, wait ? DDFLIP_WAIT : 0);

      /* If the surface has been lost, try to restore all surfaces
       * and, on success, try again to flip the surfaces.
       */
      if (hr == DDERR_SURFACELOST) {
         if (gfx_directx_restore() == 0)
            hr = IDirectDrawSurface2_Flip(visible, invisible, wait ? DDFLIP_WAIT : 0);
      }

      /* detach invisible surface */
      IDirectDrawSurface2_DeleteAttachedSurface(visible, 0, invisible);
   }

   if (FAILED(hr)) {
      _TRACE("Can't flip (%x)\n", hr);
      return -1;
   }
   else {
      /* the contents of 'bmp' formerly pointed to by 'invisible' are now
       * pointed to by 'visible', so we need to link 'bmp' to 'visible'
       */
      BMP_EXTRA(bmp)->surf = visible;

      /* link formerly forefront bitmap to the invisible surface
       * (since this bitmap may be a copy of 'screen', this latter bitmap
       *  is not guaranteed to point to the visible contents any longer)
       */
      BMP_EXTRA(dd_frontbuffer)->surf = invisible;

      /* dd_frontbuffer must keep track of the forefront bitmap */
      dd_frontbuffer = bmp;  /* hence BMP_EXTRA(dd_frontbuffer)->surf == visible */

      return 0;
   }
}



/* gfx_directx_show_video_bitmap:
 */
int gfx_directx_show_video_bitmap(BITMAP *bmp)
{
   return flip_directx_bitmap(bmp, TRUE);
}



/* gfx_directx_request_video_bitmap:
 */
int gfx_directx_request_video_bitmap(BITMAP *bmp)
{
   return flip_directx_bitmap(bmp, FALSE);
}



/* gfx_directx_poll_scroll:
 */
int gfx_directx_poll_scroll(void)
{
   HRESULT hr;

   hr = IDirectDrawSurface2_GetFlipStatus(BMP_EXTRA(dd_frontbuffer)->surf, DDGFS_ISFLIPDONE);

   /* If the surface has been lost, try to restore all surfaces
    * and, on success, try again to get flip status.
    */
   if (hr == DDERR_SURFACELOST) {
      if (gfx_directx_restore() == 0)
         hr = IDirectDrawSurface2_GetFlipStatus(BMP_EXTRA(dd_frontbuffer)->surf, DDGFS_ISFLIPDONE);
   }

   if (FAILED(hr))
      return -1;
   else
      return 0;
}



/* gfx_directx_create_system_bitmap:
 */
BITMAP *gfx_directx_create_system_bitmap(int width, int height)
{
   LPDIRECTDRAWSURFACE2 surf;

   /* create DirectDraw surface */
   surf = gfx_directx_create_surface(width, height, dd_pixelformat, SURF_SYSTEM);
   if (!surf)
      return NULL;

   /* create Allegro bitmap for surface */
   return make_directx_bitmap(surf, width, height, BMP_ID_SYSTEM);
}



/* gfx_directx_destroy_system_bitmap:
 */
void gfx_directx_destroy_system_bitmap(BITMAP *bmp)
{
   gfx_directx_destroy_surf(BMP_EXTRA(bmp)->surf);
   destroy_directx_bitmap(bmp);
}



/* win_get_dc: (WinAPI)
 *  Returns device context of a video or system bitmap.
 */
HDC win_get_dc(BITMAP *bmp)
{
   LPDIRECTDRAWSURFACE2 surf;
   HDC dc;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         surf = BMP_EXTRA(bmp)->surf;
         hr = IDirectDrawSurface2_GetDC(surf, &dc);

         /* If the surface has been lost, try to restore all surfaces
          * and, on success, try again to get the DC.
          */
         if (hr == DDERR_SURFACELOST) {
            if (gfx_directx_restore() == 0)
               hr = IDirectDrawSurface2_GetDC(surf, &dc);
         }

         if (hr == DD_OK)
            return dc;
      }
   }

   return NULL;
}



/* win_release_dc: (WinAPI)
 *  Releases device context of a video or system bitmap.
 */
void win_release_dc(BITMAP *bmp, HDC dc)
{
   LPDIRECTDRAWSURFACE2 surf;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         surf = BMP_EXTRA(bmp)->surf;
         hr = IDirectDrawSurface2_ReleaseDC(surf, dc);

         /* If the surface has been lost, try to restore all surfaces
          * and, on success, try again to release the DC.
          */
         if (hr == DDERR_SURFACELOST) {
            if (gfx_directx_restore() == 0)
               hr = IDirectDrawSurface2_ReleaseDC(surf, dc);
         }
      }
   }
}
