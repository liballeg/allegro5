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
#include <process.h>
#include <windows.h>

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/platform/aintwin.h"


#define PREFIX_I                "al-newin INFO: "
#define PREFIX_W                "al-newin WARNING: "
#define PREFIX_E                "al-newin ERROR: "

static WNDCLASS window_class;

static bool resize_postponed = false;

static HWND capture_window = 0;


UINT _al_win_msg_call_proc = 0;
UINT _al_win_msg_suicide = 0;



/*
 * Find the top left position of the client area of a window.
 */
static void get_window_pos(HWND window, RECT *pos)
{
   RECT with_decorations;
   RECT adjusted;
   int top;
   int left;
   WINDOWINFO wi;

   wi.cbSize = sizeof(WINDOWINFO);

   GetWindowRect(window, &with_decorations);
   memcpy(&adjusted, &with_decorations, sizeof(RECT));

   GetWindowInfo(window, &wi);
   AdjustWindowRectEx(&adjusted, wi.dwStyle, false, wi.dwExStyle);

   top = with_decorations.top - adjusted.top;
   left = with_decorations.left - adjusted.left;

   pos->top = with_decorations.top + top;
   pos->left = with_decorations.left + left;
}

HWND _al_win_create_hidden_window()
{
   HWND window = CreateWindowEx(0, 
      "ALEX", "hidden", WS_POPUP,
      -5000, -5000, 0, 0,
      NULL,NULL,window_class.hInstance,0);
   return window;
}


HWND _al_win_create_window(ALLEGRO_DISPLAY *display, int width, int height, int flags)
{
   HWND my_window;
   RECT win_size;
   DWORD style;
   DWORD ex_style;
   int pos_x, pos_y;
   bool center = false;
   ALLEGRO_MONITOR_INFO info;
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;
   WINDOWINFO wi;
   int bw, bh;

   wi.cbSize = sizeof(WINDOWINFO);

   if (!(flags & ALLEGRO_FULLSCREEN)) {
      if  (flags & ALLEGRO_RESIZABLE) {
         style = WS_OVERLAPPEDWINDOW;
         ex_style = WS_EX_APPWINDOW|WS_EX_OVERLAPPEDWINDOW;
      }
      else {
         style = WS_SYSMENU;
         ex_style = WS_EX_APPWINDOW;
      }
   }
   else {
      style = WS_POPUP;
      ex_style = WS_EX_APPWINDOW;
   }

   al_get_new_window_position(&pos_x, &pos_y);
   if (pos_x == INT_MAX) {
   	pos_x = pos_y = 0;
      if (!(flags & ALLEGRO_FULLSCREEN)) {
         center = true;
      }
   }

   if (center) {
      //int a = al_get_current_video_adapter();
      int a = win_display->adapter;

      if (a == -1) {
         ALLEGRO_SYSTEM *sys = al_system_driver();
         unsigned int num;
         bool *is_fullscreen;
         unsigned int i;
         unsigned int fullscreen_found = 0;
         num = al_get_num_video_adapters();
         is_fullscreen = _AL_MALLOC(sizeof(bool)*num);
         memset(is_fullscreen, 0, sizeof(bool)*num);
         for (i = 0; i < sys->displays._size; i++) {
            ALLEGRO_DISPLAY **dptr = _al_vector_ref(&sys->displays, i);
            ALLEGRO_DISPLAY *d = *dptr;
            if (d->flags & ALLEGRO_FULLSCREEN) {
               ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)d;
               is_fullscreen[win_display->adapter] = true;
               fullscreen_found++;
            }
         }
         if (fullscreen_found && fullscreen_found < num) {
            for (i = 0; i < num; i++) {
               if (is_fullscreen[i] == false) {
                  a = i;
                  break;
               }
            }
         }
         else
            a = 0;
         _AL_FREE(is_fullscreen);
      }

      al_set_current_video_adapter(a);

      al_get_monitor_info(a, &info);

      win_size.left = info.x1 + (info.x2 - info.x1 - width) / 2;
      win_size.right = win_size.left + width;
      win_size.top = info.y1 + (info.y2 - info.y1 - height) / 2;
      win_size.bottom = win_size.top + height;

      AdjustWindowRectEx(&win_size, style, false, ex_style);

      pos_x = win_size.left;
      pos_y = win_size.top;
   }

   my_window = CreateWindowEx(ex_style,
      "ALEX", "Allegro", style,
      pos_x, pos_y, width, height,
      NULL,NULL,window_class.hInstance,0);

   GetWindowInfo(my_window, &wi);
      
   bw = (wi.rcClient.left - wi.rcWindow.left) + (wi.rcWindow.right - wi.rcClient.right),
   bh = (wi.rcClient.top - wi.rcWindow.top) + (wi.rcWindow.bottom - wi.rcClient.bottom),

   SetWindowPos(my_window, 0, pos_x-bw/2, pos_y-bh/2,
      width+bw,
      height+bh,
      SWP_NOMOVE | SWP_NOZORDER);

   if (flags & ALLEGRO_NOFRAME) {
      SetWindowLong(my_window, GWL_STYLE, WS_VISIBLE);
      SetWindowLong(my_window, GWL_EXSTYLE, WS_EX_APPWINDOW);
      SetWindowPos(my_window, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
   }

   ShowWindow(my_window, SW_SHOW);

   if (!(flags & ALLEGRO_RESIZABLE) && !(flags & ALLEGRO_FULLSCREEN)) {
      /* Make the window non-resizable */
      HMENU menu;
      menu = GetSystemMenu(my_window, false);
      DeleteMenu(menu, SC_SIZE, MF_BYCOMMAND);
      DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
      DrawMenuBar(my_window);
   }

   return my_window;
}


