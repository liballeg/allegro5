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
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"


BITMAP* pseudo_screen = NULL;   /* for page-flipping          */
int* allegro_palette = NULL;    /* for conversion from 8-bit  */
int* rgb_scale_5335 = NULL;     /* for conversion from 16-bit */
int* dirty_lines = NULL;        /* used in WRITE_BANK         */
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
   NULL,                        // gfx_directx_poll_scroll,
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
   0, 0,                        // int w, h;                     /* physical (not virtual!) screen size */
   TRUE,                        // int linear;                   /* true if video memory is linear */
   0,                           // long bank_size;               /* bank size, in bytes */
   0,                           // long bank_gran;               /* bank granularity, in bytes */
   0,                           // long vid_mem;                 /* video memory size, in bytes */
   0,                           // long vid_phys_base;           /* physical address of video memory */
};

static char gfx_driver_desc[256] = EMPTY_STRING;
static LPDIRECTDRAWSURFACE offscreen_surface = NULL;
/* if the allegro color depth is not the same as the desktop color depth,
 * we need a pre-converted offscreen surface that will be blitted to the window
 * when in background, in order to ensure a proper clipping
 */ 
static LPDIRECTDRAWSURFACE preconv_offscreen_surface = NULL;
static RECT window_rect;
static void (*_update) (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc);
static int clipped_updating_mode;
static int allegro_palette_size;
static GFX_VTABLE _special_vtable; /* special vtable for offscreen bitmap */



/* handle_window_moving_win
 *  makes the driver switch to the clipped updating mode,
 *  if window is moved and color conversion is in use,  
 */
void handle_window_moving_win(void)
{
   if (!same_color_depth && !clipped_updating_mode) {
      clipped_updating_mode = TRUE;
      _TRACE("clipped updating mode on\n");
   }
}



/* handle_window_size_win:
 *  updates window_rect if window has been moved or resized
 */
void handle_window_size_win(void)
{
   /* update window_rect */
   GetClientRect(allegro_wnd, &window_rect);
   ClientToScreen(allegro_wnd, (LPPOINT)&window_rect);
   ClientToScreen(allegro_wnd, (LPPOINT)&window_rect + 1);
      
   /* force alignment to speed up color conversion */
   if (!same_color_depth) {
      int lmod;

      if ( (((desktop_depth == 15) || (desktop_depth == 16)) && (lmod=window_rect.left%2)) ||
                                      ((desktop_depth == 24) && (lmod=window_rect.left%4))  ) {
         RECT dwin_rect;
         GetWindowRect(allegro_wnd, &dwin_rect);
         SetWindowPos(allegro_wnd, 0, dwin_rect.left + lmod, dwin_rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);     
      }   

      clipped_updating_mode = FALSE;
      _TRACE("clipped updating mode off\n");
      if (update_window)
         update_window(NULL);
   }
}



/* update_window_hw
 * function synced with the vertical refresh
 */
void update_window_hw (RECT* rect)
{
   RECT update_rect;
   HRESULT hr;
   
   if (!pseudo_screen)
      return;

   if (rect) {
      update_rect.left = window_rect.left + rect->left;
      update_rect.top = window_rect.top + rect->top;
      update_rect.right = window_rect.left + rect->right;
      update_rect.bottom = window_rect.top + rect->bottom;
   }
   else
      update_rect = window_rect;
   
   _enter_gfx_critical();

   /* blit offscreen backbuffer to the window */
   hr = IDirectDrawSurface_Blt(dd_prim_surface,
        		       &update_rect,
			       BMP_EXTRA(pseudo_screen)->surf,
			       rect, 0, NULL);

   if (FAILED(hr))
      _TRACE("Blt failed\n");

   _exit_gfx_critical();
}



/* update_window_ex:
 * converts between two color formats
 */
