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
 *      DirectDraw windowed gfx driver.
 *
 *      By Isaac Cruz.
 *
 *      General overhaul by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"


BITMAP* pseudo_screen = NULL;   /* for page-flipping          */

/* exported only for asmlock.s */
char* wd_dirty_lines = NULL;    /* used in WRITE_BANK()       */
void (*update_window) (RECT* rect) = NULL;  /* window updater */


static void gfx_directx_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync);
static int gfx_directx_show_video_bitmap_win(struct BITMAP *bitmap);
static struct BITMAP *init_directx_win(int w, int h, int v_w, int v_h, int color_depth);
static void gfx_directx_win_exit(struct BITMAP *b);


GFX_DRIVER gfx_directx_win =
{
   GFX_DIRECTX_WIN,
   empty_string,
   empty_string,
   "DirectDraw window",
   init_directx_win,
   gfx_directx_win_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap,
   gfx_directx_destroy_video_bitmap,
   gfx_directx_show_video_bitmap_win,
   gfx_directx_show_video_bitmap_win,
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
   0, 0,                        // int w, h;
   TRUE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   TRUE                         // int windowed;
};


static void switch_in_win(void);
static void handle_window_enter_sysmode_win(void);
static void handle_window_exit_sysmode_win(void);
static void handle_window_move_win(int, int, int, int);


static WIN_GFX_DRIVER win_gfx_driver_windowed =
{
   TRUE,
   switch_in_win,
   NULL,                        // AL_METHOD(void, switch_out, (void));
   handle_window_enter_sysmode_win,
   handle_window_exit_sysmode_win,
   handle_window_move_win,
   NULL,                        // AL_METHOD(void, iconify, (void));
   NULL,                        // AL_METHOD(void, paint, (RECT *));
};


static char gfx_driver_desc[256] = EMPTY_STRING;
static LPDIRECTDRAWSURFACE2 offscreen_surface = NULL;
/* If the allegro color depth is not the same as the desktop color depth,
 * we need a pre-converted offscreen surface that will be blitted to the window
 * when in background, in order to ensure a proper clipping.
 */ 
static LPDIRECTDRAWSURFACE2 preconv_offscreen_surface = NULL;
static RECT working_area;
static COLORCONV_BLITTER_FUNC *colorconv_blit = NULL;
static int direct_updating_mode;
static GFX_VTABLE _special_vtable; /* special vtable for offscreen bitmap */



/* get_working_area:
 *  Retrieves the rectangle of the working area.
 */
static void get_working_area(RECT *working_area)
{
   SystemParametersInfo(SPI_GETWORKAREA, 0, working_area, 0);
   working_area->left   += 3;  /* for the taskbar */
   working_area->top    += 3;
   working_area->right  -= 3;
   working_area->bottom -= 3;
}



/* switch_in_win:
 *  Handles window switched in.
 */
static void switch_in_win(void)
{
   get_working_area(&working_area);
}



/* handle_window_enter_sysmode_win:
 *  Causes the driver to switch to indirect updating mode.
 */
static void handle_window_enter_sysmode_win(void)
{
   if (colorconv_blit && direct_updating_mode) {
      direct_updating_mode = FALSE;
      _TRACE("direct updating mode off\n");
   }
}



/* handle_window_exit_sysmode_win:
 *  Causes the driver to switch back to direct updating mode.
 */
static void handle_window_exit_sysmode_win(void)
{
   if (colorconv_blit && !direct_updating_mode) {
      direct_updating_mode = TRUE;
      _TRACE("direct updating mode on\n");
   }
}



/* handle_window_move_win:
 *  Makes sure alignment is kept after the window has been moved.
 */
