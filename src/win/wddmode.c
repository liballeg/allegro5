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



/* window thread callback parameters */
int _wnd_width = 0;
int _wnd_height = 0;
int _wnd_depth = 0;



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



/* gfx_directx_update_color_format:
 *  set the _rgb variables for correct color format
 */
int gfx_directx_update_color_format(int color_depth)
{
   DDSURFACEDESC surf_desc;
   HRESULT hr;
   int shift_r, shift_g, shift_b;
   int true_depth;

   if (color_depth > 8) {
      /* Get current video mode */
      surf_desc.dwSize = sizeof(surf_desc);
      hr = IDirectDraw_GetDisplayMode(directdraw, &surf_desc);
      if (FAILED(hr)) {
	 _TRACE("Can't get color format.\n");
	 return 0;
      }

      /* pass color format */
      shift_r = _get_color_shift(surf_desc.ddpfPixelFormat.dwRBitMask);
      shift_g = _get_color_shift(surf_desc.ddpfPixelFormat.dwGBitMask);
      shift_b = _get_color_shift(surf_desc.ddpfPixelFormat.dwBBitMask);
      true_depth = _get_color_bits(surf_desc.ddpfPixelFormat.dwRBitMask) +
	  _get_color_bits(surf_desc.ddpfPixelFormat.dwGBitMask) +
	  _get_color_bits(surf_desc.ddpfPixelFormat.dwBBitMask);

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

      return true_depth;
   }

   return 8;
}



/* try_video_mode:
 *  sets the passed video mode and returns the real color depth. 
 *  drv_depth means the color depth that is passed to the driver.
 *  all_depth is the allegro color depth.
 */
static int try_video_mode(int w, int h, int drv_depth, int all_depth)
{
   int true_depth;
   _wnd_width = w;
   _wnd_height = h;
   _wnd_depth = drv_depth;

   _TRACE("Setting display mode(%u, %u, %u)\n", w, h, drv_depth);
   if (wnd_call_proc(wnd_set_video_mode) != 0)
      return 0;

   if (all_depth != 8) {
      /* for high and true color modes check color format */
      true_depth = gfx_directx_update_color_format(all_depth);
      if (true_depth == 0)
	 return -1;

      return true_depth;
   }
   else
      return 8;
}



/* set_video_mode:
 */
int set_video_mode(int w, int h, int v_w, int v_h, int color_depth)
{
   int real_depth;

   /* let the window thread do the hard work */
   real_depth = try_video_mode(w, h, color_depth, color_depth);

   /* check whether the requested mode was set by driver */
   if (real_depth != color_depth) {
      if (color_depth == 15 || color_depth == 16) {
	 _TRACE("Wrong color depth set by DirectDraw (passed: %u, set: %u)\n", color_depth, real_depth);

	 /* Some graphic drivers set a 15 bit mode, although 16 bit was passed. If 
	  * this is the case we have to try out the other high color mode.
	  */
	 if (color_depth == 15) {
	    _TRACE("Trying to set 16 bit mode instead.\n");
	    real_depth = try_video_mode(w, h, 16, color_depth);
	 }
	 else {
	    _TRACE("Trying to set 15 bit mode instead.\n");
	    real_depth = try_video_mode(w, h, 15, color_depth);
	 }

	 if (real_depth != color_depth) {
	    _TRACE("Wrong color depth set by DirectDraw (passed: %u, set: %u)\n", color_depth, real_depth);
	    _TRACE("Unable to set correct color depth.\n");
	    return -1;
	 }
      }
      else if (real_depth == 0) {
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
