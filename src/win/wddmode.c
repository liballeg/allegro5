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
 *      DirectDraw video mode setting.
 *
 *      By Stefan Schimanski.
 *
 *      Unified setup code and refresh rate support by Eric Botcazou.
 *
 *      Graphics mode list fetching code by Henrik Stokseth.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"


int desktop_depth;
BOOL same_color_depth;


typedef struct MODE_INFO {
  int mode_supported;
  int modes;
  GFX_MODE_LIST *gfx;
} MODE_INFO;

static int pixel_realdepth[] = {8, 15, 15, 16, 16, 24, 24, 32, 32, 0};

static DDPIXELFORMAT pixel_format[] = {
   /* 8-bit */
   {sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED8, 0, {8}, {0}, {0}, {0}, {0}},
   /* 16-bit RGB 5:5:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x7C00}, {0x03e0}, {0x001F}, {0}},
   /* 16-bit BGR 5:5:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x001F}, {0x03e0}, {0x7C00}, {0}},
   /* 16-bit RGB 5:6:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0xF800}, {0x07e0}, {0x001F}, {0}},
   /* 16-bit BGR 5:6:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x001F}, {0x07e0}, {0xF800}, {0}},
   /* 24-bit RGB */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {24}, {0xFF0000}, {0x00FF00}, {0x0000FF}, {0}},
   /* 24-bit BGR */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {24}, {0x0000FF}, {0x00FF00}, {0xFF0000}, {0}},
   /* 32-bit RGB */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {32}, {0xFF0000}, {0x00FF00}, {0x0000FF}, {0}},
   /* 32-bit BGR */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {32}, {0x0000FF}, {0x00FF00}, {0xFF0000}, {0}} 
};


/* window thread callback parameters */
static int _wnd_width, _wnd_height, _wnd_depth, _wnd_refresh_rate, _wnd_flags;



/* wnd_set_video_mode:
 *  called by window thread to set a gfx mode; this is needed because DirectDraw can only
 *  change the mode in the thread that handles the window
 */
static int wnd_set_video_mode(void)
{
   HRESULT hr;

   /* set the cooperative level to allow fullscreen access */
   hr = IDirectDraw2_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
   if (FAILED(hr)) {
      _TRACE("SetCooperativeLevel() failed (%x)\n", hr);
      goto Error;
   }

   /* switch to fullscreen mode */
   hr = IDirectDraw2_SetDisplayMode(directdraw, _wnd_width, _wnd_height, _wnd_depth,
                                                _wnd_refresh_rate, _wnd_flags);
   if (FAILED(hr)) {
      _TRACE("SetDisplayMode(%u, %u, %u, %u, %u) failed (%x)\n", _wnd_width, _wnd_height, _wnd_depth,
                                                                 _wnd_refresh_rate, _wnd_flags, hr);
      goto Error;
   }

   return 0;

 Error:
   return -1;
}



/* _get_color_shift:
 *  returns shift value for color mask
 */
static int _get_color_shift(int mask)
{
   int n;

   for (n = 0; ((mask & 1) == 0) && (mask != 0); n++)
      mask >>= 1;

   return n;
}



/* _get_color_bits:
 *  returns used bits in color mask
 */
static int _get_color_bits(int mask)
{
   int n;
   int num = 0;

   for (n = 0; n < 32; n++) {
      num += (mask & 1);
      mask >>= 1;
   }

   return num;
}



/* gfx_directx_compare_color_depth:
 *  compares the requested color depth with the desktop depth and find the best
 *   pixel format for color conversion if they don't match
 *  returns 0 if the depths match, -1 if they don't
 */ 
