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
 *      New Windows window handling
 *
 *      By Trent Gamblin.
 *
 */

#include <allegro5/allegro5.h>
#include <allegro5/winalleg.h>
#include <process.h>

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/platform/aintwin.h"

#include "win_new.h"

static ATOM window_identifier;
static WNDCLASS window_class;
static _AL_VECTOR thread_handles = _AL_VECTOR_INITIALIZER(DWORD);

/* We have to keep track of windows some how */
typedef struct WIN_WINDOW {
   ALLEGRO_DISPLAY *display;
   HWND window;
} WIN_WINDOW;
static _AL_VECTOR win_window_list = _AL_VECTOR_INITIALIZER(WIN_WINDOW *);

/*
 * Find the top left position of the client area of a window.
 */
static void win_get_window_pos(HWND window, RECT *pos)
{
   RECT with_decorations;
   RECT adjusted;
   int top;
   int left;
   WINDOWINFO wi;

   GetWindowRect(window, &with_decorations);
   memcpy(&adjusted, &with_decorations, sizeof(RECT));

   GetWindowInfo(window, &wi);
   AdjustWindowRectEx(&adjusted, wi.dwStyle, FALSE, wi.dwExStyle);

   top = with_decorations.top - adjusted.top;
   left = with_decorations.left - adjusted.left;

   pos->top = with_decorations.top + top;
   pos->left = with_decorations.left + left;
}

HWND _al_win_create_hidden_window()
{
   HWND window = CreateWindowEx(0, 
      "ALEX", wnd_title, WS_POPUP,
      -100, -100, 0, 0,
      NULL,NULL,window_class.hInstance,0);
   ShowWindow(_al_win_compat_wnd, SW_HIDE);
   return window;
}


HWND _al_win_create_window(ALLEGRO_DISPLAY *display, int width, int height, int flags)
{
   HWND my_window;
   RECT working_area;
   RECT win_size;
   RECT pos;
   DWORD style;
   DWORD ex_style;
   WIN_WINDOW *win_window;
   WIN_WINDOW **add;
   WINDOWINFO wi;

   /* Save the thread handle for later use */
   *((DWORD *)_al_vector_alloc_back(&thread_handles)) = GetCurrentThreadId();

   SystemParametersInfo(SPI_GETWORKAREA, 0, &working_area, 0);

   win_size.left = (working_area.left + working_area.right - width) / 2;
   win_size.top = (working_area.top + working_area.bottom - height) / 2;
   win_size.right = win_size.left + width;
   win_size.bottom = win_size.top + height;

   if (!(flags & ALLEGRO_FULLSCREEN)) {
      if  (flags & ALLEGRO_RESIZABLE) {
         style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
         ex_style = WS_EX_APPWINDOW|WS_EX_OVERLAPPEDWINDOW;
      }
      else {
         style = WS_VISIBLE|WS_SYSMENU;
         ex_style = WS_EX_APPWINDOW;
      }
   }
   else {
      style = WS_POPUP|WS_VISIBLE;
      ex_style = 0;
   }

   my_window = CreateWindowEx(ex_style,
      "ALEX", wnd_title, style,
      0, 0, width, height,
      NULL,NULL,window_class.hInstance,0);
   ShowWindow(_al_win_compat_wnd, SW_HIDE);

   win_window = _AL_MALLOC(sizeof(WIN_WINDOW));
   win_window->display = display;
   win_window->window = my_window;
   add = _al_vector_alloc_back(&win_window_list);
   *add = win_window;

   GetWindowInfo(my_window, &wi);
   AdjustWindowRectEx(&win_size, wi.dwStyle, FALSE, wi.dwExStyle);

   MoveWindow(my_window, win_size.left, win_size.top,
      win_size.right - win_size.left,
      win_size.bottom - win_size.top,
      TRUE);

   win_get_window_pos(my_window, &pos);
   wnd_x = pos.left;
   wnd_y = pos.top;

   if (!(flags & ALLEGRO_RESIZABLE)) {
      /* Make the window non-resizable */
      HMENU menu;
      menu = GetSystemMenu(my_window, FALSE);
      DeleteMenu(menu, SC_SIZE, MF_BYCOMMAND);
      DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
      DrawMenuBar(my_window);
   }

   if (flags & ALLEGRO_FULLSCREEN) {
      wnd_x = 0;
      wnd_y = 0;
   }

   _al_win_wnd = my_window;
   return my_window;
}

/* This must be called by Windows drivers after their window is deleted */
void _al_win_delete_thread_handle(DWORD handle)
{
   _al_vector_find_and_delete(&thread_handles, &handle);
}