static void handle_window_move_win(int x, int y, int w, int h)
{
   int xmod;
   RECT window_rect;

   if (colorconv_blit) {
      if (((BYTES_PER_PIXEL(desktop_depth) == 1) && (xmod=ABS(x)%4)) ||
          ((BYTES_PER_PIXEL(desktop_depth) == 2) && (xmod=ABS(x)%2)) ||
          ((BYTES_PER_PIXEL(desktop_depth) == 3) && (xmod=ABS(x)%4))) {
         GetWindowRect(allegro_wnd, &window_rect);
         SetWindowPos(allegro_wnd, 0, window_rect.left + (x > 0 ? -xmod : xmod),
                      window_rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
         _TRACE("window shifted by %d pixe%s to the %s to enforce alignment\n",
                xmod, xmod > 1 ? "ls" : "l", x > 0 ? "left" : "right");
      }
   }
}



/* update_matching_window:
 *  Updates a portion of the window when the color depths match.
 */
static void update_matching_window(RECT* rect)
{
   RECT dest_rect;

   _enter_gfx_critical();

   if (!pseudo_screen) {
      _exit_gfx_critical();
      return;
   }

   if (rect)
      dest_rect = *rect;
   else {
      dest_rect.left   = 0;
      dest_rect.right  = gfx_directx_win.w;
      dest_rect.top    = 0;
      dest_rect.bottom = gfx_directx_win.h;
   }

   ClientToScreen(allegro_wnd, (LPPOINT)&dest_rect);
   ClientToScreen(allegro_wnd, (LPPOINT)&dest_rect + 1);
 
   /* blit offscreen backbuffer to the window */
   IDirectDrawSurface2_Blt(dd_prim_surface, &dest_rect,
                           BMP_EXTRA(pseudo_screen)->surf, rect,
                           0, NULL);
   _exit_gfx_critical();
}



/* is_not_contained:
 *  Helper to find the relative position of two rectangles.
 */ 
static INLINE int is_not_contained(RECT *rect1, RECT *rect2)
{
   if ( (rect1->left   < rect2->left)   ||
        (rect1->top    < rect2->top)    ||
        (rect1->right  > rect2->right)  ||
        (rect1->bottom > rect2->bottom) )
      return TRUE;
   else
      return FALSE;
}



/* ddsurf_blit_ex:
 *  Extended blit function performing color conversion.
 */
static int ddsurf_blit_ex(LPDIRECTDRAWSURFACE2 dest_surf, RECT *dest_rect,
                          LPDIRECTDRAWSURFACE2 src_surf, RECT *src_rect)
{
   DDSURFACEDESC src_desc, dest_desc;
   struct GRAPHICS_RECT src_gfx_rect, dest_gfx_rect;
   HRESULT hr;

   src_desc.dwSize = sizeof(src_desc);
   src_desc.dwFlags = 0;
   dest_desc.dwSize = sizeof(dest_desc);
   dest_desc.dwFlags = 0;

   hr = IDirectDrawSurface2_Lock(dest_surf, dest_rect, &dest_desc, DDLOCK_WAIT, NULL);
   if (FAILED(hr))
      return -1;

   hr = IDirectDrawSurface2_Lock(src_surf, src_rect, &src_desc, DDLOCK_WAIT, NULL);
   if (FAILED(hr)) {
      IDirectDrawSurface2_Unlock(dest_surf, NULL);
      return -1;
   }
   
   /* fill in source graphics rectangle description */
   src_gfx_rect.width  = src_rect->right  - src_rect->left;
   src_gfx_rect.height = src_rect->bottom - src_rect->top;
   src_gfx_rect.pitch  = src_desc.lPitch;
   src_gfx_rect.data   = src_desc.lpSurface;

   /* fill in destination graphics rectangle description */
   dest_gfx_rect.pitch = dest_desc.lPitch;
   dest_gfx_rect.data  = dest_desc.lpSurface;
   
   /* function doing the hard work */
   colorconv_blit(&src_gfx_rect, &dest_gfx_rect);

   IDirectDrawSurface2_Unlock(src_surf, NULL);
   IDirectDrawSurface2_Unlock(dest_surf, NULL);

   return 0;
}



/* update_colorconv_window:
 *  Updates a portion of the window when the color depths don't match.
 */
static void update_colorconv_window(RECT* rect)
{
   RECT src_rect, dest_rect;

   _enter_gfx_critical();

   if (!pseudo_screen) {
      _exit_gfx_critical();
      return;
   }

   if (rect) {
#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      src_rect.left   = rect->left & 0xfffffffc;
      src_rect.right  = (rect->right+3) & 0xfffffffc;
#else
      src_rect.left   = rect->left;
      src_rect.right  = rect->right;
#endif
      src_rect.top    = rect->top;
      src_rect.bottom = rect->bottom;
   }
   else {
      src_rect.left   = 0;
      src_rect.right  = gfx_directx_win.w;
      src_rect.top    = 0;
      src_rect.bottom = gfx_directx_win.h;
   }

   dest_rect = src_rect; 
   ClientToScreen(allegro_wnd, (LPPOINT)&dest_rect);
   ClientToScreen(allegro_wnd, (LPPOINT)&dest_rect + 1);

   if (!direct_updating_mode || is_not_contained(&dest_rect, &working_area) ||
                                       (GetForegroundWindow() != allegro_wnd)) {
      /* first blit to the pre-converted offscreen buffer */
      if (ddsurf_blit_ex(preconv_offscreen_surface, &src_rect,
                         BMP_EXTRA(pseudo_screen)->surf, &src_rect) != 0) {
         _exit_gfx_critical();
         return;
      }
 
      /* blit preconverted offscreen buffer to the window (clipping done by DirectDraw) */
      IDirectDrawSurface2_Blt(dd_prim_surface, &dest_rect,
                              preconv_offscreen_surface, &src_rect,
                              0, NULL);
   }
   else {
      /* blit directly to the primary surface WITHOUT clipping */
      ddsurf_blit_ex(dd_prim_surface, &dest_rect,
                     BMP_EXTRA(pseudo_screen)->surf, &src_rect);
   }

   _exit_gfx_critical();
}



/* gfx_directx_show_video_bitmap_win:
 *  Makes the specified video bitmap visible.
 */
static int gfx_directx_show_video_bitmap_win(struct BITMAP *bitmap)
{
   if (BMP_EXTRA(bitmap)->surf) {
      pseudo_screen->vtable->release = gfx_directx_unlock;
      pseudo_screen->vtable->unwrite_bank = gfx_directx_unwrite_bank;
      pseudo_screen->write_bank = gfx_directx_write_bank;
      pseudo_screen = bitmap;
      pseudo_screen->vtable->release = gfx_directx_unlock_win;
      pseudo_screen->vtable->unwrite_bank = gfx_directx_unwrite_bank_win;
      pseudo_screen->write_bank = gfx_directx_write_bank_win;
      update_window(NULL);
      return 0;
   }

   return -1;
}



/* gfx_directx_set_palette_win_8:
 *  Updates the palette for color conversion from 8-bit to 8-bit.
 */
static void gfx_directx_set_palette_win_8(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   unsigned char *cmap;
   int n;

   cmap = _get_colorconv_map();

   for (n = from; n <= to; n++)
      cmap[n] = desktop_rgb_map.data[p[n].r>>1][p[n].g>>1][p[n].b>>1];

   update_window(NULL);
}



/* gfx_directx_set_palette_win:
 *  Updates the palette for color conversion from 8-bit to hi/true color.
 */
static void gfx_directx_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   _set_colorconv_palette(p, from, to);
   update_window(NULL);
}



