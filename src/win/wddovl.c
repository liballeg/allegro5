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


static void switch_in_ovl(void);
static void switch_out_ovl(void);
static void handle_window_move_ovl(int, int, int, int);
static void hide_overlay(void);


static WIN_GFX_DRIVER win_gfx_driver_overlay =
{
   TRUE,
   switch_in_ovl,
   switch_out_ovl,
   NULL,                        // AL_METHOD(void, enter_sysmode, (void));
   NULL,                        // AL_METHOD(void, exit_sysmode, (void));
   handle_window_move_ovl,
   hide_overlay,
   NULL,                        // AL_METHOD(void, paint, (RECT *));
};


static char gfx_driver_desc[256] = EMPTY_STRING;
static LPDIRECTDRAWSURFACE2 overlay_surface = NULL;
static RECT working_area;
static BOOL overlay_visible = FALSE;



/* is_not_contained
 *  helper to find the relative position of two rectangles
 */ 
static INLINE int is_not_contained(RECT *rect1, RECT *rect2)
{
   if ( (rect1->left   < rect2->left)   ||
        (rect1->top    < rect2->top)    ||
        (rect1->right  > rect2->right)  ||
        (rect1->bottom > rect2->bottom) )
      return 1;
   else
      return 0;
}



/* show_overlay:
 */
static int show_overlay(int x, int y, int w, int h)
{
   HRESULT hr;
   DDCOLORKEY key;
   RECT dest_rect = {x, y, x + w, y + h};

   _TRACE("show_overlay(%d, %d, %d, %d)\n", x, y, w, h);

   overlay_visible = TRUE;

   /* dest color keying */
   key.dwColorSpaceLowValue = 0;
   key.dwColorSpaceHighValue = 0;

   hr = IDirectDrawSurface2_SetColorKey(dd_prim_surface, DDCKEY_DESTOVERLAY, &key);
   if (FAILED(hr)) {
      _TRACE("Can't set overlay dest color key\n");
      return -1;
   }

   /* update overlay */
   _TRACE("Updating overlay (key=0x%x)\n", 0);

   if (is_not_contained(&dest_rect, &working_area))
      hr = DDERR_INVALIDRECT;
   else
      hr = IDirectDrawSurface2_UpdateOverlay(overlay_surface, NULL,
                                             dd_prim_surface, &dest_rect,
                                             DDOVER_SHOW | DDOVER_KEYDEST, NULL);
   if (FAILED(hr)) {
      IDirectDrawSurface2_UpdateOverlay(overlay_surface, NULL,
                                        dd_prim_surface, NULL,
                                        DDOVER_HIDE, NULL);

      _TRACE("Can't display overlay (%x)\n", hr);
      /* but we keep overlay_visible as TRUE to allow future updates */
      return -1;
   }

   return 0;
}



/* hide_overlay:
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



/* update_overlay:
 *  moves and resizes overlay to fit into the window's client area
 */
static int update_overlay()
{
   RECT window_rect = {0, 0, wnd_width, wnd_height};

   ClientToScreen(allegro_wnd, (LPPOINT)&window_rect);

   return show_overlay(window_rect.left, window_rect.top,
                                         wnd_width, wnd_height);
}



/* wnd_set_windowed_coop:
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



/* verify_color_depth:
 * compares the color depth requested with the real color depth
 */
static int verify_color_depth (int color_depth)
{
   if (gfx_directx_compare_color_depth(color_depth) != 0) {
      /* the color depths don't match, need color conversion */

      /* conversion involving 8-bit color isn't supported */
      if ((desktop_depth == 8) || (color_depth == 8))
          return -1;

      /* disallow 15<-->16-bit conversion */
      if ( ((desktop_depth == 15) && (color_depth == 16)) ||
           ((desktop_depth == 16) && (color_depth == 15)) )
         return -1;
   }

   return 0;
}



/* setup_driver_desc:
 *  Sets up the driver description string.
 */
static void setup_driver_desc(void)
{
   char tmp1[80], tmp2[80];

   uszprintf(gfx_driver_desc, sizeof(gfx_driver_desc), 
	     uconvert_ascii("DirectDraw, in %s, %d bpp overlay", tmp1),
	     uconvert_ascii((same_color_depth ? "matching" : "color conversion"), tmp2),
	     desktop_depth );
   
   gfx_directx_ovl.desc = gfx_driver_desc;
}