HWND _al_win_create_faux_fullscreen_window(LPCTSTR devname, ALLEGRO_DISPLAY *display, 
	int x1, int y1, int width, int height, int refresh_rate, int flags)
{ 
   HWND my_window;
   DWORD style;
   DWORD ex_style;
   DEVMODE mode;
   LONG temp;

   (void)flags;

   style = WS_VISIBLE;
   ex_style = WS_EX_TOPMOST;

   my_window = CreateWindowEx(ex_style,
      "ALEX", "Allegro", style,
      x1, y1, width, height,
      NULL,NULL,window_class.hInstance,0);

   temp = GetWindowLong(my_window, GWL_STYLE);
   temp &= ~WS_CAPTION;
   SetWindowLong(my_window, GWL_STYLE, temp);
   SetWindowPos(my_window, 0, 0,0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

   /* Go fullscreen */
   memset(&mode, 0, sizeof(DEVMODE));
   mode.dmSize = sizeof(DEVMODE);
   mode.dmDriverExtra = 0;
   mode.dmBitsPerPel = al_get_pixel_format_bits(display->format);
   mode.dmPelsWidth = display->w;
   mode.dmPelsHeight = display->h;
   mode.dmDisplayFlags = 0;
   mode.dmDisplayFrequency = refresh_rate;
   mode.dmPosition.x = x1;
   mode.dmPosition.y = y1;
   mode.dmFields = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT|DM_DISPLAYFLAGS|
   	DM_DISPLAYFREQUENCY|DM_POSITION;

   ChangeDisplaySettingsEx(devname, &mode, NULL, 0, NULL/*CDS_FULLSCREEN*/);
   
   return my_window;
}


/* _al_win_grab_input:
 * Makes the passed display grab the input. All consequent input events will be
 * generated the this display's window. The display's window must the the
 * foreground window.
 */
void _al_win_grab_input(ALLEGRO_DISPLAY_WIN *win_disp)
{
   _al_win_wnd_schedule_proc(win_disp->window,
                             _al_win_key_dinput_grab,
                             win_disp);
   _al_win_wnd_schedule_proc(win_disp->window,
                             _al_win_mouse_dinput_grab,
                             win_disp);
   _al_win_wnd_schedule_proc(win_disp->window,
                             _al_win_joystick_dinput_grab,
                             win_disp);
}


/* _al_win_unacquire_input: [window thread]
 * Called when the window looses focus to release the input.
 */
static void unacquire_input(ALLEGRO_DISPLAY_WIN *win_disp)
{
   _al_win_key_dinput_unacquire(win_disp);
   _al_win_mouse_dinput_unacquire(win_disp);
   _al_win_joystick_dinput_unacquire(win_disp);
}


static void postpone_thread_proc(void *arg)
{
   HWND window = (HWND)arg;

   Sleep(500);

   SendMessage(window, WM_USER+0, 0, 0);
}


static void postpone_resize(HWND window)
{
   _beginthread(postpone_thread_proc, 0, (void *)window);
}


static LRESULT CALLBACK window_callback(HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam)
{
   ALLEGRO_DISPLAY *d = NULL;
   ALLEGRO_DISPLAY_WIN *win_display = NULL;
   WINDOWINFO wi;
   int w;
   int h;
   int x;
   int y;
   unsigned int i;
   ALLEGRO_EVENT_SOURCE *es = NULL;
   ALLEGRO_SYSTEM *system = al_system_driver();

   wi.cbSize = sizeof(WINDOWINFO);

   if (message == _al_win_msg_call_proc) {
      ((void (*)(void*))wParam) ((void*)lParam);
      return 0;
   }

   if (!system) {
      return DefWindowProc(hWnd,message,wParam,lParam); 
   }

   if (message == _al_win_msg_suicide && wParam) {
      win_display = (ALLEGRO_DISPLAY_WIN*)wParam;
      win_display->end_thread = true;
      DestroyWindow(hWnd);
      return 0;
   }

   for (i = 0; i < system->displays._size; i++) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&system->displays, i);
      d = *dptr;
      win_display = (void*)d;
      if (win_display->window == hWnd) {
         es = &d->es;
         break;
      }
   }

   if (i == system->displays._size)
      return DefWindowProc(hWnd,message,wParam,lParam); 

   if (message == _al_win_msg_suicide) {
      win_display->end_thread = true;
      DestroyWindow(hWnd);
      return 0;
   }

   switch (message) {
      case WM_MOUSEMOVE: {
         /* Track the mouse as it enters/exits windows */
         int mx = lParam & 0xFFFF;
         int my = (lParam >> 16) & 0xFFFF;
         if (!capture_window || capture_window != hWnd) {
            //SetCapture(hWnd);
            // emit event
            if (al_is_mouse_installed()) {
               ALLEGRO_MOUSE *mouse = al_get_mouse();
               if (mouse) {
                  capture_window = hWnd;
                  es = (ALLEGRO_EVENT_SOURCE *)mouse;
                  if (_al_event_source_needs_to_generate_event(es)) {
                     ALLEGRO_EVENT event;
                     _al_event_source_lock(es);
                        event.type = ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY;
                        event.mouse.x = mx;
                        event.mouse.y = my;
                        event.mouse.z = 0;
                        event.mouse.timestamp = al_current_time();
                        _al_event_source_emit_event(es, &event);
                     _al_event_source_unlock(es);
                  }
               }
            }
         }
         else if (mx < 0 || my < 0 || mx >= d->w || my >= d->h) {
            //ReleaseCapture();
            // emit event
            if (al_is_mouse_installed()) {
               ALLEGRO_MOUSE *mouse = al_get_mouse();
               if (mouse) {
                  capture_window = 0;
                  es = (ALLEGRO_EVENT_SOURCE *)mouse;
                  if (_al_event_source_needs_to_generate_event(es)) {
                     ALLEGRO_EVENT event;
                     _al_event_source_lock(es);
                        event.type = ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY;
                        event.mouse.x = mx;
                        event.mouse.y = my;
                        event.mouse.z = 0;
                        event.mouse.timestamp = al_current_time();
                        _al_event_source_emit_event(es, &event);
                     _al_event_source_unlock(es);
                  }
               }
            }
         }
         break;
      }
      case WM_SYSCOMMAND: {
         if (_al_win_disable_screensaver &&
               (wParam == SC_MONITORPOWER || wParam == SC_SCREENSAVE)) {
            return 0;
         }
         break;
      }
      case WM_PAINT: {
         if ((win_display->display.flags & ALLEGRO_GENERATE_EXPOSE_EVENTS) &&
                  _al_event_source_needs_to_generate_event(es)) {
            RECT r;
            HRGN hrgn;
            GetWindowRect(win_display->window, &r);
            hrgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
            if (GetUpdateRgn(win_display->window, hrgn, false) != ERROR) {
               PAINTSTRUCT ps;
               DWORD size;
               LPRGNDATA rgndata;
               int n;
               int i;
               RECT *rects;
               BeginPaint(win_display->window, &ps);
               size = GetRegionData(hrgn, 0, NULL);
               rgndata = _AL_MALLOC(size);
               GetRegionData(hrgn, size, rgndata);
               n = rgndata->rdh.nCount;
               rects = (RECT *)rgndata->Buffer;
               //GetWindowInfo(win_display->window, &wi);
               _al_event_source_lock(es);
               for (i = 0; i < n; i++) {
                  ALLEGRO_EVENT event;
                  event.display.type = ALLEGRO_EVENT_DISPLAY_EXPOSE;
                  event.display.timestamp = al_current_time();
                  event.display.x = rects[i].left;
                  event.display.y = rects[i].top;
                  event.display.width = rects[i].right - rects[i].left;
                  event.display.height = rects[i].bottom - rects[i].top;
                  _al_event_source_emit_event(es, &event);
               }
               _al_event_source_unlock(es);
               _AL_FREE(rgndata);
               EndPaint(win_display->window, &ps);
               DeleteObject(hrgn);
            }
            return 0;
         }
         break;
      }

      case WM_SETCURSOR:
         switch (LOWORD(lParam)) {
            case HTLEFT:
            case HTRIGHT:
               SetCursor(LoadCursor(NULL, IDC_SIZEWE));
               break;
            case HTBOTTOM:
            case HTTOP:
               SetCursor(LoadCursor(NULL, IDC_SIZENS));
               break;
            case HTBOTTOMLEFT:
            case HTTOPRIGHT:
               SetCursor(LoadCursor(NULL, IDC_SIZENESW));
               break;
            case HTBOTTOMRIGHT:
            case HTTOPLEFT:
               SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
               break;
            default:
               if (win_display->mouse_cursor_shown) {
                  SetCursor(win_display->mouse_selected_hcursor);
               }
               else {
                  SetCursor(NULL);
               }
               break;
         }
         return 1;
      case WM_ACTIVATE:
         if (LOWORD(wParam) != WA_INACTIVE) {
            _al_win_grab_input(win_display);
            if (d->vt->switch_in)
            	  d->vt->switch_in(d);
            //SetCapture(hWnd);
            _al_event_source_lock(es);
            if (_al_event_source_needs_to_generate_event(es)) {
               ALLEGRO_EVENT event;
               event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
               event.display.timestamp = al_current_time();
               _al_event_source_emit_event(es, &event);
            }
            _al_event_source_unlock(es);
            return 0;
         }
         else {
            unacquire_input(win_display);
            if (d->flags & ALLEGRO_FULLSCREEN) {
               d->vt->switch_out(d);
            }
            //ReleaseCapture();
            _al_event_source_lock(es);
            if (_al_event_source_needs_to_generate_event(es)) {
               ALLEGRO_EVENT event;
               event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
               event.display.timestamp = al_current_time();
               _al_event_source_emit_event(es, &event);
            }
            _al_event_source_unlock(es);
            return 0;
         }
         break;
      case WM_INITMENUPOPUP:
         win_display->is_in_sysmenu = true;
         _al_win_mouse_set_sysmenu(true);
         return 0;
      case WM_MENUSELECT:
         if ((HIWORD(wParam) == 0xFFFF) && (!lParam)) {
            win_display->is_in_sysmenu = false;
            _al_win_mouse_set_sysmenu(false);
         }
         return 0;
      case WM_MENUCHAR :
         return (MNC_CLOSE << 16) | (wParam & 0xffff);
      case WM_CLOSE:
         _al_event_source_lock(es);
         if (_al_event_source_needs_to_generate_event(es)) {
            ALLEGRO_EVENT event;
            event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
            event.display.timestamp = al_current_time();
            _al_event_source_emit_event(es, &event);
         }
         _al_event_source_unlock(es);
         return 0;
      case WM_SIZE:
         if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED) {
            /*
             * Delay the resize event so we don't get bogged down with them
             */
            if (!resize_postponed) {
               resize_postponed = true;
               postpone_resize(win_display->window);
            }
         }
         return 0;
      case WM_USER+0:
         /* Generate a resize event if the size has changed. We cannot asynchronously
          * change the display size here yet, since the user will only know about a
          * changed size after receiving the resize event. Here we merely add the
          * event to the queue.
          */
         GetWindowInfo(win_display->window, &wi);
         x = wi.rcClient.left;
         y = wi.rcClient.top;
         w = wi.rcClient.right - wi.rcClient.left;
         h = wi.rcClient.bottom - wi.rcClient.top;
         if (d->w != w || d->h != h) {
            _al_event_source_lock(es);
            if (_al_event_source_needs_to_generate_event(es)) {
               ALLEGRO_EVENT event;
               event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
               event.display.timestamp = al_current_time();
               event.display.x = x;
               event.display.y = y;
               event.display.width = w;
               event.display.height = h;
               _al_event_source_emit_event(es, &event);
            }

            /* Generate an expose event. */
            if (_al_event_source_needs_to_generate_event(es)) {
               ALLEGRO_EVENT event;
               event.display.type = ALLEGRO_EVENT_DISPLAY_EXPOSE;
               event.display.timestamp = al_current_time();
               event.display.x = x;
               event.display.y = y;
               event.display.width = w;
               event.display.height = h;
               _al_event_source_emit_event(es, &event);
            }
            _al_event_source_unlock(es);
            resize_postponed = false;
         }
         return 0;
   } 

   return DefWindowProc(hWnd,message,wParam,lParam); 
}