/* wnd_set_windowed_coop:
 *  Sets the DirectDraw cooperative level.
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
 *  Compares the requested color depth with the desktop color depth.
 */
static int verify_color_depth (int color_depth)
{
   if ((gfx_directx_compare_color_depth(color_depth) == 0) && (color_depth != 8)) {
      /* the color depths match */ 
      update_window = update_matching_window;
   }
   else {
      /* disallow cross-conversion between 15-bit and 16-bit colors */
      if ((BYTES_PER_PIXEL(color_depth) == 2) && (BYTES_PER_PIXEL(desktop_depth) == 2))
         return -1;

      /* the color depths don't match, need color conversion */
      colorconv_blit = _get_colorconv_blitter(color_depth, desktop_depth);

      if (!colorconv_blit)
         return -1;

      update_window = update_colorconv_window;
      direct_updating_mode = TRUE;
   }

   win_gfx_driver_windowed.paint = update_window;

   return 0;
}



/* create_offscreen:
 *  Creates the offscreen surface.
 */
static int create_offscreen(int w, int h, int color_depth)
{
   if (colorconv_blit) {
      /* create pre-converted offscreen surface in video memory */
      preconv_offscreen_surface = gfx_directx_create_surface(w, h, NULL, 1, 0, 0);

      if (!preconv_offscreen_surface) {
         _TRACE("Can't create preconverted offscreen surface in video memory.\n");

         /* create pre-converted offscreen surface in plain memory */
         preconv_offscreen_surface = gfx_directx_create_surface(w, h, NULL, 0, 0, 0);

         if (!preconv_offscreen_surface) {
            _TRACE("Can't create preconverted offscreen surface.\n");
            return -1;
         }
      }

      offscreen_surface = gfx_directx_create_surface(w, h, dd_pixelformat, 0, 0, 0);
   }
   else {
      /* create offscreen surface in video memory */
      offscreen_surface = gfx_directx_create_surface(w, h, NULL, 1, 0, 0);
      
      if (!offscreen_surface) {
         _TRACE("Can't create offscreen surface in video memory.\n");

         /* create offscreen surface in plain memory */
         offscreen_surface = gfx_directx_create_surface(w, h, NULL, 0, 0, 0);
      }
   }

   if (!offscreen_surface) {
      _TRACE("Can't create offscreen surface.\n");
      return -1;
   }

   return 0;
}


/* setup_driver_desc:
 *  Sets the driver description string.
 */
static void setup_driver_desc(void)
{
   char tmp1[128], tmp2[128];

   uszprintf(gfx_driver_desc, sizeof(gfx_driver_desc),
	     uconvert_ascii("DirectDraw, in %s, %d bpp window", tmp1),
	     uconvert_ascii((colorconv_blit ? "color conversion" : "matching"), tmp2),
	     desktop_depth);
   
   gfx_directx_win.desc = gfx_driver_desc;
}