int gfx_directx_compare_color_depth(int color_depth)
{
   DDSURFACEDESC surf_desc;
   HRESULT hr;
   int i;

   /* get current video mode */
   surf_desc.dwSize = sizeof(surf_desc);
   hr = IDirectDraw2_GetDisplayMode(directdraw, &surf_desc);
   if (FAILED(hr)) {
      _TRACE("Can't get color format.\n");
      return -1;
   }

   /* get the *real* color depth of the desktop */
   desktop_depth = surf_desc.ddpfPixelFormat.dwRGBBitCount;
   if (desktop_depth == 16) /* sure? */
      desktop_depth = _get_color_bits (surf_desc.ddpfPixelFormat.dwRBitMask) +
                      _get_color_bits (surf_desc.ddpfPixelFormat.dwGBitMask) +
                      _get_color_bits (surf_desc.ddpfPixelFormat.dwBBitMask);

   if (color_depth == desktop_depth) {
      dd_pixelformat = NULL;
      same_color_depth = TRUE;
      return 0;
   }
   else {
      /* test for the same depth and RGB order */
      for (i=0 ; pixel_realdepth[i] ; i++)
         if ((pixel_realdepth[i] == color_depth) &&
            ((surf_desc.ddpfPixelFormat.dwRBitMask & pixel_format[i].dwRBitMask) ||
                (surf_desc.ddpfPixelFormat.dwBBitMask & pixel_format[i].dwBBitMask) ||
                   (color_depth == 8))) {
                      dd_pixelformat = &pixel_format[i];
                      break;
         }

      same_color_depth = FALSE;
      return -1;
   }
}



/* gfx_directx_update_color_format:
 *  sets the _rgb variables for correct color format
 */
int gfx_directx_update_color_format(LPDIRECTDRAWSURFACE2 surf, int color_depth)
{
   DDPIXELFORMAT pixel_format;
   HRESULT hr;
   int shift_r, shift_g, shift_b;

   /* get pixel format */
   pixel_format.dwSize = sizeof(DDPIXELFORMAT);
   hr = IDirectDrawSurface2_GetPixelFormat(surf, &pixel_format);
   if (FAILED(hr)) {
      _TRACE("Can't get color format.\n");
      return -1;
   }

   /* pass color format */
   shift_r = _get_color_shift(pixel_format.dwRBitMask);
   shift_g = _get_color_shift(pixel_format.dwGBitMask);
   shift_b = _get_color_shift(pixel_format.dwBBitMask);

   /* set correct shift values */
   switch (color_depth) {
      case 15:
         _rgb_r_shift_15 = shift_r;
         _rgb_g_shift_15 = shift_g;
         _rgb_b_shift_15 = shift_b;
         break;

      case 16:
         _rgb_r_shift_16 = shift_r;
         _rgb_g_shift_16 = shift_g;
         _rgb_b_shift_16 = shift_b;
         break;

      case 24:
         _rgb_r_shift_24 = shift_r;
         _rgb_g_shift_24 = shift_g;
         _rgb_b_shift_24 = shift_b;
         break;

      case 32:
         _rgb_r_shift_32 = shift_r;
         _rgb_g_shift_32 = shift_g;
         _rgb_b_shift_32 = shift_b;
         break;
   }

   return 0;
}



/* EnumModesCallback:
 *  callback for graphics mode enumeration
 */ 
static HRESULT CALLBACK EnumModesCallback(LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID mode_info_addr)
{
   MODE_INFO *mode_info = (MODE_INFO *) mode_info_addr;
   int real_bpp;

   if (mode_info->gfx) {
      mode_info->gfx[mode_info->modes].width = lpDDSurfaceDesc->dwWidth;
      mode_info->gfx[mode_info->modes].height = lpDDSurfaceDesc->dwHeight;
      mode_info->gfx[mode_info->modes].bpp = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

      /* check if 16 bpp mode is 16 bpp or 15 bpp*/
      if (mode_info->gfx[mode_info->modes].bpp == 16) {
         real_bpp = _get_color_bits (lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask) +
                    _get_color_bits (lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask) +
                    _get_color_bits (lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask);
         if (real_bpp == 15)
            mode_info->gfx[mode_info->modes].bpp = real_bpp;
      }
   }

   mode_info->mode_supported = TRUE;
   mode_info->modes++;

   return DDENUMRET_OK;
}



/* gfx_directx_fetch_mode_list:
 *  creates a list of available video modes,
 *  returns 0 on success and -1 on failure.
 */