/* handle_window_move_ovl:
 *  updates overlay if window has been moved or resized
 */
static void handle_window_move_ovl(int x, int y, int w, int h)
{
   int xmod;

   /* handle hardware limitations */
   if ((dd_caps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST) &&
               (xmod = x%dd_caps.dwAlignBoundaryDest)) {
      RECT window_rect;
      GetWindowRect(allegro_wnd, &window_rect);
      SetWindowPos(allegro_wnd, 0, window_rect.left + xmod,
                   window_rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
   }
   else if (overlay_visible)
      show_overlay(x, y, w, h);
}



/* switch_out_ovl:
 *  handle overlay switched out
 */
static void switch_out_ovl(void)
{
   /* if (overlay_surface)
      hide_overlay(); */
}



/* switch_in_ovl:
 *  handle overlay switched in
 */
static void switch_in_ovl(void)
{
   get_working_area(&working_area);

   if (overlay_surface)
      show_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
}



static int gfx_directx_show_video_bitmap_ovl(struct BITMAP *bitmap)
{
   if (gfx_directx_show_video_bitmap(bitmap))
      return -1;
   else {
      update_overlay();
      return 0;
   } 
}



static int gfx_directx_request_video_bitmap_ovl(struct BITMAP *bitmap)
{
   if (gfx_directx_request_video_bitmap(bitmap))
      return -1;
   else {
      update_overlay();
      return 0;
   } 
}



/* create_overlay:
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



/* gfx_directx_ovl:
 */
static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth)
{
   RECT win_size;
   HRESULT hr;

   /* overlay would allow scrolling on some cards, but isn't implemented yet */
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
   if (verify_color_depth(color_depth)) {
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

   /* get the working area */
   get_working_area(&working_area);

   /* create surfaces */
   if (create_primary() != 0)
      goto Error;

   if (create_overlay(w, h, color_depth) != 0)
      goto Error;

   /* handle hardware limitations; according to the DirectX SDK, "these restrictions
      can vary depending on the pixel formats of the overlay and primary surface", so
      we handle them after creating the surfaces */
   dd_caps.dwSize = sizeof(dd_caps);
   hr = IDirectDraw2_GetCaps(directdraw, &dd_caps, NULL);
   if (FAILED(hr)) {
      _TRACE("Can't get driver caps\n");
      goto Error;
   }

   if (dd_caps.dwCaps & DDCAPS_ALIGNSIZESRC) {
      if (w%dd_caps.dwAlignSizeSrc) {
         _TRACE("Alignment requirement not met: source size %d.\n", dd_caps.dwAlignSizeSrc);
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         goto Error;
      }
   } 
   else if (dd_caps.dwCaps & DDCAPS_ALIGNSIZEDEST) {
      if (w%dd_caps.dwAlignSizeDest) {
         _TRACE("Alignment requirement not met: dest size %d.\n", dd_caps.dwAlignSizeDest);
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         goto Error;
      }
   }

   /* finalize driver initialization */
   if (color_depth == 8) {
      if (create_palette(overlay_surface) != 0)
	 goto Error;
   }
   else {
      if (gfx_directx_update_color_format(overlay_surface, color_depth) != 0)
         goto Error;
   }

   if (update_overlay() != 0)
      goto Error;

   /* setup Allegro gfx driver */
   setup_driver_desc();

   if (setup_driver(&gfx_directx_ovl, w, h, color_depth) != 0)
      goto Error;

   dd_frontbuffer = make_directx_bitmap(overlay_surface, w, h, color_depth, BMP_ID_VIDEO);

   /* on my ATI card, hardware-accelerated primitives corrupt the screen when the color depths don't match */
   if (same_color_depth)
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



/* gfx_directx_exit:
 */
static void gfx_directx_ovl_exit(struct BITMAP *b)
{
   _enter_gfx_critical();

   if (b)
      clear (b);

   /* disconnect from the system driver */
   win_gfx_driver = NULL;

   /* destroy overlay surface */
   if (overlay_surface) {
      hide_overlay();
      gfx_directx_destroy_surf(overlay_surface);
      overlay_surface = NULL;
      overlay_visible = FALSE;
   }

   /* unlink surface from bitmap */
   if (b)
      b->extra = NULL;

   _exit_gfx_critical();
   
   gfx_directx_exit(NULL);
}
