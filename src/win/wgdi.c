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
 *      Windowed mode gfx driver using gdi.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



static struct BITMAP *gfx_gdi_dblbuf_init(int w, int h, int v_w, int v_h, int color_depth);
static void gfx_gdi_dblbuf_exit(struct BITMAP *b);
static void gfx_gdi_set_palette(struct RGB *p, int from, int to, int vsync);
static void gfx_gdi_vsync(void);


GFX_DRIVER gfx_gdi =
{
   GFX_GDI,
   empty_string,
   empty_string,
   "GDI",
   gfx_gdi_dblbuf_init,
   gfx_gdi_dblbuf_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_gdi_vsync,
   gfx_gdi_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // gfx_directx_poll_scroll,
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL, NULL, NULL, NULL, NULL, NULL,
   NULL,                        // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                        // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                        // AL_METHOD(void, hide_mouse, (void));
   NULL,                        // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                       // AL_METHOD(void, save_video_state, (void*));
   NULL,                       // AL_METHOD(void, restore_video_state, (void*));
   0, 0,                        // int w, h;                     /* physical (not virtual!) screen size */
   TRUE,                        // int linear;                   /* true if video memory is linear */
   0,                           // long bank_size;               /* bank size, in bytes */
   0,                           // long bank_gran;               /* bank granularity, in bytes */
   0,                           // long vid_mem;                 /* video memory size, in bytes */
   0,                           // long vid_phys_base;           /* physical address of video memory */
};

static UINT _render_timer = 0;
static WNDPROC _old_window_proc = NULL;
static PALETTE _palette;
static HANDLE _vsync_event;
#define RENDER_DELAY (1000/70)



/* _render_proc:
 *  timer proc that repaints the window
 */
static void CALLBACK _render_proc(HWND hwnd, UINT msg, UINT id_event, DWORD time)
{
   RedrawWindow(allegro_wnd, NULL, NULL, RDW_INVALIDATE);
}



/* _window_proc:
 *  custom window proc to blit the screen bitmap
 */
static LRESULT CALLBACK _window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
   RECT update_rect;
   PAINTSTRUCT ps; 
   HDC hdc;

   if (msg==WM_PAINT && screen!=NULL)
   {
      if (GetUpdateRect(hwnd, &update_rect, FALSE))
      {
	 hdc = BeginPaint(hwnd, &ps);

	 if (_color_depth==8)
	 {
	    set_palette_to_hdc(hdc, _palette);
	 }

	 draw_to_hdc(hdc, screen, 0, 0);

	 EndPaint(hwnd, &ps);

	 PulseEvent(_vsync_event);
      } 
   }

   return CallWindowProc(_old_window_proc, hwnd, msg, wparam, lparam);
}



/* gfx_gdi_dblbuf_init:
 */
static struct BITMAP *gfx_gdi_dblbuf_init(int w, int h, int v_w, int v_h, int color_depth)
{
   wnd_paint_back = TRUE;

   gfx_gdi.w = w;
   gfx_gdi.h = h;

   /* virtual screen are not supported */
   if ((v_w!=0 && v_w!=w) || (v_h!=0 && v_h!=h)) return NULL;

   /* resize window */
   SetWindowPos(allegro_wnd, HWND_NOTOPMOST, 
      (GetSystemMetrics(SM_CXSCREEN)-w)/2, (GetSystemMetrics(SM_CYSCREEN)-h)/2, 
      w, h, 0);

   /* install custom window proc */
   _old_window_proc = (WNDPROC)SetWindowLong(allegro_wnd, GWL_WNDPROC, (LONG)_window_proc);

   /* create render timer */
   _render_timer = SetTimer(allegro_wnd, 0, RENDER_DELAY, (TIMERPROC)_render_proc);
   _vsync_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   wnd_windowed = TRUE;
   set_display_switch_mode(SWITCH_BACKGROUND);

   return create_bitmap_ex(color_depth, w, h);
}



/* gfx_gdi_dblbuf_exit:
 */
static void gfx_gdi_dblbuf_exit(struct BITMAP *b)
{
   /* stop timer */
   KillTimer(allegro_wnd, 0);
   CloseHandle(_vsync_event);

   /* restore old window proc */
   SetWindowLong(allegro_wnd, GWL_WNDPROC, (LONG)_old_window_proc);

   /* before restoring video mode, hide window */
   wnd_paint_back = FALSE;
   restore_window_style();
   SetWindowPos(allegro_wnd, HWND_TOPMOST,
		-100, -100, 0, 0, SWP_SHOWWINDOW); 
}



/* gfx_gdi_set_palette:
 */
static void gfx_gdi_set_palette(struct RGB *p, int from, int to, int vsync)
{
   int c;

   for (c=from; c<=to; c++)
      _palette[c] = p[c]; 
}



/* gfx_gdi_vsync:
 */
static void gfx_gdi_vsync(void)
{
   WaitForSingleObject(_vsync_event, INFINITE);
}

