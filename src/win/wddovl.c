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
 *      DirectDraw overlay gfx driver.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth);



GFX_DRIVER gfx_directx_ovl =
{
   GFX_DIRECTX_OVL,
   empty_string,
   empty_string,
   "DirectDraw overlay",
   init_directx_ovl,
   gfx_directx_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // gfx_directx_poll_scroll,
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap,
   gfx_directx_destroy_video_bitmap,
   NULL,                        // gfx_directx_show_video_bitmap,
   NULL,                        // gfx_directx_request_video_bitmap,
   gfx_directx_create_system_bitmap,
   gfx_directx_destroy_system_bitmap,
   NULL,                        // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                        // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                        // AL_METHOD(void, hide_mouse, (void));
   NULL,                        // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   0, 0,                        // int w, h;                     /* physical (not virtual!) screen size */
   TRUE,                        // int linear;                   /* true if video memory is linear */
   0,                           // long bank_size;               /* bank size, in bytes */
   0,                           // long bank_gran;               /* bank granularity, in bytes */
   0,                           // long vid_mem;                 /* video memory size, in bytes */
   0,                           // long vid_phys_base;           /* physical address of video memory */
};

static char gfx_driver_desc[256] = EMPTY_STRING;

LPDIRECTDRAWSURFACE overlay_surface = NULL;
BOOL overlay_visible = FALSE;

static int desktop_depth;


/* create_overlay:
 */
static int create_overlay(int w, int h, int color_depth)
{
   /* create primary surface */
   overlay_surface = gfx_directx_create_surface(w, h, color_depth, 1, 0, 1);
   if (!overlay_surface) {
      _TRACE("Can't create overlay surface.\n");
      return -1;
   }

   return 0;
}



/* show_overlay:
 */
static int show_overlay(int x, int y, int w, int h)
{
   HRESULT hr;
   DDCOLORKEY key;
   RECT dest_rect =
   {x, y, x + w, y + h};

   _TRACE("show_overlay(%d, %d, %d, %d)\n", x, y, w, h);

   overlay_visible = TRUE;

   /* dest color keying */
   key.dwColorSpaceLowValue = 0; /* (wnd_back_color & 0xffff); */
   key.dwColorSpaceHighValue = 0; /* (wnd_back_color >> 16); */

   hr = IDirectDrawSurface_SetColorKey(dd_prim_surface, DDCKEY_DESTOVERLAY, &key);
   if (FAILED(hr)) {
      _TRACE("Can't set overlay dest color key\n");
      return -1;
   }

   /* update overlay */
   _TRACE("Updating overlay (key=0x%x)\n", wnd_back_color);
   hr = IDirectDrawSurface_UpdateOverlay(overlay_surface, NULL,
					 dd_prim_surface, &dest_rect,
				     DDOVER_SHOW | DDOVER_KEYDEST, NULL);
   if (FAILED(hr)) {
      IDirectDrawSurface_UpdateOverlay(overlay_surface, NULL,
	 dd_prim_surface, NULL, DDOVER_HIDE, NULL);

      _TRACE("Can't display overlay (%x)\n", hr);
      /* but we keep overlay_visible as TRUE to allow future updates */
      return -1;
   }

   return 0;
}



/* hide_overlay:
 */
void hide_overlay(void)
{
   if (overlay_visible) {
      overlay_visible = FALSE;

      IDirectDrawSurface_UpdateOverlay(overlay_surface, NULL,
	 dd_prim_surface, NULL, DDOVER_HIDE, NULL);
   }
}



/* update_overlay:
 *  moves and resizes overlay to fit into the window's client area
 */
static int update_overlay()
{
   RECT pos;

   GetClientRect(allegro_wnd, &pos);
   ClientToScreen(allegro_wnd, (LPPOINT)&pos);
   ClientToScreen(allegro_wnd, (LPPOINT)&pos + 1);

   return show_overlay(pos.left, pos.top,
		       pos.right - pos.left, pos.bottom - pos.top);
}



/* wnd_set_windowed_coop:
 */
static int wnd_set_windowed_coop(void)
{
   HRESULT hr;

   hr = IDirectDraw_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_NORMAL);
   if (FAILED(hr)) {
      _TRACE("SetCooperative level = %s (%x), hwnd = %x\n", win_err_str(hr), hr, allegro_wnd);
      return -1;
   }

   return 0;
}