void update_window_ex (RECT* rect)
{
   DDSURFACEDESC src_desc, dest_desc;
   HRESULT hr;
   RECT update_rect;

   if (!pseudo_screen)
      return;

   src_desc.dwSize = sizeof(src_desc);
   src_desc.dwFlags = 0;
   dest_desc.dwSize = sizeof(dest_desc);
   dest_desc.dwFlags = 0;

   if (rect) {
      rect->left &= 0xfffffffc;  /* align it */
      rect->right = (rect->right+3) & 0xfffffffc;
      update_rect.left = window_rect.left + rect->left;
      update_rect.top = window_rect.top + rect->top;
      update_rect.right = window_rect.left + rect->right;
      update_rect.bottom = window_rect.top + rect->bottom;
   }
   else
      update_rect = window_rect;

   _enter_gfx_critical();

   if (clipped_updating_mode || (GetForegroundWindow() != allegro_wnd)) {
      /* first blit to the pre-converted offscreen buffer then blit to the primary surface WITH clipping */ 
      hr = IDirectDrawSurface_Lock(preconv_offscreen_surface, rect, &dest_desc, DDLOCK_WAIT, NULL);
      if (FAILED(hr))
         goto End;

      hr = IDirectDrawSurface_Lock(BMP_EXTRA(pseudo_screen)->surf, rect, &src_desc, DDLOCK_WAIT, NULL);
      if (FAILED(hr)) {
         IDirectDrawSurface_Unlock(preconv_offscreen_surface, NULL);
         goto End;
      }   

      src_desc.dwWidth = update_rect.right - update_rect.left;
      src_desc.dwHeight = update_rect.bottom - update_rect.top;

      /* function doing the hard work */
      _update (&src_desc, &dest_desc);

      IDirectDrawSurface_Unlock(BMP_EXTRA(pseudo_screen)->surf, NULL);
      IDirectDrawSurface_Unlock(preconv_offscreen_surface, NULL);

      /* blit preconverted offscreen buffer to the window (clipping done by DirectDraw) */
      hr = IDirectDrawSurface_Blt(dd_prim_surface,
        		          &update_rect,
			          preconv_offscreen_surface,
			          rect, 0, NULL);
      if (FAILED(hr))
         _TRACE("Blt failed\n");
   }
   else {
      /* blit directly to the primary surface WITHOUT clipping */
      hr = IDirectDrawSurface_Lock(dd_prim_surface, &update_rect, &dest_desc, DDLOCK_WAIT, NULL);
      if (FAILED(hr))
         goto End;

      hr = IDirectDrawSurface_Lock(BMP_EXTRA(pseudo_screen)->surf, rect, &src_desc, DDLOCK_WAIT, NULL);
      if (FAILED(hr)) {
         IDirectDrawSurface_Unlock(dd_prim_surface, NULL);
         goto End;
      }
   
      src_desc.dwWidth = update_rect.right - update_rect.left;
      src_desc.dwHeight = update_rect.bottom - update_rect.top;

      /* function doing the hard work */
      _update (&src_desc, &dest_desc);

      IDirectDrawSurface_Unlock(BMP_EXTRA(pseudo_screen)->surf, NULL);
      IDirectDrawSurface_Unlock(dd_prim_surface, NULL);
   }

  End:
   _exit_gfx_critical();
}



/* build_rgb_scale_5335_table
 *  create pre-calculated tables for 16-bit to truecolor conversion
 */
static void build_rgb_scale_5335_table(void)
{
   int i, color, red, green, blue;

   if (desktop_depth == 24)
      /* 6 contiguous 256-entry tables (6k) */
      rgb_scale_5335 = malloc(sizeof(int)*1536);
   else  /* 32-bit */
      /* 2 contiguous 256-entry tables (2k) */
      rgb_scale_5335 = malloc(sizeof(int)*512);
    
   /* 1st table: r5g3 to r8g8b0 */ 
   for (i=0; i<256; i++) {
      red = _rgb_scale_5[i>>3];
      green=(i&7)<<5;

      if (green >= 68)
           green++;

       if (green >= 160)
           green++;

      color = (red<<16) | (green<<8);
      rgb_scale_5335[i] = color;

      if (desktop_depth == 24) {
         rgb_scale_5335[ 512+i] = (color>>8)+((color&0xff)<<24);
         rgb_scale_5335[1024+i] = (color>>16)+((color&0xffff)<<16);
      }
   }        

   /* 2nd table: g3b5 to r0g8b8 */
   for (i=0; i<256; i++) {
      blue = _rgb_scale_5[i&0x1f];
      green=(i>>5)<<2;

      if (green == 0x1c)
          green++;

      color = (green<<8) | blue;
      rgb_scale_5335[256+i] = color;

      if (desktop_depth == 24) {
         rgb_scale_5335[ 512+256+i] = (color>>8)+((color&0xff)<<24);
         rgb_scale_5335[1024+256+i] = (color>>16)+((color&0xffff)<<16);
      }
   }     
}



