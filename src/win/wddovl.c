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


static int gfx_directx_show_video_bitmap_ovl(struct BITMAP *bitmap);
static int gfx_directx_request_video_bitmap_ovl(struct BITMAP *bitmap);
static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth);
static void gfx_directx_ovl_exit(struct BITMAP *b);


GFX_DRIVER gfx_directx_ovl =
{
   GFX_DIRECTX_OVL,
   empty_string,
   empty_string,
   "DirectDraw overlay",
   init_directx_ovl,
   gfx_directx_ovl_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // gfx_directx_poll_scroll,
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap,
   gfx_directx_destroy_video_bitmap,
   gfx_directx_show_video_bitmap_ovl,
   gfx_directx_request_video_bitmap_ovl,
   gfx_directx_create_system_bitmap,
   gfx_directx_destroy_system_bitmap,
   NULL,                        // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                        // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                        // AL_METHOD(void, hide_mouse, (void));
   NULL,                        // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                        // int w, h;                     /* physical (not virtual!) screen size */
   TRUE,                        // int linear;                   /* true if video memory is linear */
   0,                           // long bank_size;               /* bank size, in bytes */
   0,                           // long bank_gran;               /* bank granularity, in bytes */
   0,                           // long vid_mem;                 /* video memory size, in bytes */
   0,                           // long vid_phys_base;           /* physical address of video memory */
   TRUE                         // int windowed;                 /* true if driver runs windowed */
};


static void show_overlay(void);
static void move_overlay(int, int, int, int);
static void hide_overlay(void);


static WIN_GFX_DRIVER win_gfx_driver_overlay =
{
   TRUE,
   show_overlay,
   NULL,                        // AL_METHOD(void, switch_out, (void));
   NULL,                        // AL_METHOD(void, enter_sysmode, (void));
   NULL,                        // AL_METHOD(void, exit_sysmode, (void));
   move_overlay,
   hide_overlay,
   NULL                         // AL_METHOD(void, paint, (RECT *rect));
};


static char gfx_driver_desc[256] = EMPTY_STRING;
static LPDIRECTDRAWSURFACE2 overlay_surface = NULL;
static BOOL overlay_visible = FALSE;
static HBRUSH original_brush, overlay_brush;



/* update_overlay:
 *  moves and resizes the overlay surface to make it fit into the specified area
 */
static int update_overlay(int x, int y, int w, int h)
{
   HRESULT hr;
   RECT dest_rect = {x, y, x + w, y + h};

   /* show the overlay surface */
   hr = IDirectDrawSurface2_UpdateOverlay(overlay_surface, NULL,
                                          dd_prim_surface, &dest_rect,
                                          DDOVER_SHOW | DDOVER_KEYDEST, NULL);
   if (FAILED(hr)) {
      _TRACE("Can't display overlay (%x)\n", hr);
      IDirectDrawSurface2_UpdateOverlay(overlay_surface, NULL,
	                                dd_prim_surface, NULL,
                                        DDOVER_HIDE, NULL);
      return -1;
   }

   return 0;
}



/* show_overlay:
 *  makes the overlay surface visible on the primary surface
 */
static void show_overlay(void)
{
   if (overlay_surface) {
      overlay_visible = TRUE;
      update_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
   }
}



/* move_overlay:
 *  moves the overlay surface
 */
static void move_overlay(int x, int y, int w, int h)
{
   RECT window_rect;
   int xmod;

   /* handle hardware limitations */
   if ((dd_caps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST) && (xmod = x%dd_caps.dwAlignBoundaryDest)) {
      GetWindowRect(allegro_wnd, &window_rect);
      SetWindowPos(allegro_wnd, 0, window_rect.left + xmod, window_rect.top,
                   0, 0, SWP_NOZORDER | SWP_NOSIZE);
   }
   else if (overlay_visible)
      update_overlay(x, y, w, h);
}



