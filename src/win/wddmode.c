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
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"


int desktop_depth;
BOOL same_color_depth;


static int pixel_realdepth[] = { 8, 15, 15, 16, 16, 24, 24, 32, 32, 0 };

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
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {32}, {0x0000FF}, {0x00FF00}, {0xFF0000}, {0}	} 
};


/* window thread callback parameters */
static int _wnd_width, _wnd_height, _wnd_depth;



/* wnd_set_video_mode:
 *  called by window thread to set a gfx mode. This is needed because DirectDraw can only
 *  change the mode in the thread that handles the window
 */
static int wnd_set_video_mode(void)
{
   HRESULT hr;

   /* set the cooperative level to allow fullscreen access */
   hr = IDirectDraw_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
   if (FAILED(hr)) {
      _TRACE("SetCooperative level = %s (%x), hwnd = %x\n", win_err_str(hr), hr, allegro_wnd);
      goto Error;
   }

   /* switch to fullscreen mode */
   hr = IDirectDraw_SetDisplayMode(directdraw, _wnd_width, _wnd_height, _wnd_depth);
   if (FAILED(hr)) {
      _TRACE("SetDisplayMode(%u, %u, %u) = %s (%x)\n", _wnd_width, _wnd_height, _wnd_depth, win_err_str(hr), hr);
      goto Error;
   }

   wnd_acquire_mouse();

   return 0;

 Error:
   return -1;
}



/* _get_color_shift:
 *  return shift value for color mask
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
 *  return value: 0 if the depths match, -1 if they don't
 */ 
int gfx_directx_compare_color_depth(int color_depth)
{
   DDSURFACEDESC surf_desc;
   HRESULT hr;
   int i;

   /* get current video mode */
   surf_desc.dwSize = sizeof(surf_desc);
   hr = IDirectDraw_GetDisplayMode(directdraw, &surf_desc);
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
 *  set the _rgb variables for correct color format
 */
int gfx_directx_update_color_format(LPDIRECTDRAWSURFACE surf, int color_depth)
{
   DDPIXELFORMAT pixel_format;
   HRESULT hr;
   int shift_r, shift_g, shift_b;

   /* get pixel format */
   pixel_format.dwSize = sizeof(DDPIXELFORMAT);
   hr = IDirectDrawSurface_GetPixelFormat(surf, &pixel_format);
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



/* try_video_mode:
 *  sets the passed video mode. 
 *  drv_depth means the color depth that is passed to the driver.
 *  all_depth is the allegro color depth.
 */
static int try_video_mode(int w, int h, int drv_depth, int all_depth)
{
   _wnd_width = w;
   _wnd_height = h;
   _wnd_depth = drv_depth;

   _TRACE("Setting display mode(%u, %u, %u)\n", w, h, drv_depth);
   if (wnd_call_proc(wnd_set_video_mode) != 0)
      return -1;

   return gfx_directx_compare_color_depth(all_depth);
}



/* set_video_mode:
 */
int set_video_mode(int w, int h, int v_w, int v_h, int color_depth)
{
   int ret;

   /* let the window thread do the hard work */
   ret = try_video_mode(w, h, color_depth, color_depth);

   /* check whether the requested mode was set by driver */
   if (ret != 0) {
      if (color_depth == 15 || color_depth == 16) {
	 _TRACE("Wrong color depth set by DirectDraw (passed: %u, set: %u)\n", color_depth, desktop_depth);

	 /* Some graphic drivers set a 15 bit mode, although 16 bit was passed. If 
	  * this is the case we have to try out the other high color mode.
	  */
	 if (color_depth == 15) {
	    _TRACE("Trying to set 16 bit mode instead.\n");
	    ret = try_video_mode(w, h, 16, color_depth);
	 }
	 else {
	    _TRACE("Trying to set 15 bit mode instead.\n");
	    ret = try_video_mode(w, h, 15, color_depth);
	 }

	 if (ret != 0) {
	    _TRACE("Wrong color depth set by DirectDraw (passed: %u, set: %u)\n", color_depth, desktop_depth);
	    _TRACE("Unable to set correct color depth.\n");
	    return -1;
	 }
      }
      else {
	 _TRACE("Unable to set correct color depth.\n");
	 return -1;
      }
   }

   /* remove window controls */
   SetWindowLong(allegro_wnd, GWL_STYLE, WS_POPUP);
   ShowWindow(allegro_wnd, SW_MAXIMIZE);

   /* update the screensaver flag to disallow alt-tab in SWITCH_NONE mode */
   if (get_display_switch_mode()==SWITCH_NONE)
      SystemParametersInfo(SPI_SCREENSAVERRUNNING, TRUE, 0, 0);

   return 0;
}
