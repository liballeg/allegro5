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
 *      DirectDraw bitmap creation.
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
 * in order to be consistent with how other platforms behave.
 */
static int reused_screen = 0;



/* get_surface2_int:
 *  helper function to get the DirectDrawSurface2 interface
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
 *  creates a DirectDraw surface
 */ 
LPDIRECTDRAWSURFACE2 gfx_directx_create_surface(int w, int h, LPDDPIXELFORMAT pixel_format,
                                                int video, int primary, int overlay)
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

   if (primary) {
      surf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
      surf_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
      surf_desc.dwBackBufferCount = 2;
   }
   else if (overlay) {
      surf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
      surf_desc.dwFlags |= DDSD_BACKBUFFERCOUNT | DDSD_HEIGHT | DDSD_WIDTH;
      surf_desc.dwBackBufferCount = 2;
      surf_desc.dwHeight = h;
      surf_desc.dwWidth = w;

      if (pixel_format) {    /* use pixel format */
         surf_desc.dwFlags |= DDSD_PIXELFORMAT;
         surf_desc.ddpfPixelFormat = *pixel_format;
      }
   }
   else if (video) {
      surf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN;
      surf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
      surf_desc.dwHeight = h;
      surf_desc.dwWidth = w;
   }
   else {
      surf_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
      surf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
      surf_desc.dwHeight = h;
      surf_desc.dwWidth = w;

      if (pixel_format) {    /* use pixel format */
         surf_desc.dwFlags |= DDSD_PIXELFORMAT;
         surf_desc.ddpfPixelFormat = *pixel_format;
      }
   }

   /* create the surface with this properties */
   hr = IDirectDraw2_CreateSurface(directdraw, &surf_desc, &_surf1, NULL);

   if (FAILED(hr)) {
      if (primary || overlay) {
         /* we lower the number of backbuffers */
         surf_desc.dwBackBufferCount = 1;
         hr = IDirectDraw2_CreateSurface(directdraw, &surf_desc, &_surf1, NULL);
            
         if (FAILED(hr)) {
            /* no backbuffer any more (e.g. in windowed mode) */
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
 *  destroys a DirectDraw surface
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
 *  worker function to wrap up a DirectDraw surface in a video bitmap
 */
static BITMAP *_make_video_bitmap(int w, int h, unsigned long addr, struct GFX_VTABLE *vtable,
                                  int color_depth, int bpl)
{
   int i, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *) malloc(size);
   if (!b)
      return NULL;

   _gfx_bank = realloc(_gfx_bank, h * sizeof(int));
   if (!_gfx_bank) {
      free(b);
      return NULL;
   }

   LOCK_DATA(b, size);
   LOCK_DATA(_gfx_bank, h * sizeof(int));

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

   _last_bank_1 = _last_bank_2 = -1;

   b->line[0] = (char *)addr;
   _gfx_bank[0] = 0;

   for (i = 1; i < h; i++) {
      b->line[i] = b->line[i - 1] + bpl;
      _gfx_bank[i] = 0;
   }

   return b;
}



/* make_directx_bitmap:
 *  connects a DirectDraw surface with an Allegro bitmap
 */
BITMAP *make_directx_bitmap(LPDIRECTDRAWSURFACE2 surf, int w, int h, int color_depth, int id)
{
   struct BITMAP *bmp;
   struct BMP_EXTRA_INFO *bmp_extra;

   /* create Allegro bitmap */
   bmp = _make_video_bitmap(w, h, (long)pseudo_surf_mem, &_screen_vtable, color_depth, 0);
   bmp->id = id;
   bmp->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
   bmp->write_bank = gfx_directx_write_bank;
   bmp->read_bank = gfx_directx_write_bank;

   /* setup surface info structure to store additional information */
   bmp_extra = BMP_EXTRA(bmp);
   bmp_extra->surf = surf;
   bmp_extra->next = NULL;
   bmp_extra->prev = NULL;
   bmp_extra->flags = 0;
   bmp_extra->lock_nesting = 0;

   register_directx_bitmap(bmp);

   return bmp;
}



/* release_directx_bitmap:
 *  releases a DirectDraw surface and destroys its info structure
 */
void release_directx_bitmap(BITMAP *bmp)
{
   struct BMP_EXTRA_INFO *bmp_extra;

   if (bmp) {
      if (bmp->extra) {
         bmp_extra = BMP_EXTRA(bmp);
         unregister_directx_bitmap(bmp);
         free(bmp_extra);
      }
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

   /* can we reuse the screen bitmap for this? */
   if ((reused_screen < 3) && (screen->w == width) && (screen->h == height)) {
      reused_screen ++;

      if (reused_screen == 1)
          return screen;
      else if (reused_screen == 2) {
         if (backbuffersurf)
            return make_directx_bitmap (backbuffersurf,
			width, height, _color_depth, BMP_ID_VIDEO);
         else
            reused_screen--;
      }
      else if (reused_screen == 3) {
         if (tripbuffersurf)
            return make_directx_bitmap (tripbuffersurf,
			width, height, _color_depth, BMP_ID_VIDEO);
         else
            reused_screen--;
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
      surf = gfx_directx_create_surface(width, height, dd_pixelformat, 0, 0, 0);
   else
      surf = gfx_directx_create_surface(width, height, NULL, 1, 0, 0);

   if (surf == NULL)
      return NULL;

   /* create Allegro bitmap for surface */
   return make_directx_bitmap(surf, width, height, _color_depth, BMP_ID_VIDEO);
}



/* gfx_directx_destroy_video_bitmap:
 */
void gfx_directx_destroy_video_bitmap(BITMAP *bitmap)
{
   if ( (bitmap == screen) || 
        (BMP_EXTRA(bitmap)->surf == backbuffersurf) ||
	(BMP_EXTRA(bitmap)->surf == tripbuffersurf) ) {
      reused_screen --;
      return;
   }

   if (bitmap == dd_frontbuffer)
      gfx_directx_show_video_bitmap(screen);

   gfx_directx_destroy_surf(BMP_EXTRA(bitmap)->surf);
   release_directx_bitmap(bitmap);
   destroy_bitmap(bitmap);
}



/* _flip_bitmap:
 *  worker function to do DirectDraw page flipping 
 */
static int _flip_bitmap(BITMAP *bitmap, int wait)
{
   LPDIRECTDRAWSURFACE2 visible;
   LPDIRECTDRAWSURFACE2 invisible;
   HRESULT hr;

   /* flip only in the foreground */
   if (!app_foreground) {
      thread_switch_out();
      return 0;
   }

   visible = BMP_EXTRA(dd_frontbuffer)->surf;
   invisible = BMP_EXTRA(bitmap)->surf;

   if (visible == invisible)
      return 0;

   /* try to flip already attached surfaces */
   if ((backbuffersurf && (invisible == backbuffersurf)) ||
       (tripbuffersurf && (invisible == tripbuffersurf))) {
      /* visible is always equal to primbuffersurf */
      hr = IDirectDrawSurface2_Flip(visible, invisible, wait?DDFLIP_WAIT:0);
   }
   else {
      /* link invisible surface to primary surface */
      hr = IDirectDrawSurface2_AddAttachedSurface(visible, invisible);
      if (FAILED(hr)) {
         _TRACE("Can't attach surface (%x)\n", hr);
         return -1;
      }

      /* flip to invisible surface */
      hr = IDirectDrawSurface2_Flip(visible, invisible, wait?DDFLIP_WAIT:0);
      IDirectDrawSurface2_DeleteAttachedSurface(visible, 0, invisible);
   }

   if (FAILED(hr)) {
      _TRACE("Can't flip (%x)\n", hr);
      return -1;
   }
   else {
      BMP_EXTRA(bitmap)->surf = visible;
      BMP_EXTRA(dd_frontbuffer)->surf = invisible;
      dd_frontbuffer = bitmap;
      return 0;
   }
}



/* gfx_directx_show_video_bitmap:
 */
int gfx_directx_show_video_bitmap(struct BITMAP *bitmap)
{
   return _flip_bitmap(bitmap, 1);
}



/* gfx_directx_request_video_bitmap:
 */
int gfx_directx_request_video_bitmap(struct BITMAP *bitmap)
{
   return _flip_bitmap(bitmap, 0);
}



/* gfx_directx_poll_scroll:
 */
int gfx_directx_poll_scroll(void)
{
   HRESULT hr;

   hr = IDirectDrawSurface2_GetFlipStatus(BMP_EXTRA(dd_frontbuffer)->surf,
                                          DDGFS_ISFLIPDONE);
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
   surf = gfx_directx_create_surface(width, height, dd_pixelformat, 0, 0, 0);
   if (surf == NULL)
      return NULL;

   /* create Allegro bitmap for surface */
   return make_directx_bitmap(surf, width, height, _color_depth, BMP_ID_SYSTEM);
}



/* gfx_directx_destroy_system_bitmap:
 */
void gfx_directx_destroy_system_bitmap(BITMAP *bitmap)
{
   if (BMP_EXTRA(bitmap))
      gfx_directx_destroy_surf(BMP_EXTRA(bitmap)->surf);

   release_directx_bitmap(bitmap);
   bitmap->id &= ~BMP_ID_SYSTEM;
   destroy_bitmap(bitmap);
}



/* win_get_dc:
 *  returns device context of a video or system bitmap
 */
HDC win_get_dc(BITMAP * bmp)
{
   LPDIRECTDRAWSURFACE2 surf;
   HDC dc;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         surf = BMP_EXTRA(bmp)->surf;
         hr = IDirectDrawSurface2_GetDC(surf, &dc);
         if (hr == DD_OK)
            return dc;
      }
   }

   return NULL;
}



/* win_release_dc:
 *  releases device context of a video or system bitmap
 */
void win_release_dc(BITMAP * bmp, HDC dc)
{
   LPDIRECTDRAWSURFACE2 surf;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         surf = BMP_EXTRA(bmp)->surf;
         IDirectDrawSurface2_ReleaseDC(surf, dc);
      }
   }
}