/* setup_driver_desc:
 *  Sets up the driver description string.
 */
static void setup_driver_desc(void)
{
   char tmp1[80], tmp2[80];

   usprintf(gfx_driver_desc,
       uconvert_ascii("DirectDraw, in %s, %d bpp window", tmp1),
           uconvert_ascii((same_color_depth ? "matching" : "color conversion"), tmp2),
               desktop_depth );
   
   gfx_directx_win.desc = gfx_driver_desc;
}



/* gfx_directx_set_palette_win:
 * update the palette for color conversion from 8 bit
 */
static void gfx_directx_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int n, color;

   for (n = from; n <= to; n++) {
      color = makecol_depth(desktop_depth, p[n].r<<2, p[n].g<<2, p[n].b<<2);
      allegro_palette[n] = color;

      if ((desktop_depth == 15) || (desktop_depth == 16)) {
         /* 2 pre-calculated shift tables (2k) */
         allegro_palette[PAL_SIZE+n] = color<<16; 
      }
      else if (desktop_depth == 24) {
         /* 4 pre-calculated shift tables (4k) */
         allegro_palette[PAL_SIZE+n]   = (color>>8)+((color&0xff)<<24);
         allegro_palette[PAL_SIZE*2+n] = (color>>16)+((color&0xffff)<<16);
         allegro_palette[PAL_SIZE*3+n] = color<<8;
      }
   }

   update_window(NULL);
}



/* wddwin_switch_out:
 *  handle window switched out
 */
void wddwin_switch_out(void)
{
}



/* wddwin_switch_in:
 *  handle window switched in
 */
void wddwin_switch_in(void)
{
   handle_window_size_win();
}



/* wnd_set_windowed_coop
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



/* verify_color_depth:
 * compares the color depth requested with the real color depth
 */
static int verify_color_depth (int color_depth)
{
   if (gfx_directx_compare_color_depth(color_depth) == 0) {
      /* the color depths match */ 
      update_window = update_window_hw;
   }
   else {
      /* the color depths don't match, need color conversion */
      switch (desktop_depth) {
         case 8:
            return -1;   /* no conversion from true color to 8 bit */
	    break;

         case 15:
            if (color_depth == 8) {
	       allegro_palette_size = PAL_SIZE*2;
               _update = _update_8_to_15;
            }
            else if (color_depth == 24) {
               _update = _update_24_to_15;
            }
            else if (color_depth == 32) {
               _update = _update_32_to_15;
            }
            else
               return -1;
            break;
      
         case 16:
            if (color_depth == 8) {
               allegro_palette_size = PAL_SIZE*2; 
               _update = _update_8_to_16;
            }
            else if (color_depth == 24) {
               _update = _update_24_to_16;
            }
            else if (color_depth == 32) {
               _update = _update_32_to_16;
            }
            else
               return -1;
            break;

         case 24:
            if (color_depth == 8) {
               allegro_palette_size = PAL_SIZE*4;
               _update = _update_8_to_24;
            }
            else if (color_depth == 16) {
               build_rgb_scale_5335_table();
               _update = _update_16_to_24;
            }
            else if (color_depth == 32) {
               _update = _update_32_to_24;
            }
            else
               return -1;
	    break;

         case 32:
            if (color_depth == 8) {
               allegro_palette_size = PAL_SIZE;
               _update = _update_8_to_32;
            }
            else if (color_depth == 16) {
               build_rgb_scale_5335_table();
               _update = _update_16_to_32;
            }
            else if (color_depth == 24) {
               _update = _update_24_to_32;
            }
            else
               return -1;
            break;
      } /* end of switch(desktop_depth) */

      update_window = update_window_ex;
      clipped_updating_mode = FALSE;
   }

   return 0;
}



/* gfx_directx_show_video_bitmap_win:
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



/* create_offscreen:
 */
