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

/* functions in update.s */
extern void _update_32_to_16 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc);
extern void _update_32_to_15 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc);
extern void _update_16_to_32 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc);
extern void _update_8_to_32 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc);
extern void _update_8_to_16 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc);


static struct BITMAP *gfx_directx_create_video_bitmap_win(int width, int height);
static void gfx_directx_destroy_video_bitmap_win(struct BITMAP *bitmap);
static void gfx_directx_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync);
static int wnd_set_windowed_coop(void);
static void create_offscreen (int w, int h, int color_depth);
static int verify_color_depth (int color_depth);
static int gfx_directx_show_video_bitmap_win(struct BITMAP *bitmap);
static void gfx_directx_unlock_win(BITMAP *bmp);
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

static int pixel_match[] = { 8, 15, 15, 16, 16, 24, 24, 32, 32, 0 };

DDPIXELFORMAT pixel_format[] = {
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


LPDDPIXELFORMAT dd_pixelformat = NULL;  /* pixel format used */

/* offscreen surface that will be blitted to the window:*/
LPDIRECTDRAWSURFACE dd_offscreen = NULL;
BITMAP* pseudo_screen = NULL;  /* for page-flipping */
BOOL app_active = TRUE;
RECT window_rect;
BOOL same_color_depth;
int* allegro_palette = NULL;
void (*update_window) (RECT* rect);
void (*_update) (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc);
int* dirty_lines; /* used in WRITE_BANK */
static int desktop_depth = 0;
static int desk_r, desk_g, desk_b;
static int reused_screen;
static GFX_VTABLE _special_vtable; /* special vtable for offscreen bitmap */


/* handle_window_size_win:
 *  updates window_rect if window is moved or resized
 */
void handle_window_size_win(void)
{
   /* update window_rect */
   GetClientRect(allegro_wnd, &window_rect);
   ClientToScreen(allegro_wnd, (LPPOINT)&window_rect);
   ClientToScreen(allegro_wnd, (LPPOINT)&window_rect + 1);
}


/* update_window_hw
 * function synced with the vertical refresh
 */
void update_window_hw (RECT* rect)
{
   RECT update_rect;
   HRESULT hr;
   
   if (!app_active) return;

   if (rect) {
      update_rect.left = window_rect.left + rect->left;
      update_rect.top = window_rect.top + rect->top;
      update_rect.right = window_rect.left + rect->right;
      update_rect.bottom = window_rect.top + rect->bottom;
   }
   else
      update_rect = window_rect;
   
   /* blit offscreen backbuffer to the window */
   hr = IDirectDrawSurface_Blt(dd_prim_surface,
        		       &update_rect,
			       BMP_EXTRA(pseudo_screen)->surf,
			       rect, 0, NULL);
}


/* update_window_ex:
 * converts between two color formats
 */
void update_window_ex (RECT* rect)
{
   DDSURFACEDESC src_desc, dest_desc;
   HRESULT hr;
   RECT update_rect;

   if (!app_active) return;
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
   
   hr = IDirectDrawSurface_Lock(dd_prim_surface, &update_rect, &dest_desc, DDLOCK_WAIT, NULL);
   if (FAILED(hr)) {
      return;
   }

   hr = IDirectDrawSurface_Lock(BMP_EXTRA(pseudo_screen)->surf, rect, &src_desc, DDLOCK_WAIT, NULL);
   if (FAILED(hr)) {
      IDirectDrawSurface_Unlock(dd_prim_surface, NULL);
      return;
   }

   src_desc.dwWidth = update_rect.right - update_rect.left;
   src_desc.dwHeight = update_rect.bottom - update_rect.top;
   
   /* function doing the hard work */
   _update (&src_desc, &dest_desc);

   IDirectDrawSurface_Unlock(BMP_EXTRA(pseudo_screen)->surf, NULL);
   IDirectDrawSurface_Unlock(dd_prim_surface, NULL);
}


/* gfx_directx_create_video_bitmap_win:
 */
static struct BITMAP *gfx_directx_create_video_bitmap_win(int width, int height)
{
   LPDIRECTDRAWSURFACE surf;

   /* can we reuse the screen bitmap for this? */
   if ((!reused_screen) && (screen->w == width) && (screen->h == height)) {
      reused_screen = TRUE;
      return screen;
   }

   /* create DirectDraw surface */
   surf = gfx_directx_create_surface(width, height, _color_depth, 0, 0, 0);
   if (surf == NULL)
      return NULL;

   /* create Allegro bitmap for surface */
   return make_directx_bitmap(surf, width, height, _color_depth, BMP_ID_VIDEO);
}



/* gfx_directx_destroy_video_bitmap_win:
 */
static void gfx_directx_destroy_video_bitmap_win(struct BITMAP *bitmap)
{
   if (bitmap == screen) {
      reused_screen = FALSE;
      return;
   }

   if (BMP_EXTRA(bitmap))
      gfx_directx_destroy_surf(BMP_EXTRA(bitmap)->surf);
   release_directx_bitmap(bitmap);
   bitmap->id &= ~BMP_ID_VIDEO;
   destroy_bitmap(bitmap);
}


/* gfx_directx_set_palette_win:
 * update the palette for color conversion from 8 bit
 */
static void gfx_directx_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int n;

   switch (desktop_depth) {
   
      case 15:
         for (n = from; n <= to; n++)
            allegro_palette[n] = ( ((p[n].r>>1) << desk_r) |
                                   ((p[n].g>>1) << desk_g) |
                                   ((p[n].b>>1) << desk_b) );
            break;
      case 16:
         for (n = from; n <= to; n++)
            allegro_palette[n] = ( ((p[n].r>>1) << desk_r) |
                                   ( p[n].g     << desk_g) |
                                   ((p[n].b>>1) << desk_b) );
            break;
      case 32:
         for (n = from; n <= to; n++)
            allegro_palette[n] = ( ((p[n].r<<2) << desk_r) |
                                   ((p[n].g<<2) << desk_g) |
                                   ((p[n].b<<2) << desk_b) );
   }
   update_window(NULL);
}


