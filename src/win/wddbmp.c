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


static LPDIRECTDRAWSURFACE2 backbuffer_surf = NULL;
static LPDIRECTDRAWSURFACE2 tripbuffer_surf = NULL;
static int flipping_pages = 0;


/* gfx_directx_create_surface:
 *  Creates a DirectDraw surface.
 */ 
LPDIRECTDRAWSURFACE2 gfx_directx_create_surface(int w, int h, LPDDPIXELFORMAT pixel_format, int type)
{
   DDSURFACEDESC surf_desc;
   LPDIRECTDRAWSURFACE surf1;
   LPDIRECTDRAWSURFACE2 surf2;
   DDSCAPS ddscaps;
   HRESULT hr;

   /* describe surface characteristics */
   memset(&surf_desc, 0, sizeof(DDSURFACEDESC));
   surf_desc.dwSize = sizeof(surf_desc);
   surf_desc.dwFlags = DDSD_CAPS;

   switch (type) {

      case SURF_PRIMARY:
         surf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
         break;

      case SURF_OVERLAY:
         surf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY;
         surf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
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
   hr = IDirectDraw2_CreateSurface(directdraw, &surf_desc, &surf1, NULL);

   if (FAILED(hr)) {
      _TRACE("Unable to create the surface\n");
      return NULL;
   }

   /*  retrieve the DirectDrawSurface2 interface */
   hr = IDirectDrawSurface_QueryInterface(surf1, &IID_IDirectDrawSurface2, (LPVOID *)&surf2);

   /* there is a bug in the COM part of DirectX 3:
    *  If we release the DirectSurface interface, the actual
    *  object is also released. It is fixed in DirectX 5.
    */
   if (_dx_ver >= 0x500)
      IDirectDrawSurface_Release(surf1);

   if (FAILED(hr)) {
      _TRACE("Unable to retrieve the DirectDrawSurface2 interface\n");
      return NULL;
   }

   return surf2;
}



/* gfx_directx_destroy_surf:
 *  Destroys a DirectDraw surface.
 */
void gfx_directx_destroy_surf(LPDIRECTDRAWSURFACE2 surf)
{
   if (surf)
      IDirectDrawSurface2_Release(surf);
}



/* make_video_bitmap:
 *  Helper function for wrapping up video memory in a video bitmap.
 */
static BITMAP *make_video_bitmap(int w, int h, unsigned long addr, struct GFX_VTABLE *vtable, int bpl)
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
   bmp = make_video_bitmap(w, h, (unsigned long)pseudo_surf_mem, &_screen_vtable, 0);
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
   HRESULT hr;

   /* try to detect page flipping and triple buffering patterns */
   if ((width == dd_frontbuffer->w) && (height == dd_frontbuffer->h)) {

      switch (flipping_pages) {

         case 0:
            /* recycle the frontbuffer as a video bitmap */
            flipping_pages++;
            return dd_frontbuffer;

         case 1:
            /* create a backbuffer and attach it to the frontbuffer */
            backbuffer_surf = gfx_directx_create_surface(width, height, NULL, SURF_VIDEO);
            if (backbuffer_surf) {
               hr = IDirectDrawSurface2_AddAttachedSurface(BMP_EXTRA(dd_frontbuffer)->surf, backbuffer_surf);
               if (hr == DD_OK) {
                  bmp = make_directx_bitmap(backbuffer_surf, width, height, BMP_ID_VIDEO);
                  if (bmp) {
                     flipping_pages++;
                     return bmp;
                  }

                  IDirectDrawSurface2_DeleteAttachedSurface(BMP_EXTRA(dd_frontbuffer)->surf, 0, backbuffer_surf);
               }

               gfx_directx_destroy_surf(backbuffer_surf);
            }
            return NULL;

        case 2:
            /* create a third buffer and attach it to the backbuffer */
            tripbuffer_surf = gfx_directx_create_surface(width, height, NULL, SURF_VIDEO);
            if (tripbuffer_surf) {
               hr = IDirectDrawSurface2_AddAttachedSurface(backbuffer_surf, tripbuffer_surf);
               if (hr == DD_OK) {
                  bmp = make_directx_bitmap(tripbuffer_surf, width, height, BMP_ID_VIDEO);
                  if (bmp) {
                     flipping_pages++;
                     return bmp;
                  }

                  IDirectDrawSurface2_DeleteAttachedSurface(backbuffer_surf, 0, tripbuffer_surf);
               }

               gfx_directx_destroy_surf(tripbuffer_surf);
            }
            return NULL;
      }
   }

   /* create the DirectDraw surface */
   if (dd_pixelformat)
      surf = gfx_directx_create_surface(width, height, dd_pixelformat, SURF_SYSTEM);
   else
      surf = gfx_directx_create_surface(width, height, NULL, SURF_VIDEO);

   if (!surf)
      return NULL;

   /* create the bitmap that wraps up the surface */
   bmp = make_directx_bitmap(surf, width, height, BMP_ID_VIDEO);
   if (!bmp) {
      gfx_directx_destroy_surf(surf);
      return NULL;
   }

   return bmp;
}