int _al_win_init_window()
{
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

   RegisterClass(&window_class);

   if (_al_win_msg_call_proc == 0 && _al_win_msg_suicide == 0) {
      _al_win_msg_call_proc = RegisterWindowMessage("Allegro call proc");
      _al_win_msg_suicide = RegisterWindowMessage("Allegro window suicide");
   }

   return true;
}


void _al_win_set_display_icon(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_BITMAP *scaled_bmp;
   HICON icon, old_small, old_big;
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;
   ALLEGRO_STATE backup;

   al_store_state(&backup, ALLEGRO_STATE_BITMAP | ALLEGRO_STATE_BLENDER);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   scaled_bmp = al_create_bitmap(32, 32);
   al_set_target_bitmap(scaled_bmp);
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));
   al_draw_scaled_bitmap(bmp, 0, 0,
      al_get_bitmap_width(bmp),
      al_get_bitmap_height(bmp),
      0, 0, 32, 32, 0);

   al_restore_state(&backup);

   icon = _al_win_create_icon(win_display->window, scaled_bmp, 0, 0, false);

   old_small = (HICON)SendMessage(win_display->window, WM_SETICON,
      ICON_SMALL, (LPARAM)icon);
   old_big = (HICON)SendMessage(win_display->window, WM_SETICON,
      ICON_BIG, (LPARAM)icon);

   if (old_small)
      DestroyIcon(old_small);
   if (old_big)
      DestroyIcon(old_big);

   al_destroy_bitmap(scaled_bmp);
}

