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


static DDRAW_SURFACE *back_buffer = NULL;
static DDRAW_SURFACE *triple_buffer = NULL;
static int flipping_pages = 0;


/* gfx_directx_create_surface:
 *  Creates a DirectDraw surface.
 */ 
DDRAW_SURFACE *gfx_directx_create_surface(int w, int h, LPDDPIXELFORMAT pixel_format, int type)
{
   DDSURFACEDESC ddsurf_desc;
   LPDIRECTDRAWSURFACE ddsurf1;
   LPDIRECTDRAWSURFACE2 ddsurf2;
   DDSCAPS ddscaps;
   HRESULT hr;
   DDRAW_SURFACE *surf;

   /* describe surface characteristics */
   memset(&ddsurf_desc, 0, sizeof(DDSURFACEDESC));
   ddsurf_desc.dwSize = sizeof(DDSURFACEDESC);
   ddsurf_desc.dwFlags = DDSD_CAPS;

   switch (type) {

      case DDRAW_SURFACE_PRIMARY:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
         break;

      case DDRAW_SURFACE_OVERLAY:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY;
         ddsurf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         ddsurf_desc.dwHeight = h;
         ddsurf_desc.dwWidth = w;

         if (pixel_format) {    /* use pixel format */
            ddsurf_desc.dwFlags |= DDSD_PIXELFORMAT;
            ddsurf_desc.ddpfPixelFormat = *pixel_format;
         }
         break;

      case DDRAW_SURFACE_VIDEO:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN;
         ddsurf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         ddsurf_desc.dwHeight = h;
         ddsurf_desc.dwWidth = w;
         break;

      case DDRAW_SURFACE_SYSTEM:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
         ddsurf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         ddsurf_desc.dwHeight = h;
         ddsurf_desc.dwWidth = w;

         if (pixel_format) {    /* use pixel format */
            ddsurf_desc.dwFlags |= DDSD_PIXELFORMAT;
            ddsurf_desc.ddpfPixelFormat = *pixel_format;
         }
         break;

      default:
         _TRACE("Unknown surface type\n");
         return NULL;
   }

   /* create the surface with this properties */
   hr = IDirectDraw2_CreateSurface(directdraw, &ddsurf_desc, &ddsurf1, NULL);

   if (FAILED(hr)) {
      _TRACE("Unable to create the surface\n");
      return NULL;
   }

   /*  retrieve the DirectDrawSurface2 interface */
   hr = IDirectDrawSurface_QueryInterface(ddsurf1, &IID_IDirectDrawSurface2, (LPVOID *)&ddsurf2);

   /* there is a bug in the COM part of DirectX 3:
    *  If we release the DirectSurface interface, the actual
    *  object is also released. It is fixed in DirectX 5.
    */
   if (_dx_ver >= 0x500)
      IDirectDrawSurface_Release(ddsurf1);

   if (FAILED(hr)) {
      _TRACE("Unable to retrieve the DirectDrawSurface2 interface\n");
      return NULL;
   }

   /* setup surface info structure to store additional information */
   surf = malloc(sizeof(DDRAW_SURFACE));
   if (!surf) {
      IDirectDrawSurface2_Release(ddsurf2);
      return NULL;
   }

   surf->id = ddsurf2;
   surf->next = NULL;
   surf->prev = NULL;
   surf->flags = 0;
   surf->lock_nesting = 0;

   register_ddraw_surface(surf);

   return surf;
}



/* gfx_directx_destroy_surface:
 *  Destroys a DirectDraw surface.
 */
void gfx_directx_destroy_surface(DDRAW_SURFACE *surf)
{
   IDirectDrawSurface2_Release(surf->id);
   unregister_ddraw_surface(surf);
   free(surf);
}



/* make_bitmap_from_surface:
 *  Connects a DirectDraw surface with an Allegro bitmap.
 */