/* hide_overlay:
 *  makes the overlay surface invisible on the primary surface
 */
static void hide_overlay(void)
{
   if (overlay_visible) {
      overlay_visible = FALSE;

      IDirectDrawSurface2_UpdateOverlay(overlay_surface, NULL,
	                                dd_prim_surface, NULL,
                                        DDOVER_HIDE, NULL);
   }
}



/* wnd_set_windowed_coop:
 *  sets the DirectDraw cooperative level to normal
 */
static int wnd_set_windowed_coop(void)
{
   HRESULT hr;

   hr = IDirectDraw2_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_NORMAL);
   if (FAILED(hr)) {
      _TRACE("SetCooperative level = %s (%x), hwnd = %x\n", win_err_str(hr), hr, allegro_wnd);
      return -1;
   }

   return 0;
}



/* setup_driver_desc:
 *  sets up the driver description string
 */
static void setup_driver_desc(void)
{
   char tmp[256];

   uszprintf(gfx_driver_desc, sizeof(gfx_driver_desc), 
	     uconvert_ascii("DirectDraw, in matching, %d bpp overlay", tmp),
	     desktop_depth);
   
   gfx_directx_ovl.desc = gfx_driver_desc;
}



/* gfx_directx_show_video_bitmap_ovl:
 *  driver-specific method for show_video_bitmap()
 */
static int gfx_directx_show_video_bitmap_ovl(struct BITMAP *bitmap)
{
   if (gfx_directx_show_video_bitmap(bitmap)) {
      return -1;
   }
   else {
      if (overlay_visible)
         update_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
      return 0;
   } 
}



/* gfx_directx_request_video_bitmap_ovl:
 *  driver-specific method for request_video_bitmap()
 */
static int gfx_directx_request_video_bitmap_ovl(struct BITMAP *bitmap)
{
   if (gfx_directx_request_video_bitmap(bitmap)) {
      return -1;
   }
   else {
      if (overlay_visible)
         update_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
      return 0;
   } 
}



/* create_overlay:
 *  creates the overlay surface
 */
static int create_overlay(int w, int h, int color_depth)
{
   overlay_surface = gfx_directx_create_surface(w, h, dd_pixelformat, 0, 0, 1);
   
   if (!overlay_surface) {
      _TRACE("Can't create overlay surface.\n");
      return -1;
   }

   return 0;
}



/* init_directx_ovl:
 *  initializes the driver
 */