/* gfx_directx_destroy_video_bitmap:
 */
void gfx_directx_destroy_video_bitmap(BITMAP *bmp)
{
   if ((bmp == screen) || (bmp == dd_frontbuffer)) {
      /* makes sure that screen == dd_frontbuffer */
      gfx_directx_show_video_bitmap(screen);
   }

   if (BMP_EXTRA(bmp)->surf == tripbuffer_surf) {
      if (backbuffer_surf)
         IDirectDrawSurface2_DeleteAttachedSurface(backbuffer_surf, 0, tripbuffer_surf);
      tripbuffer_surf = NULL;
      flipping_pages--;
   }
   else if (BMP_EXTRA(bmp)->surf == backbuffer_surf) {
      IDirectDrawSurface2_DeleteAttachedSurface(BMP_EXTRA(dd_frontbuffer)->surf, 0, backbuffer_surf);
      backbuffer_surf = NULL;
      flipping_pages--;
   }
   else if (bmp == dd_frontbuffer) {
      flipping_pages--;
      return;  /* don't destroy the frontbuffer! */
   }

   /* destroy the surface */
   gfx_directx_destroy_surf(BMP_EXTRA(bmp)->surf);

   /* destroy the wrapper */
   destroy_directx_bitmap(bmp);
}



/* flip_directx_bitmap:
 *  Worker function for DirectDraw page flipping.
 */
static int flip_directx_bitmap(BITMAP *bmp, int wait)
{
   LPDIRECTDRAWSURFACE2 surf;
   HRESULT hr;

   /* flip only in the foreground */
   if (!app_foreground) {
      thread_switch_out();
      return 0;
   }

   if (bmp == dd_frontbuffer)
      return 0;

   /* retrieve the underlying DirectDraw surface */
   surf = BMP_EXTRA(bmp)->surf;

   ASSERT((surf == backbuffer_surf) || (surf == tripbuffer_surf));

   /* flip the contents of the surfaces */
   hr = IDirectDrawSurface2_Flip(BMP_EXTRA(dd_frontbuffer)->surf, surf, wait ? DDFLIP_WAIT : 0);

   /* if the surface has been lost, try to restore all surfaces */
   if (hr == DDERR_SURFACELOST) {
      if (gfx_directx_restore() == 0)
         hr = IDirectDrawSurface2_Flip(BMP_EXTRA(dd_frontbuffer)->surf, surf, wait ? DDFLIP_WAIT : 0);
   }

   if (FAILED(hr)) {
      _TRACE("Can't flip (%x)\n", hr);
      return -1;
   }

   /* link newly forefront bitmap to the frontbuffer surface:
    * The contents of 'bmp' formerly pointed to by 'surf' are now pointed to
    * by 'frontbuffer_surf', so we need to link 'bmp' to 'frontbuffer_surf'.
    */
   BMP_EXTRA(bmp)->surf = BMP_EXTRA(dd_frontbuffer)->surf;

   /* link formerly forefront bitmap to the newly hidden surface:
    *  Since this bitmap may be a copy of 'screen', this latter bitmap
    *  is not guaranteed to point to the visible contents any longer.
    */
   BMP_EXTRA(dd_frontbuffer)->surf = surf;

   /* keep track of the forefront bitmap */
   dd_frontbuffer = bmp;

   return 0;
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

   ASSERT(flipping_pages == 3);

   hr = IDirectDrawSurface2_GetFlipStatus(BMP_EXTRA(dd_frontbuffer)->surf, DDGFS_ISFLIPDONE);

   /* if the surface has been lost, try to restore all surfaces */
   if (hr == DDERR_SURFACELOST) {
      if (gfx_directx_restore() == 0)
         hr = IDirectDrawSurface2_GetFlipStatus(BMP_EXTRA(dd_frontbuffer)->surf, DDGFS_ISFLIPDONE);
   }

   if (FAILED(hr))
      return -1;

   return 0;
}



/* gfx_directx_create_system_bitmap:
 */
BITMAP *gfx_directx_create_system_bitmap(int width, int height)
{
   LPDIRECTDRAWSURFACE2 surf;
   BITMAP *bmp;

   /* create the DirectDraw surface */
   surf = gfx_directx_create_surface(width, height, dd_pixelformat, SURF_SYSTEM);
   if (!surf)
      return NULL;

   /* create the bitmap that wraps up the surface */
   bmp = make_directx_bitmap(surf, width, height, BMP_ID_SYSTEM);
   if (!bmp) {
      gfx_directx_destroy_surf(surf);
      return NULL;
   }

   return bmp;
}



/* gfx_directx_destroy_system_bitmap:
 */
void gfx_directx_destroy_system_bitmap(BITMAP *bmp)
{
   /* destroy the surface */
   gfx_directx_destroy_surf(BMP_EXTRA(bmp)->surf);

   /* destroy the wrapper */
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