/* wddwin_switch_out:
 *  handle window switched out
 */
void wddwin_switch_out(void)
{
   if (app_active) {
      app_active = FALSE;
   }
}



/* wddwin_switch_in:
 *  handle window switched in
 */
void wddwin_switch_in(void)
{
   handle_window_size_win();
   if (!app_active)
      app_active = TRUE;
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

   switch (desktop_depth) {

      case 8:  /* colors will be incorrect */
	 if (color_depth == 8) {
	    same_color_depth = TRUE;
	 }
         else return -1;   /* no conversion from true color to 8 bit */
	 break;

      case 15:
	 if (color_depth == 15) {
	    same_color_depth = TRUE;
	 }
	 else if (color_depth == 32) {
	    _update = _update_32_to_15;
	    same_color_depth = FALSE;
	 }
	 else if (color_depth == 8) {
	    _update = _update_8_to_16;
	    same_color_depth = FALSE;
	 }
         else return -1;
	 break;
      
      case 16:
	 if (color_depth == 16) {
	    same_color_depth = TRUE;
	 }
	 else if (color_depth == 32) {
	    _update = _update_32_to_16;
	    same_color_depth = FALSE;
	 }
	 else if (color_depth == 8) {
	    _update = _update_8_to_16;
	    same_color_depth = FALSE;
	 }
         else return -1;
	 break;

      case 24:
      	 if (color_depth == 24) {
	    same_color_depth = TRUE;
	 }
	 else return -1;
	 break;

      case 32:
	 if (color_depth == 32) {
	    same_color_depth = TRUE;
	 }
	 else if (color_depth == 16) {
	    _update = _update_16_to_32;
	    same_color_depth = FALSE;
	 }
	 else if (color_depth == 8) {
	    _update = _update_8_to_32;
	    same_color_depth = FALSE;
	 }
         else return -1;
   }
   if (same_color_depth)
      update_window = update_window_hw;
   else if (cpu_mmx) update_window = update_window_ex;
   else return -1;       // TODO: hacer una version en C
 
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



/* gfx_directx_unlock_win:
 */
static void gfx_directx_unlock_win(BITMAP *bmp)
{
   gfx_directx_unlock(bmp);
   update_window(NULL);
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


/* create_offscreen:
 */
static void create_offscreen(int w, int h, int color_depth)
{
   DDSURFACEDESC surf_desc;
   DDPIXELFORMAT temp_pixel_format;
   HRESULT hr;
   int i;
   int shift_r, shift_g, shift_b;

   /* describe surface characteristics */
   surf_desc.dwSize = sizeof(surf_desc);
   surf_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
   surf_desc.dwHeight = h;
   surf_desc.dwWidth = w;
   if (same_color_depth) {
      surf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
      hr = IDirectDraw_CreateSurface(directdraw, &surf_desc, &dd_offscreen, NULL);
      if (FAILED(hr)) dd_offscreen = NULL;
      else gfx_directx_update_color_format(color_depth);
   } 
   else {
      /* get pixel format of primary surface */
      temp_pixel_format.dwSize = sizeof(DDPIXELFORMAT);
      IDirectDrawSurface_GetPixelFormat (dd_prim_surface, &temp_pixel_format);
      desk_r = _get_color_shift(temp_pixel_format.dwRBitMask);
      desk_g = _get_color_shift(temp_pixel_format.dwGBitMask);
      desk_b = _get_color_shift(temp_pixel_format.dwBBitMask);
      surf_desc.dwFlags |= DDSD_PIXELFORMAT;
      /* if not the same color depth as the primary, must be in system memory (don't ask me) */
      surf_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
      for (i=0 ; pixel_match[i] ; i++) {
         if (pixel_match[i] == color_depth) {
            surf_desc.ddpfPixelFormat = pixel_format[i];
            if ((temp_pixel_format.dwRBitMask & pixel_format[i].dwRBitMask) ||
                (temp_pixel_format.dwBBitMask & pixel_format[i].dwBBitMask) ||
                 color_depth == 8) {
               hr = IDirectDraw_CreateSurface(directdraw, &surf_desc, &dd_offscreen, NULL);
               if (!FAILED(hr)) break;
            }
	 }
      }
      if (pixel_match[i] == 0) {
	 _TRACE ("No se ha podido crear dd_offscreen\n");
	 dd_offscreen = NULL;
      }
      else {
	 /* update color format */
	 dd_pixelformat = &(pixel_format[i]);
	 shift_r = _get_color_shift(surf_desc.ddpfPixelFormat.dwRBitMask);
	 shift_g = _get_color_shift(surf_desc.ddpfPixelFormat.dwGBitMask);
	 shift_b = _get_color_shift(surf_desc.ddpfPixelFormat.dwBBitMask);
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
      }
   }
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
   window_rect = win_size;
   wnd_windowed = TRUE;
   set_display_switch_mode(SWITCH_BACKGROUND);

   AdjustWindowRect(&win_size, GetWindowLong(allegro_wnd, GWL_STYLE), FALSE);
   MoveWindow(allegro_wnd, win_size.left, win_size.top,
   win_size.right - win_size.left, win_size.bottom - win_size.top, TRUE);
   if (!same_color_depth)
      SetWindowPos(allegro_wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

   /* create primary surface */
   if (create_primary(w, h, color_depth) != 0)
      goto Error;

   /* create offscreen backbuffer */
   create_offscreen (w, h, color_depth);
   if (dd_offscreen == NULL)
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
         allegro_palette = malloc (sizeof(int) * PAL_SIZE);
         gfx_directx_win.set_palette = gfx_directx_set_palette_win;
      }
   }

   /* setup Allegro gfx driver */
   if (!same_color_depth) {  /* use system bitmaps instead of video */
      gfx_directx_win.create_video_bitmap = gfx_directx_create_video_bitmap_win;
      gfx_directx_win.destroy_video_bitmap = gfx_directx_destroy_video_bitmap_win;
   }
   if (setup_driver(&gfx_directx_win, w, h, color_depth) != 0)
      goto Error;
   dd_frontbuffer = make_directx_bitmap(dd_offscreen, w, h, color_depth, BMP_ID_VIDEO);

   enable_acceleration(&gfx_directx_win);
   memcpy (&_special_vtable, &_screen_vtable, sizeof (GFX_VTABLE));
   _special_vtable.release = gfx_directx_unlock_win;
   _special_vtable.unwrite_bank = gfx_directx_unwrite_bank_win;
   dd_frontbuffer->vtable = &_special_vtable;
   dd_frontbuffer->write_bank = gfx_directx_write_bank_win;

   dirty_lines = malloc(4*h);
   memset(dirty_lines, 0, 4*h);
   app_active = TRUE;
   reused_screen = FALSE;

   _exit_critical();

   pseudo_screen = dd_frontbuffer;
   return dd_frontbuffer;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_exit(NULL);

   return NULL;
}

/* gfx_directx_exit:
 */
static void gfx_directx_win_exit(struct BITMAP *b)
{ 
   _enter_gfx_critical();
   
   if (app_active) {
      app_active = FALSE;
   }

   free (dirty_lines);
   
   dd_pixelformat = NULL;

   if (b)
      clear (b);

   /* destroy the offscreen backbuffer used in windowed mode */
   if (dd_offscreen) {
      IDirectDrawSurface_Release(dd_offscreen);
      dd_offscreen = NULL;
   }

   /* destroy the palette */
   if (allegro_palette) {
      free (allegro_palette);
      allegro_palette = NULL;
   }

   /* unlink surface from bitmap */
   if (b)
      b->extra = NULL;

   _exit_gfx_critical();
   
   gfx_directx_exit(NULL);
}