void _al_win_set_window_position(HWND window, int x, int y)
{
   SetWindowPos(
      window,
      HWND_TOP,
      x,
      y,
      0,
      0,
      SWP_NOSIZE | SWP_NOZORDER);
}

void _al_win_get_window_position(HWND window, int *x, int *y)
{
   RECT r;

   get_window_pos(window, &r);

   if (x) {
      *x = r.left;
   }
   if (y) {
      *y = r.top;
   }
}


void _al_win_toggle_window_frame(ALLEGRO_DISPLAY *display, HWND hWnd,
   int w, int h, bool onoff)
{
   if (onoff) {
      display->flags &= ~ALLEGRO_NOFRAME;
   }
   else {
      display->flags |= ALLEGRO_NOFRAME;
   }

   if (display->flags & ALLEGRO_NOFRAME) {
      SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE);
      SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
      SetWindowPos(hWnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
   }
   else {
      RECT r;
      DWORD style;
      DWORD exStyle;

      if (display->flags & ALLEGRO_RESIZABLE) {
         style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
         exStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;
      }
      else {
         style = WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
         exStyle = WS_EX_APPWINDOW;
      }

      GetWindowRect(hWnd, &r);
      AdjustWindowRectEx(&r, style, false, exStyle);

      w = r.right - r.left;
      h = r.bottom - r.top;

      SetWindowLong(hWnd, GWL_STYLE, style);
      SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);
      SetWindowPos(hWnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
   }
}

void _al_win_set_window_title(ALLEGRO_DISPLAY *display, AL_CONST char *title)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;
   SetWindowText(win_display->window, title);
}


/* _al_win_wnd_call_proc:
 *  instructs the specifed window thread to call the specified procedure. Waits
 *  until the procedure has returned.
 */
void _al_win_wnd_call_proc(HWND wnd, void (*proc) (void*), void* param)
{
   ASSERT(_al_win_msg_call_proc);
   SendMessage(wnd, _al_win_msg_call_proc, (WPARAM)proc, (LPARAM)param);
}


/* _al_win_wnd_schedule_proc:
 *  instructs the specifed window thread to call the specified procedure but
 *  doesn't wait until the procedure has returned.
 */
void _al_win_wnd_schedule_proc(HWND wnd, void (*proc) (void*), void* param)
{
   ASSERT(_al_win_msg_call_proc);
   if (!PostMessage(wnd, _al_win_msg_call_proc, (WPARAM)proc, (LPARAM)param)) {
      TRACE(PREFIX_E "_al_win_wnd_schedule_proc failed.\n");
   }
}

/* vi: set ts=8 sts=3 sw=3 et: */
