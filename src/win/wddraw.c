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
LPDIRECTDRAW2 directdraw = NULL;
LPDIRECTDRAWSURFACE2 dd_prim_surface = NULL;
LPDIRECTDRAWPALETTE dd_palette = NULL;
LPDIRECTDRAWCLIPPER dd_clipper = NULL;
LPDDPIXELFORMAT dd_pixelformat = NULL;
DDCAPS dd_caps;
struct BITMAP *dd_frontbuffer = NULL;
char *pseudo_surf_mem;

static PALETTEENTRY _palette[256];


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
   IDirectDraw2_WaitForVerticalBlank(directdraw, DDWAITVB_BLOCKBEGIN, NULL);
}



/* init_directx:
 */
int init_directx(void)
{
   LPDIRECTDRAW _directdraw1 = NULL;
   HRESULT hr;

   /* first we have to setup the DirectDraw2 interface */
   hr = DirectDrawCreate(NULL, &_directdraw1, NULL);
   if (FAILED(hr))
      return -1;

   hr = IDirectDraw_SetCooperativeLevel(_directdraw1, allegro_wnd, DDSCL_NORMAL);
   if (FAILED(hr))
      return -1;

   hr = IDirectDraw_QueryInterface(_directdraw1, &IID_IDirectDraw2, (LPVOID *)&directdraw);
   if (FAILED(hr))
      return -1;

   IDirectDraw_Release(_directdraw1);

   /* get capabilities */
   dd_caps.dwSize = sizeof(dd_caps);
   hr = IDirectDraw2_GetCaps(directdraw, &dd_caps, NULL);
   if (FAILED(hr)) {
      _TRACE("Can't get driver caps\n");
      return -1;
   }

   return 0;
}



/* create_palette:
 */
int create_palette(LPDIRECTDRAWSURFACE2 surf)
{
   HRESULT hr;
   int n;

   /* prepare palette entries */
   for (n = 0; n < 256; n++) {
      _palette[n].peFlags = PC_NOCOLLAPSE | PC_RESERVED;
   }

   hr = IDirectDraw2_CreatePalette(directdraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, _palette, &dd_palette, NULL);
   if (FAILED(hr))
      return -1;

   IDirectDrawSurface2_SetPalette(surf, dd_palette);

   return 0;
}



/* create_primary:
 */
int create_primary(void)
{
   /* create primary surface */
   dd_prim_surface = gfx_directx_create_surface(0, 0, NULL, 0, 1, 0);
   if (!dd_prim_surface) {
      _TRACE("Can't create primary surface.\n");
      return -1;
   }

   return 0;
}



/* create_clipper:
 */
int create_clipper(HWND hwnd)
{
   HRESULT hr;

   hr = IDirectDraw2_CreateClipper(directdraw, 0, &dd_clipper, NULL);
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
   DDSCAPS ddsCaps;

   /* setup the driver structure */
   drv->w = w;
   drv->h = h;
   drv->linear = 1;
   ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
   IDirectDraw2_GetAvailableVidMem(directdraw, &ddsCaps, &drv->vid_mem, NULL);
   drv->vid_mem += w * h * BYTES_PER_PIXEL(color_depth);

   /* create our pseudo surface memory */
   pseudo_surf_mem = malloc(2048 * BYTES_PER_PIXEL(color_depth));

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
   hr = IDirectDraw2_GetMonitorFrequency(directdraw, &freq);

   if ((FAILED(hr)) || (freq < 40) || (freq > 200)) {
      set_sync_timer_freq(70);
      _current_refresh_rate = 0;
   }
   else {
      set_sync_timer_freq(freq);
      _current_refresh_rate = freq;
   }

   return 0;
}



/* exit_directx:
 */
int exit_directx(void)
{
   if (directdraw) {
      /* set cooperative level back to normal */
      IDirectDraw2_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_NORMAL);

      /* release DirectDraw interface */
      IDirectDraw2_Release(directdraw);

      directdraw = NULL;
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
      clear_bitmap(b);

   /* disconnect from the system driver */
   win_gfx_driver = NULL;

   /* destroy primary surface */
   gfx_directx_destroy_surf(dd_prim_surface);
   dd_prim_surface = NULL;

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
   set_display_switch_mode(SWITCH_PAUSE);
   system_driver->restore_console_state();
   restore_window_style();

   /* let the window thread set the coop level back
    * to normal and destroy the directdraw object
    */
   wnd_call_proc(exit_directx);

   _exit_critical();
}
