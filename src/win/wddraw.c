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
 *      DirectDraw gfx drivers.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



/* directx vars */
LPDIRECTDRAW directdraw = NULL;
LPDIRECTDRAWSURFACE dd_prim_surface = NULL;
LPDIRECTDRAWSURFACE dd_back_surface = NULL;
LPDIRECTDRAWSURFACE dd_trip_surface = NULL;
LPDIRECTDRAWPALETTE dd_palette = NULL;
LPDIRECTDRAWCLIPPER dd_clipper = NULL;
DDCAPS dd_caps;
CRITICAL_SECTION gfx_crit_sect;
struct BITMAP *dd_frontbuffer = NULL;
static PALETTEENTRY _palette[256];
char *pseudo_surf_mem;


/* gfx_directx_set_palette:
 */
void gfx_directx_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int n;

   /* convert into Windows format */
   for (n = from; n <= to; n++) {
      _palette[n].peRed = (p[n].r << 2) | ((p[n].r & 0x30) >> 4);
      _palette[n].peGreen = (p[n].g << 2) | ((p[n].g & 0x30) >> 4);
      _palette[n].peBlue = (p[n].b << 2) | ((p[n].b & 0x30) >> 4);
   }

   /* wait for vertical retrace */
   if (vsync)
      gfx_directx_sync();

   /* set the convert palette */
   IDirectDrawPalette_SetEntries(dd_palette, 0, from, to - from + 1, &_palette[from]);
}



/* gfx_directx_sync:
 *  wait for vertical sync
 */
void gfx_directx_sync(void)
{
   IDirectDraw_WaitForVerticalBlank(directdraw, DDWAITVB_BLOCKBEGIN, NULL);
}



/* init_directx:
 */
int init_directx(void)
{
   HRESULT hr;

   /* Initialize gfx critical section */
   InitializeCriticalSection(&gfx_crit_sect);

   /* first we have to setup directdraw */
   hr = DirectDrawCreate(NULL, &directdraw, NULL);
   if (FAILED(hr))
      return -1;

   /* get capabilities */
   dd_caps.dwSize = sizeof(dd_caps);
   hr = IDirectDraw_GetCaps(directdraw, &dd_caps, NULL);
   if (FAILED(hr)) {
      _TRACE("Can't get driver caps\n");
      return -1;
   }

   return 0;
}



/* create_palette:
 */
int create_palette(LPDIRECTDRAWSURFACE surf)
{
   HRESULT hr;
   int n;

   /* prepare palette entries */
   for (n = 0; n < 256; n++) {
      _palette[n].peFlags = PC_NOCOLLAPSE | PC_RESERVED;
   }

   hr = IDirectDraw_CreatePalette(directdraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, _palette, &dd_palette, NULL);
   if (FAILED(hr))
      return -1;
   IDirectDrawSurface_SetPalette(surf, dd_palette);

   return 0;
}



/* create_primary:
 */
int create_primary(int w, int h, int color_depth)
{
   /* create primary surface */
   dd_prim_surface = gfx_directx_create_surface(w, h, color_depth, 1, 1, 0);
   if (!dd_prim_surface) {
      _TRACE("Can't create primary surface.\n");
      return -1;
   }
   dd_back_surface = gfx_directx_create_surface(w, h, color_depth, 1, 2, 0);
   dd_trip_surface = gfx_directx_create_surface(w, h, color_depth, 1, 3, 0);

   return 0;
}



/* create_clipper:
 */
int create_clipper(HWND hwnd)
{
   HRESULT hr;

   hr = IDirectDraw_CreateClipper(directdraw, 0, &dd_clipper, NULL);
   if (FAILED(hr)) {
      _TRACE("Can't create clipper (%x)\n", hr);
      return -1;
   }

   hr = IDirectDrawClipper_SetHWnd(dd_clipper, 0, hwnd);
   if (FAILED(hr)) {
      _TRACE("Can't set clipper window (%x)\n", hr);
      return -1;
   }

   return 0;
}



/* setup_driver:
 */
int setup_driver(GFX_DRIVER * drv, int w, int h, int color_depth)
{
   /* setup the driver structure */
   drv->w = w;
   drv->h = h;
   drv->linear = 1;
   drv->vid_mem = dd_caps.dwVidMemTotal + ((color_depth + 7) >> 3) * w * h;

   /* create our pseudo surface memory */
   pseudo_surf_mem = malloc(2048);

   /* modify the vtable to work with video memory */
   memcpy(&_screen_vtable, _get_vtable(color_depth), sizeof(_screen_vtable));
   _screen_vtable.unwrite_bank = gfx_directx_unwrite_bank;
   _screen_vtable.acquire = gfx_directx_lock;
   _screen_vtable.release = gfx_directx_unlock;
   _screen_vtable.created_sub_bitmap = gfx_directx_created_sub_bitmap;

   return 0;
}



/* finalize_directx_init:
 */
int finalize_directx_init(void)
{
   HRESULT hr;
   long int freq;

   /* set correct sync timer speed */
   hr = IDirectDraw_GetMonitorFrequency(directdraw, &freq);
   if ((FAILED(hr)) || (freq < 40) || (freq > 200))
      set_sync_timer_freq(70);
   else
      set_sync_timer_freq(freq);

   return 0;
}



/* gfx_directx_wnd_exit:
 *  restore old mode from window thread
 */
int gfx_directx_wnd_exit(void)
{
   if (directdraw) {
      /* set cooperative level back to normal */
      IDirectDraw_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_NORMAL);

      /* release DirectDraw interface */
      IDirectDraw_Release(directdraw);
   }

   return 0;
}



/* gfx_directx_exit:
 */
void gfx_directx_exit(struct BITMAP *b)
{ 
   _enter_critical();

   set_sync_timer_freq(70);
   if (b)
      clear(b);

   /* destroy overlay surface */
   if (overlay_surface) {
      hide_overlay();
      IDirectDrawSurface_Release(overlay_surface);
      overlay_surface = NULL;
      overlay_visible = FALSE;
   }

   /* destroy primary surface */
   if (dd_prim_surface) {
      IDirectDrawSurface_Release(dd_prim_surface);
      dd_prim_surface = NULL;
      dd_back_surface = NULL;
      dd_trip_surface = NULL;
   }

   /* unlink surface from bitmap */
   if (b)
      b->extra = NULL;

   /* normally this list must be empty */
   unregister_all_directx_bitmaps();

   /* destroy clipper */
   if (dd_clipper) {
      IDirectDrawClipper_Release(dd_clipper);
      dd_clipper = NULL;
   }

   /* destroy palette */
   if (dd_palette) {
      IDirectDrawPalette_Release(dd_palette);
      dd_palette = NULL;
   }

   /* free pseudo memory */
   if (pseudo_surf_mem) {
      free(pseudo_surf_mem);
      pseudo_surf_mem = NULL;
   }

   /* before restoring video mode, hide window */
   wnd_paint_back = FALSE;
   restore_window_style();
      /* HWND_TOPMOST isn't a good idea because it's a darned sticky flag 
         which prevents the windowed driver from behaving nicely after a
         first driver shutdown (e.g in test.exe) */
   SetWindowPos(allegro_wnd, HWND_TOP,
		-100, -100, 0, 0, SWP_SHOWWINDOW);

   if (directdraw) {
      /* let the window thread set the coop level back to normal 
	 and destory the directdraw object */
      wnd_call_proc(gfx_directx_wnd_exit);
      directdraw = NULL;
   }

   DeleteCriticalSection(&gfx_crit_sect);

   _exit_critical();
}