/* _get_n_bits:
 * gets the number of bits of a given bitmask
 */
static int _get_n_bits (int mask)
{
    int n = 0;

    while (mask) {
        mask = mask & (mask - 1);  
        n++;
    }
    return n;
}


/* verify_color_depth:
 * compares the color depth requested with the real color depth
 */
static int verify_color_depth (int color_depth)
{
   DDSURFACEDESC surf_desc;
   HRESULT hr;
   
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
      desktop_depth = _get_n_bits (surf_desc.ddpfPixelFormat.dwRBitMask) +
                      _get_n_bits (surf_desc.ddpfPixelFormat.dwGBitMask) +
                      _get_n_bits (surf_desc.ddpfPixelFormat.dwBBitMask);

   if (color_depth == desktop_depth)
      return 0;
   else
      return -1;
}


/* setup_driver_desc:
 *  Sets up the driver description string.
 */
static void setup_driver_desc(void)
{
   char tmp1[80];

   usprintf(gfx_driver_desc,
       uconvert_ascii("DirectDraw, in matching, %d bpp window", tmp1),
           desktop_depth);
   
   gfx_directx_ovl.desc = gfx_driver_desc;
}


/* handle_window_size:
 *  updates overlay if window is moved or resized
 */
void handle_window_size(int x, int y, int w, int h)
{
   if (overlay_visible)
      show_overlay(x, y, w, h);
}



/* wddovl_switch_out:
 *  handle overlay switched out
 */
void wddovl_switch_out(void)
{
   if (overlay_surface)
      hide_overlay();
}



/* wddovl_switch_in:
 *  handle overlay switched in
 */
void wddovl_switch_in(void)
{
   if (overlay_surface)
      show_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
}



/* gfx_directx_ovl:
 */
static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth)
{
   RECT win_size;

   /* overlay would allow scrolling on some cards, but isn't implemented yet */
   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0))
      return NULL;

   _enter_critical();

   /* init DirectX */
   if (init_directx() != 0)
      goto Error;
   if (verify_color_depth(color_depth))
      goto Error;
   if (wnd_call_proc(wnd_set_windowed_coop) != 0)
      goto Error;
   if (finalize_directx_init() != 0)
      goto Error;
   if (finalize_directx_init() != 0)
      goto Error;
   if ((dd_caps.dwCaps & DDCAPS_OVERLAY) == 0)
      goto Error;

   /* adjust window */
   wnd_paint_back = TRUE;
   win_size.left = wnd_x = 32;  /* (GetSystemMetrics(SM_CXSCREEN) - w) / 2 */
   win_size.right = wnd_x + w;
   win_size.top = wnd_y = 32;   /* (GetSystemMetrics(SM_CYSCREEN) - h) / 2 */
   win_size.bottom = wnd_y + h;
   wnd_width = w;
   wnd_height = h;

   AdjustWindowRect(&win_size, GetWindowLong(allegro_wnd, GWL_STYLE), FALSE);
   MoveWindow(allegro_wnd, win_size.left, win_size.top,
   win_size.right - win_size.left, win_size.bottom - win_size.top, TRUE);

   /* create surfaces */
   if (create_primary(w, h, color_depth) != 0)
      goto Error;
   if (create_overlay(w, h, color_depth) != 0)
      goto Error;
   if (color_depth == 8) {
      if (create_palette(overlay_surface) != 0)
	 goto Error;
   }
   else
       gfx_directx_update_color_format(color_depth);
   if (update_overlay() != 0)
      goto Error;

   /* setup Allegro gfx driver */
   setup_driver_desc();
   if (setup_driver(&gfx_directx_ovl, w, h, color_depth) != 0)
      goto Error;
   dd_frontbuffer = make_directx_bitmap(overlay_surface, w, h, color_depth, BMP_ID_VIDEO);
   enable_acceleration(&gfx_directx_ovl);
   wnd_windowed = TRUE;
   set_display_switch_mode(SWITCH_PAUSE);

   _exit_critical();

   return dd_frontbuffer;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_exit(NULL);

   return NULL;
}