/* init_directx_win:
 *  Initializes the driver.
 */
static struct BITMAP *init_directx_win(int w, int h, int v_w, int v_h, int color_depth)
{
   RECT win_size;
   unsigned char *cmap;
   HRESULT hr;
   int i;

   /* flipping is impossible in windowed mode */
   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported virtual resolution"));
      return NULL;
   }

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   if (w%4)
      return NULL;
#endif

   _enter_critical();

   /* init DirectX */
   if (init_directx() != 0)
      goto Error;
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
   win_size.right = 32 + w;
   win_size.top = wnd_y = 32;
   win_size.bottom = 32 + h;
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

   /* create primary surface */
   if (create_primary() != 0)
      goto Error;

   /* create clipper */
   if (create_clipper(allegro_wnd) != 0)
      goto Error;

   hr = IDirectDrawSurface_SetClipper(dd_prim_surface, dd_clipper);
   if (FAILED(hr))
      goto Error;

   /* create offscreen backbuffer */
   if (create_offscreen(w, h, color_depth) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Windowed mode not supported"));
      goto Error;
   }

   /* setup color management */
   if (desktop_depth == 8) {
      build_desktop_rgb_map();

      if (color_depth == 8) {
         gfx_directx_win.set_palette = gfx_directx_set_palette_win_8;
      }
      else {
         cmap = _get_colorconv_map();

         for (i=0; i<4096; i++)
            cmap[i] = desktop_rgb_map.data[((i&0xF00)>>7) | ((i&0x800)>>11)]
                                          [((i&0x0F0)>>3) | ((i&0x080)>>7)]
                                          [((i&0x00F)<<1) | ((i&0x008)>>3)];

         if (gfx_directx_update_color_format(offscreen_surface, color_depth) != 0)
            goto Error;
      }
   }
   else {
      if (color_depth == 8) {
         gfx_directx_win.set_palette = gfx_directx_set_palette_win;

         /* init the core library color conversion functions */ 
         if (gfx_directx_update_color_format(dd_prim_surface, desktop_depth) != 0)
            goto Error;
      }
      else {
         if (gfx_directx_update_color_format(offscreen_surface, color_depth) != 0)
            goto Error;
      }
   }

   /* setup Allegro gfx driver */
   setup_driver_desc();
   if (setup_driver(&gfx_directx_win, w, h, color_depth) != 0)
      goto Error;

   dd_frontbuffer = make_directx_bitmap(offscreen_surface, w, h, color_depth, BMP_ID_VIDEO);

   enable_acceleration(&gfx_directx_win);
   memcpy (&_special_vtable, &_screen_vtable, sizeof (GFX_VTABLE));
   _special_vtable.release = gfx_directx_unlock_win;
   _special_vtable.unwrite_bank = gfx_directx_unwrite_bank_win;
   dd_frontbuffer->vtable = &_special_vtable;
   dd_frontbuffer->write_bank = gfx_directx_write_bank_win;

   /* the last flag serves as end of loop delimiter */
   wd_dirty_lines = calloc(h+1, sizeof(char));
   wd_dirty_lines[h] = 0;

   pseudo_screen = dd_frontbuffer;

   /* connect to the system driver */
   win_gfx_driver = &win_gfx_driver_windowed;

   /* set default switching policy */
   set_display_switch_mode(SWITCH_PAUSE);

   /* grab input devices */
   win_grab_input();

   _exit_critical();

   return dd_frontbuffer;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_win_exit(NULL);

   return NULL;
}



/* gfx_directx_win_exit:
 *  Shuts down the driver.
 */
static void gfx_directx_win_exit(struct BITMAP *b)
{ 
   _enter_gfx_critical();

   if (b)
      clear_bitmap(b);

   /* disconnect from the system driver */
   win_gfx_driver = NULL;

   /* destroy dirty lines array */   
   free(wd_dirty_lines);
   wd_dirty_lines = NULL;

   /* destroy the offscreen backbuffer */
   gfx_directx_destroy_surf(offscreen_surface);
   offscreen_surface = NULL;
   pseudo_screen = NULL;

   /* destroy the pre-converted offscreen buffer */
   gfx_directx_destroy_surf(preconv_offscreen_surface);
   preconv_offscreen_surface = NULL;

   /* release the color conversion blitter */
   if (colorconv_blit) {
      _release_colorconv_blitter(colorconv_blit);
      colorconv_blit = NULL;
   }

   /* unlink surface from bitmap */
   if (b)
      b->extra = NULL;

   _exit_gfx_critical();
   
   gfx_directx_exit(NULL);
}