BITMAP *make_bitmap_from_surface(DDRAW_SURFACE *surf, int w, int h, int id)
{
   BITMAP *bmp;
   int i;

   bmp = (BITMAP *) malloc(sizeof(BITMAP) + sizeof(char *) * h);
   if (!bmp)
      return NULL;

   bmp->w =w;
   bmp->cr = w;
   bmp->h = h;
   bmp->cb = h;
   bmp->clip = TRUE;
   bmp->cl = 0;
   bmp->ct = 0;
   bmp->vtable = &_screen_vtable;
   bmp->write_bank = gfx_directx_write_bank;
   bmp->read_bank = gfx_directx_write_bank;
   bmp->dat = NULL;
   bmp->id = id;
   bmp->extra = NULL;
   bmp->x_ofs = 0;
   bmp->y_ofs = 0;
   bmp->seg = _video_ds();
   for (i = 0; i < h; i++)
      bmp->line[i] = pseudo_surf_mem;

   DDRAW_SURFACE_OF(bmp) = surf;

   return bmp;
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
   DDRAW_SURFACE *surf;
   BITMAP *bmp;
   HRESULT hr;

   /* try to detect page flipping and triple buffering patterns */
   if ((width == forefront_bitmap->w) && (height == forefront_bitmap->h)) {

      switch (flipping_pages) {

         case 0:
            /* recycle the forefront bitmap as a video bitmap */
            bmp = make_bitmap_from_surface(DDRAW_SURFACE_OF(forefront_bitmap), width, height, BMP_ID_VIDEO);
            if (bmp) {
               flipping_pages++;
               return bmp;
            }
            return NULL;

         case 1:
            /* create a back_buffer and attach it to the frontbuffer */
            back_buffer = gfx_directx_create_surface(width, height, NULL, DDRAW_SURFACE_VIDEO);
            if (back_buffer) {
               hr = IDirectDrawSurface2_AddAttachedSurface(DDRAW_SURFACE_OF(forefront_bitmap)->id, back_buffer->id);
               if (hr == DD_OK) {
                  bmp = make_bitmap_from_surface(back_buffer, width, height, BMP_ID_VIDEO);
                  if (bmp) {
                     flipping_pages++;
                     return bmp;
                  }

                  IDirectDrawSurface2_DeleteAttachedSurface(DDRAW_SURFACE_OF(forefront_bitmap)->id, 0, back_buffer->id);
               }

               gfx_directx_destroy_surface(back_buffer);
            }
            return NULL;

        case 2:
            /* create a third buffer and attach it to the back_buffer */
            triple_buffer = gfx_directx_create_surface(width, height, NULL, DDRAW_SURFACE_VIDEO);
            if (triple_buffer) {
               hr = IDirectDrawSurface2_AddAttachedSurface(back_buffer->id, triple_buffer->id);
               if (hr == DD_OK) {
                  bmp = make_bitmap_from_surface(triple_buffer, width, height, BMP_ID_VIDEO);
                  if (bmp) {
                     flipping_pages++;
                     return bmp;
                  }

                  IDirectDrawSurface2_DeleteAttachedSurface(back_buffer->id, 0, triple_buffer->id);
               }

               gfx_directx_destroy_surface(triple_buffer);
            }
            return NULL;
      }
   }

   /* create the DirectDraw surface */
   if (ddpixel_format)
      surf = gfx_directx_create_surface(width, height, ddpixel_format, DDRAW_SURFACE_SYSTEM);
   else
      surf = gfx_directx_create_surface(width, height, NULL, DDRAW_SURFACE_VIDEO);

   if (!surf)
      return NULL;

   /* create the bitmap that wraps up the surface */
   bmp = make_bitmap_from_surface(surf, width, height, BMP_ID_VIDEO);
   if (!bmp) {
      gfx_directx_destroy_surface(surf);
      return NULL;
   }

   return bmp;
}



/* gfx_directx_destroy_video_bitmap:
 */
void gfx_directx_destroy_video_bitmap(BITMAP *bmp)
{
   DDRAW_SURFACE *surf = DDRAW_SURFACE_OF(bmp);

   if (surf == triple_buffer) {
      if (back_buffer)
         IDirectDrawSurface2_DeleteAttachedSurface(back_buffer->id, 0, triple_buffer->id);
      triple_buffer = NULL;
      flipping_pages--;
   }
   else if (surf == back_buffer) {
      IDirectDrawSurface2_DeleteAttachedSurface(DDRAW_SURFACE_OF(forefront_bitmap)->id, 0, back_buffer->id);
      back_buffer = NULL;
      flipping_pages--;
   }
   else if (surf == DDRAW_SURFACE_OF(forefront_bitmap)) {
      flipping_pages--;
      free(bmp);
      return;  /* don't destroy the surface! */
   }

   /* destroy the surface */
   gfx_directx_destroy_surface(surf);

   free(bmp);
}



/* flip_with_forefront:
 *  Worker function for DirectDraw page flipping.
 */