static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth)
{
   RECT win_size;
   HRESULT hr;
   DDCOLORKEY key;

   /* overlay would allow scrolling on some cards, but it isn't implemented yet */
   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported virtual resolution"));
      return NULL;
   }

   _enter_critical();

   /* init DirectX */
   if (init_directx() != 0)
      goto Error;
   if ((dd_caps.dwCaps & DDCAPS_OVERLAY) == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Overlays not supported"));
      goto Error;
   }
   if (gfx_directx_compare_color_depth(color_depth) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      goto Error;
   }
   if (wnd_call_proc(wnd_set_windowed_coop) != 0)
      goto Error;
   if (finalize_directx_init() != 0)
      goto Error;
  
   /* adjust window */
   win_size.left = wnd_x = 32;
   win_size.right = wnd_x + w;
   win_size.top = wnd_y = 32;
   win_size.bottom = wnd_y + h;
   wnd_width = w;
   wnd_height = h;

   /* paint window background with overlay color key */
   original_brush = (HBRUSH) GetClassLong(allegro_wnd, GCL_HBRBACKGROUND);
   overlay_brush = CreateSolidBrush(MASK_COLOR_32);
   SetClassLong(allegro_wnd, GCL_HBRBACKGROUND, (LONG) overlay_brush);

   /* retrieve the size of the decorated window */
   AdjustWindowRect(&win_size, GetWindowLong(allegro_wnd, GWL_STYLE), FALSE);

   /* display the window */
   MoveWindow(allegro_wnd, win_size.left, win_size.top,
              win_size.right - win_size.left, win_size.bottom - win_size.top, TRUE);

   /* check that the actual window size is the one requested */
   GetClientRect(allegro_wnd, &win_size);
   if ( ((win_size.right - win_size.left) != w) || 
        ((win_size.bottom - win_size.top) != h) ) {
      _TRACE("window size not supported.\n");
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      goto Error;
   }

   /* create surfaces */
   if (create_primary() != 0)
      goto Error;

   if (create_overlay(w, h, color_depth) != 0)
      goto Error;

   /* handle hardware limitations: according to the DirectX SDK, "these restrictions
    * can vary depending on the pixel formats of the overlay and primary surface", so
    * we handle them after creating the surfaces
    */
   dd_caps.dwSize = sizeof(dd_caps);
   hr = IDirectDraw2_GetCaps(directdraw, &dd_caps, NULL);
   if (FAILED(hr)) {
      _TRACE("Can't get driver caps\n");
      goto Error;
   }

   if (dd_caps.dwCaps & DDCAPS_ALIGNSIZESRC) {
      if (w%dd_caps.dwAlignSizeSrc) {
         _TRACE("Alignment requirement not met: source size %d\n", dd_caps.dwAlignSizeSrc);
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         goto Error;
      }
   } 
   else if (dd_caps.dwCaps & DDCAPS_ALIGNSIZEDEST) {
      if (w%dd_caps.dwAlignSizeDest) {
         _TRACE("Alignment requirement not met: dest size %d\n", dd_caps.dwAlignSizeDest);
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         goto Error;
      }
   }

   /* set color format */
   if (color_depth == 8) {
      if (create_palette(overlay_surface) != 0)
	 goto Error;
   }
   else {
      if (gfx_directx_update_color_format(overlay_surface, color_depth) != 0)
         goto Error;
   }

   /* set up Allegro gfx driver */
   setup_driver_desc();

   if (setup_driver(&gfx_directx_ovl, w, h, color_depth) != 0)
      goto Error;

   dd_frontbuffer = make_directx_bitmap(overlay_surface, w, h, color_depth, BMP_ID_VIDEO);

   /* display the overlay surface */
   key.dwColorSpaceLowValue = dd_frontbuffer->vtable->mask_color;
   key.dwColorSpaceHighValue = dd_frontbuffer->vtable->mask_color;

   hr = IDirectDrawSurface2_SetColorKey(dd_prim_surface, DDCKEY_DESTOVERLAY, &key);
   if (FAILED(hr)) {
      _TRACE("Can't set overlay dest color key\n");
      goto Error;
   }

   show_overlay();

   /* use hardware accelerated primitives */
   enable_acceleration(&gfx_directx_ovl);

   /* connect to the system driver */
   win_gfx_driver = &win_gfx_driver_overlay;

   /* set default switching policy */
   set_display_switch_mode(SWITCH_PAUSE);

   /* grab input devices */
   win_grab_input();

   _exit_critical();

   return dd_frontbuffer;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_ovl_exit(NULL);

   return NULL;
}



/* gfx_directx_ovl_exit:
 *  shuts down the driver
 */
static void gfx_directx_ovl_exit(struct BITMAP *b)
{
   _enter_gfx_critical();

   if (b)
      clear_bitmap(b);

   /* disconnect from the system driver */
   win_gfx_driver = NULL;

   /* destroy overlay surface */
   if (overlay_surface) {
      hide_overlay();
      SetClassLong(allegro_wnd, GCL_HBRBACKGROUND, (LONG) original_brush);
      DeleteObject(overlay_brush);
      gfx_directx_destroy_surf(overlay_surface);
      overlay_surface = NULL;
   }

   /* unlink surface from bitmap */
   if (b)
      b->extra = NULL;

   _exit_gfx_critical();
   
   gfx_directx_exit(NULL);
}