static LRESULT CALLBACK window_callback(HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam)
{
   ALLEGRO_DISPLAY *d = NULL;
   WINDOWINFO wi;
   int w;
   int h;
   int x;
   int y;
   unsigned int i, j;
   ALLEGRO_EVENT_SOURCE *es = NULL;
   RECT pos;
   ALLEGRO_SYSTEM *system = al_system_driver();
   WIN_WINDOW *win = NULL;

   if (message == _al_win_msg_call_proc) {
      return ((int (*)(void))wParam) ();
   }

   if (!system) {
      return DefWindowProc(hWnd,message,wParam,lParam); 
   }

   for (i = 0; i < system->displays._size; i++) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&system->displays, i);
      d = *dptr;
      for (j = 0; j < win_window_list._size; j++) {
         WIN_WINDOW **wptr = _al_vector_ref(&win_window_list, j);
         win = *wptr;
         if (win->display == d && win->window == hWnd) {
            es = &d->es;
            goto found;
         }
      }
   }
   found:

   if (message == _al_win_msg_suicide) {
      if (win)
         _AL_FREE(win);
      _al_vector_find_and_delete(&win_window_list, &win);
      //SendMessage(_al_win_compat_wnd, _al_win_msg_suicide, 0, 0);
      DestroyWindow(hWnd);
      return 0;
   }


   if (i != system->displays._size) {
      switch (message) { 
         case WM_MOVE:
            if (GetActiveWindow() == win_get_window()) {
               if (!IsIconic(win_get_window())) {
                  wnd_x = (short) LOWORD(lParam);
                  wnd_y = (short) HIWORD(lParam);
               }
            }
            break;
         case WM_SETCURSOR:
	    if (_win_hcursor == NULL)
	       SetCursor(NULL);
            break;
         //case WM_SETCURSOR:
         //   mouse_set_syscursor();
         //   return 1;
         break;
         case WM_ACTIVATEAPP:
            if (wParam) {
               al_set_current_display((ALLEGRO_DISPLAY *)d);
               _al_win_wnd = win->window;
               win_grab_input();
               win_get_window_pos(win->window, &pos);
               wnd_x = pos.left;
               wnd_y = pos.top;
               if (d->vt->switch_in)
                  d->vt->switch_in(d);
               return 0;
            }
            else {
               if (_al_vector_find(&thread_handles, &lParam) < 0) {
                  /*
                   * Device can be lost here, so
                   * backup textures.
                   */
                  if (d->flags & ALLEGRO_FULLSCREEN) {
                     d->vt->switch_out(d);
                  }
                  _al_win_ungrab_input();
                  return 0;
               }
            }
            break;
         case WM_CLOSE:
            _al_event_source_lock(es);
            if (_al_event_source_needs_to_generate_event(es)) {
               ALLEGRO_EVENT *event = _al_event_source_get_unused_event(es);
               if (event) {
                  event->display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
                  event->display.timestamp = al_current_time();
                  _al_event_source_emit_event(es, event);
               }
            }
            _al_event_source_unlock(es);
            return 0;
         case WM_SIZE:
            if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) {// || wParam == SIZE_MINIMIZED) {
               /* Generate a resize event if the size has changed. We cannot asynchronously
                * change the display size here yet, since the user will only know about a
                * changed size after receiving the resize event. Here we merely add the
                * event to the queue.
                */
               h = HIWORD(lParam);
               w = LOWORD(lParam);
               wi.cbSize = sizeof(WINDOWINFO);
               GetWindowInfo(win->window, &wi);
               x = wi.rcClient.left;
               y = wi.rcClient.top;
               if (d->w != w || d->h != h) {
                  _al_event_source_lock(es);
                  if (_al_event_source_needs_to_generate_event(es)) {
                     ALLEGRO_EVENT *event = _al_event_source_get_unused_event(es);
                     if (event) {
                        event->display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
                        event->display.timestamp = al_current_time();
                        event->display.x = x;
                        event->display.y = y;
                        event->display.width = w;
                        event->display.height = h;
                        _al_event_source_emit_event(es, event);
                     }
                  }

                  /* Generate an expose event. */
                  if (_al_event_source_needs_to_generate_event(es)) {
                     ALLEGRO_EVENT *event = _al_event_source_get_unused_event(es);
                     if (event) {
                        event->display.type = ALLEGRO_EVENT_DISPLAY_EXPOSE;
                        event->display.timestamp = al_current_time();
                        event->display.x = x;
                        event->display.y = y;
                        event->display.width = w;
                        event->display.height = h;
                        _al_event_source_emit_event(es, event);
                     }
                  }
                  _al_event_source_unlock(es);
               }
               return 0;
            }
            break;
      } 
   }

   return DefWindowProc(hWnd,message,wParam,lParam); 
}