static int create_offscreen(int w, int h, int color_depth)
{
   if (same_color_depth) {
      offscreen_surface = gfx_directx_create_surface(w, h, NULL, 1, 0, 0);
      
      if (!offscreen_surface) {
         _TRACE("Can't create offscreen surface in video memory.\n");
         offscreen_surface = gfx_directx_create_surface(w, h, NULL, 0, 0, 0);
      }
   }
   else {
      /* create pre-converted offscreen surface */
      preconv_offscreen_surface = gfx_directx_create_surface(w, h, NULL, 1, 0, 0);

      if (!preconv_offscreen_surface) {
         _TRACE("Can't create preconverted offscreen surface in video memory.\n");
         preconv_offscreen_surface = gfx_directx_create_surface(w, h, NULL, 0, 0, 0);

         if (!preconv_offscreen_surface) {
            _TRACE("Can't create preconverted offscreen surface.\n");
            return -1;
         }
      }

      offscreen_surface = gfx_directx_create_surface(w, h, dd_pixelformat, 0, 0, 0);
   }

   if (!offscreen_surface) {
      _TRACE("Can't create offscreen surface.\n");
      return -1;
   }

   return 0;
}



/* init_directx_win:
 */
static struct BITMAP *init_directx_win(int w, int h, int v_w, int v_h, int color_depth)
{
   RECT win_size;
   HRESULT hr;

   /* Flipping is impossible in windowed mode */
   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0))
      return NULL;

   /* Alignment restrictions (for color conversion) */
   if (w%4)
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

   /* adjust window */
   wnd_paint_back = TRUE;
   win_size.left = wnd_x = 32;
   win_size.right = 32 + w;
   win_size.top = wnd_y = 32;
   win_size.bottom = 32 + h;
   wnd_width = w;
   wnd_height = h;
   wnd_windowed = TRUE;

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
      goto Error;
   }

   /* create primary surface */
   if (create_primary() != 0)
      goto Error;

   /* create offscreen backbuffer */
   if (create_offscreen (w, h, color_depth) != 0)
      goto Error;

   /* create clipper */
   if (create_clipper(allegro_wnd) != 0)
      goto Error;
   hr = IDirectDrawSurface_SetClipper(dd_prim_surface, dd_clipper);
   if (FAILED(hr))
      goto Error;

   if (color_depth == 8) {
      if (same_color_depth) {
         if (create_palette(dd_prim_surface) != 0)
	    goto Error;
      }
      else {
         allegro_palette = malloc(sizeof(int) * allegro_palette_size);
         gfx_directx_win.set_palette = gfx_directx_set_palette_win;
         /* use the core library color conversion functions */ 
         gfx_directx_update_color_format(dd_prim_surface, desktop_depth);
      }
   }
   else {
       if (gfx_directx_update_color_format(offscreen_surface, color_depth) != 0)
          goto Error;
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
   dirty_lines = malloc(4*(h+1));
   memset(dirty_lines, 0, 4*(h+1));

   /* set default switching policy */
   if (same_color_depth)
      set_display_switch_mode(SWITCH_BACKGROUND);
   else
      /* color conversion adds a significant overhead, so we have to pause
         in order to let the other apps live */  
      set_display_switch_mode(SWITCH_PAUSE);

   _exit_critical();

   pseudo_screen = dd_frontbuffer;
   return dd_frontbuffer;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_win_exit(NULL);

   return NULL;
}



/* gfx_directx_exit:
 */
static void gfx_directx_win_exit(struct BITMAP *b)
{ 
   _enter_gfx_critical();
   
   free (dirty_lines);
   dirty_lines = NULL;
   
   if (b)
      clear (b);

   /* destroy the offscreen backbuffer used in windowed mode */
   gfx_directx_destroy_surf(offscreen_surface);
   offscreen_surface = NULL;
   pseudo_screen = NULL;

   /* destroy the pre-converted offscreen buffer */
   gfx_directx_destroy_surf(preconv_offscreen_surface);
   preconv_offscreen_surface = NULL;

   /* destroy the palette */
   if (allegro_palette) {
      free (allegro_palette);
      allegro_palette = NULL;
   }

   /* destroy the shift table */
   if (rgb_scale_5335) {
      free (rgb_scale_5335);
      rgb_scale_5335 = NULL;
   }

   update_window = NULL;

   /* unlink surface from bitmap */
   if (b)
      b->extra = NULL;

   _exit_gfx_critical();
   
   gfx_directx_exit(NULL);
}