int gfx_directx_fetch_mode_list(void)
{
   MODE_INFO mode_info;
   HRESULT hr;
   int flags, modes, gfx_off;

   /* enumerate VGA Mode 13h under DirectX 5 or greater */
   if (_dx_ver >= 0x500)
      flags = DDEDM_STANDARDVGAMODES;
   else
      flags = 0;

   if (!directdraw) {
      init_directx();
      gfx_off = TRUE;
   }
   else
      gfx_off = FALSE;

   /* count modes */
   mode_info.gfx = NULL;
   mode_info.modes = 0;

   hr = IDirectDraw2_EnumDisplayModes(directdraw, flags, NULL, &mode_info, EnumModesCallback);
   if (FAILED(hr))
      goto Error;

   modes = mode_info.modes;

   /* allocate mode list */
   if (gfx_mode_list)
      free(gfx_mode_list);

   gfx_mode_list = malloc(sizeof(GFX_MODE_LIST) * (modes + 1));
   if (!gfx_mode_list)
      goto Error;

   /* fill in mode list */
   mode_info.gfx = gfx_mode_list;
   mode_info.modes = 0;

   hr = IDirectDraw2_EnumDisplayModes(directdraw, flags, NULL, &mode_info, EnumModesCallback);
   if (FAILED(hr))
      goto Error;

   /* terminate mode list */
   mode_info.gfx[modes].width = 0;
   mode_info.gfx[modes].height = 0;
   mode_info.gfx[modes].bpp = 0;

   if (gfx_off)
      gfx_directx_exit(dd_frontbuffer);

   return 0;

 Error:
   if (gfx_off)
      gfx_directx_exit(dd_frontbuffer);

   return -1;
}



/* set_video_mode:
 *  sets the requested fullscreen video mode
 */
int set_video_mode(int w, int h, int v_w, int v_h, int color_depth)
{
   MODE_INFO mode_info;
   DDSURFACEDESC surf_desc;
   int flags, i;
   HRESULT hr;

   surf_desc.dwSize   = sizeof(DDSURFACEDESC);
   surf_desc.dwFlags  = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
   surf_desc.dwHeight = h;
   surf_desc.dwWidth  = w;

   /* enumerate VGA Mode 13h under DirectX 5 or greater */
   if (_dx_ver >= 0x500)
      flags = DDEDM_STANDARDVGAMODES;
   else
      flags = 0;

   for (i=0 ; pixel_realdepth[i] ; i++)
      if (pixel_realdepth[i] == color_depth) {
         surf_desc.ddpfPixelFormat = pixel_format[i];
         mode_info.mode_supported = FALSE;
         mode_info.gfx = NULL;

         /* check whether the requested mode is supported */
         if (_refresh_rate_request) {
            _TRACE("refresh rate requested: %d Hz\n", _refresh_rate_request);
            surf_desc.dwFlags |= DDSD_REFRESHRATE;
            surf_desc.dwRefreshRate = _refresh_rate_request;
            hr = IDirectDraw2_EnumDisplayModes(directdraw, flags | DDEDM_REFRESHRATES, &surf_desc,
                                               &mode_info, EnumModesCallback);
            if (FAILED(hr))
               break;
         }

         if (!mode_info.mode_supported) {
            _TRACE("no refresh rate requested\n");
            surf_desc.dwFlags &= ~DDSD_REFRESHRATE;
            surf_desc.dwRefreshRate = 0;
            hr = IDirectDraw2_EnumDisplayModes(directdraw, flags, &surf_desc,
                                               &mode_info, EnumModesCallback);
            if (FAILED(hr))
               break;
         }

         if (mode_info.mode_supported) {
            _wnd_width        = surf_desc.dwWidth;
            _wnd_height       = surf_desc.dwHeight;
            _wnd_depth        = pixel_format[i].dwRGBBitCount; /* not color_depth */
            _wnd_refresh_rate = surf_desc.dwRefreshRate;

            if ((_wnd_width == 320) && (_wnd_height == 200) && (_wnd_depth == 8) &&
                                                               (flags & DDEDM_STANDARDVGAMODES))
               _wnd_flags = DDSDM_STANDARDVGAMODE;
            else
               _wnd_flags = 0;

            /* let the window thread do the hard work */ 
            _TRACE("setting display mode(%u, %u, %u, %u)\n", w, h, color_depth, _wnd_refresh_rate);
            if (wnd_call_proc(wnd_set_video_mode) != 0)
               break;

            /* check whether the requested mode was properly set by the driver */
            if (gfx_directx_compare_color_depth(color_depth) == 0)
               goto ModeSet;
         }
      }  /* end of 'if (pixel_realdepth[i] == color_depth)' */ 
      
   _TRACE("Unable to set any display mode.\n");
   return -1;

 ModeSet:
   /* remove window controls */
   SetWindowLong(allegro_wnd, GWL_STYLE, WS_POPUP);
   ShowWindow(allegro_wnd, SW_MAXIMIZE);

   return 0;
}