int _al_win_init_window()
{
   if (_al_win_msg_call_proc == 0 && _al_win_msg_suicide == 0) {
      _al_win_msg_call_proc = RegisterWindowMessage("Allegro call proc");
      _al_win_msg_suicide = RegisterWindowMessage("Allegro window suicide");
   }

   // Create A Window Class Structure 
   window_class.cbClsExtra = 0;
   window_class.cbWndExtra = 0; 
   window_class.hbrBackground = NULL;
   window_class.hCursor = NULL; 
   window_class.hIcon = NULL; 
   window_class.hInstance = GetModuleHandle(NULL);
   window_class.lpfnWndProc = window_callback;
   window_class.lpszClassName = "ALEX";
   window_class.lpszMenuName = NULL;
   window_class.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;

   window_identifier = RegisterClass(&window_class);

   return true;
}

void _al_win_ungrab_input()
{
   wnd_schedule_proc(key_dinput_unacquire);
}



static HICON win_create_icon_from_old_bitmap(struct BITMAP *sprite)
{
   int mask_color;
   int x, y;
   int width, height;
   HDC h_dc;
   HDC h_and_dc;
   HDC h_xor_dc;
   ICONINFO iconinfo;
   HBITMAP and_mask;
   HBITMAP xor_mask;
   HBITMAP hOldAndMaskBitmap;
   HBITMAP hOldXorMaskBitmap;
   HICON hicon;
   HWND allegro_wnd = win_get_window();

   /* Create bitmap */
   h_dc = GetDC(allegro_wnd);
   h_xor_dc = CreateCompatibleDC(h_dc);
   h_and_dc = CreateCompatibleDC(h_dc);

   width = sprite->w;
   height = sprite->h;

   /* Prepare AND (monochrome) and XOR (colour) mask */
   and_mask = CreateBitmap(width, height, 1, 1, NULL);
   xor_mask = CreateCompatibleBitmap(h_dc, width, height);
   hOldAndMaskBitmap = SelectObject(h_and_dc, and_mask);
   hOldXorMaskBitmap = SelectObject(h_xor_dc, xor_mask);

   /* Create transparent cursor */
   for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
	 SetPixel(h_and_dc, x, y, WINDOWS_RGB(255, 255, 255));
	 SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
      }
   }
   draw_to_hdc(h_xor_dc, sprite, 0, 0);
   mask_color = bitmap_mask_color(sprite);

   /* Make cursor background transparent */
   for (y = 0; y < sprite->h; y++) {
      for (x = 0; x < sprite->w; x++) {
	 if (getpixel(sprite, x, y) != mask_color) {
	    /* Don't touch XOR value */
	    SetPixel(h_and_dc, x, y, 0);
	 }
	 else {
	    /* No need to touch AND value */
	    SetPixel(h_xor_dc, x, y, WINDOWS_RGB(0, 0, 0));
	 }
      }
   }

   SelectObject(h_and_dc, hOldAndMaskBitmap);
   SelectObject(h_xor_dc, hOldXorMaskBitmap);
   DeleteDC(h_and_dc);
   DeleteDC(h_xor_dc);
   ReleaseDC(allegro_wnd, h_dc);

   iconinfo.fIcon = TRUE;
   iconinfo.hbmMask = and_mask;
   iconinfo.hbmColor = xor_mask;

   hicon = CreateIconIndirect(&iconinfo);

   DeleteObject(and_mask);
   DeleteObject(xor_mask);

   return hicon;
}



void _al_win_set_display_icon(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bmp)
{
   HICON scaled_icon;
   HWND hwnd;
   HICON old_small;
   HICON old_big;

   /* Convert to BITMAP */
   BITMAP *oldbmp = create_bitmap_ex(32,
      al_get_bitmap_width(bmp), al_get_bitmap_height(bmp));
   
   BITMAP *scaled_bmp = create_bitmap_ex(32, 16, 16);

   int x, y;
   for (y = 0; y < oldbmp->h; y++) {
      for (x = 0; x < oldbmp->w; x++) {
         ALLEGRO_COLOR color = al_get_pixel(bmp, x, y);
         unsigned char r, g, b, a;
         int oldcolor;
         al_unmap_rgba(color, &r, &g, &b, &a);
         if (a == 0) {
            oldcolor = makecol32(255, 0, 255);
         }
         else
            oldcolor = makecol32(r, g, b);
         putpixel(oldbmp, x, y, oldcolor);
      }
   }

   stretch_blit(oldbmp, scaled_bmp, 0, 0, oldbmp->w, oldbmp->h,
   	0, 0, scaled_bmp->w, scaled_bmp->h);

   scaled_icon = win_create_icon_from_old_bitmap(scaled_bmp);

   hwnd = win_get_window();

   /* Set new icons and destroy old */
   old_small = (HICON)SendMessage(hwnd, WM_SETICON,
      ICON_SMALL, (LPARAM)scaled_icon);
   old_big = (HICON)SendMessage(hwnd, WM_SETICON,
      ICON_BIG, (LPARAM)scaled_icon);

   if (old_small)
      DestroyIcon(old_small);
   if (old_big)
      DestroyIcon(old_big);

   destroy_bitmap(oldbmp);
   destroy_bitmap(scaled_bmp);
}