static int flip_with_forefront(DDRAW_SURFACE *surf, int wait)
{
   LPDIRECTDRAWSURFACE2 ddsurf;
   HRESULT hr;

   ASSERT((surf == back_buffer) || (surf == triple_buffer));

   /* flip only in the foreground */
   if (!app_foreground) {
      thread_switch_out();
      return 0;
   }

   /* retrieve the actual DirectDraw surface */
   ddsurf = surf->id;

   /* flip the contents of the surfaces */
   hr = IDirectDrawSurface2_Flip(DDRAW_SURFACE_OF(forefront_bitmap)->id, ddsurf, wait ? DDFLIP_WAIT : 0);

   /* if the surface has been lost, try to restore all surfaces */
   if (hr == DDERR_SURFACELOST) {
      if (restore_all_ddraw_surfaces() == 0)
         hr = IDirectDrawSurface2_Flip(DDRAW_SURFACE_OF(forefront_bitmap)->id, ddsurf, wait ? DDFLIP_WAIT : 0);
   }

   if (FAILED(hr)) {
      _TRACE("Can't flip (%x)\n", hr);
      return -1;
   }

   /* flip the actual surfaces to keep track of the contents */
   surf->id = DDRAW_SURFACE_OF(forefront_bitmap)->id;
   DDRAW_SURFACE_OF(forefront_bitmap)->id = ddsurf;

   /* keep track of the forefront bitmap */
   DDRAW_SURFACE_OF(forefront_bitmap) = surf;

   return 0;
}



/* gfx_directx_show_video_bitmap:
 */
int gfx_directx_show_video_bitmap(BITMAP *bmp)
{
   return flip_with_forefront(DDRAW_SURFACE_OF(bmp), TRUE);
}



/* gfx_directx_request_video_bitmap:
 */
int gfx_directx_request_video_bitmap(BITMAP *bmp)
{
   return flip_with_forefront(DDRAW_SURFACE_OF(bmp), FALSE);
}



/* gfx_directx_poll_scroll:
 */
int gfx_directx_poll_scroll(void)
{
   HRESULT hr;

   ASSERT(flipping_pages == 3);

   hr = IDirectDrawSurface2_GetFlipStatus(DDRAW_SURFACE_OF(forefront_bitmap)->id, DDGFS_ISFLIPDONE);

   /* if the surface has been lost, try to restore all surfaces */
   if (hr == DDERR_SURFACELOST) {
      if (restore_all_ddraw_surfaces() == 0)
         hr = IDirectDrawSurface2_GetFlipStatus(DDRAW_SURFACE_OF(forefront_bitmap)->id, DDGFS_ISFLIPDONE);
   }

   if (FAILED(hr))
      return -1;

   return 0;
}



/* gfx_directx_create_system_bitmap:
 */
BITMAP *gfx_directx_create_system_bitmap(int width, int height)
{
   DDRAW_SURFACE *surf;
   BITMAP *bmp;

   /* create the DirectDraw surface */
   surf = gfx_directx_create_surface(width, height, ddpixel_format, DDRAW_SURFACE_SYSTEM);
   if (!surf)
      return NULL;

   /* create the bitmap that wraps up the surface */
   bmp = make_bitmap_from_surface(surf, width, height, BMP_ID_SYSTEM);
   if (!bmp) {
      gfx_directx_destroy_surface(surf);
      return NULL;
   }

   return bmp;
}



/* gfx_directx_destroy_system_bitmap:
 */
void gfx_directx_destroy_system_bitmap(BITMAP *bmp)
{
   /* destroy the surface */
   gfx_directx_destroy_surface(DDRAW_SURFACE_OF(bmp));

   free(bmp);
}



/* win_get_dc: (WinAPI)
 *  Returns device context of a video or system bitmap.
 */
HDC win_get_dc(BITMAP *bmp)
{
   LPDIRECTDRAWSURFACE2 ddsurf;
   HDC dc;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         ddsurf = DDRAW_SURFACE_OF(bmp)->id;
         hr = IDirectDrawSurface2_GetDC(ddsurf, &dc);

         /* If the surface has been lost, try to restore all surfaces
          * and, on success, try again to get the DC.
          */
         if (hr == DDERR_SURFACELOST) {
            if (restore_all_ddraw_surfaces() == 0)
               hr = IDirectDrawSurface2_GetDC(ddsurf, &dc);
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
   LPDIRECTDRAWSURFACE2 ddsurf;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         ddsurf = DDRAW_SURFACE_OF(bmp)->id;
         hr = IDirectDrawSurface2_ReleaseDC(ddsurf, dc);

         /* If the surface has been lost, try to restore all surfaces
          * and, on success, try again to release the DC.
          */
         if (hr == DDERR_SURFACELOST) {
            if (restore_all_ddraw_surfaces() == 0)
               hr = IDirectDrawSurface2_ReleaseDC(ddsurf, dc);
         }
      }
   }
}
