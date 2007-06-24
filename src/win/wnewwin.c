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

#include <allegro.h>
#include <winalleg.h>
#include <process.h>

#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_vector.h"
#include "allegro/platform/aintwin.h"

#include "win_new.h"

static ATOM window_identifier;
static WNDCLASS window_class;
static _AL_VECTOR thread_handles = _AL_VECTOR_INITIALIZER(DWORD);

/* We have to keep track of windows some how */
typedef struct WIN_WINDOW {
   AL_DISPLAY *display;
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

	GetWindowRect(window, &with_decorations);
	memcpy(&adjusted, &with_decorations, sizeof(RECT));
	AdjustWindowRect(&adjusted, GetWindowLong(window, GWL_STYLE), FALSE);

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


HWND _al_win_create_window(AL_DISPLAY *display, int width, int height, int flags)
{
	HANDLE hThread;
	DWORD result;
	MSG msg;
	unsigned int i;
	HWND my_window;
	RECT working_area;
	RECT win_size;
	RECT pos;
	DWORD style;
	DWORD ex_style;
	WIN_WINDOW *win_window;
	WIN_WINDOW **add;

	/* Save the thread handle for later use */
	*((DWORD *)_al_vector_alloc_back(&thread_handles)) = GetCurrentThreadId();

	SystemParametersInfo(SPI_GETWORKAREA, 0, &working_area, 0);

	win_size.left = (working_area.left + working_area.right - width) / 2;
	win_size.top = (working_area.top + working_area.bottom - height) / 2;
	win_size.right = win_size.left + width;
	win_size.bottom = win_size.top + height;

	if (flags & AL_WINDOWED) {
		if  (flags & AL_RESIZABLE) {
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

	AdjustWindowRect(&win_size, GetWindowLong(my_window, GWL_STYLE), FALSE);

	MoveWindow(my_window, win_size.left, win_size.top,
		win_size.right - win_size.left,
		win_size.bottom - win_size.top,
		TRUE);

	win_get_window_pos(my_window, &pos);
	wnd_x = pos.left;
	wnd_y = pos.top;

	if (!(flags & AL_RESIZABLE)) {
		/* Make the window non-resizable */
		HMENU menu;
		menu = GetSystemMenu(my_window, FALSE);
		DeleteMenu(menu, SC_SIZE, MF_BYCOMMAND);
		DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
		DrawMenuBar(my_window);
	}

	if (!(flags & AL_WINDOWED)) {
	   wnd_x = 0;
	   wnd_y = 0;
	}

	_al_win_wnd = my_window;
	return my_window;
}

/* This must be called by Windows drivers after their window is deleted */
void _al_win_delete_thread_handle(DWORD handle)
{
	_al_win_delete_from_vector(&thread_handles, (void *)handle);
}

static LRESULT CALLBACK window_callback(HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam)
{
	AL_DISPLAY *d = NULL;
	WINDOWINFO wi;
	int w;
	int h;
	int x;
	int y;
	unsigned int i, j;
	int fActive;
	AL_EVENT_SOURCE *es = NULL;
	RECT pos;
	AL_SYSTEM *system = al_system_driver();
	WIN_WINDOW *win;

	if (message == _al_win_msg_call_proc) {
		return ((int (*)(void))wParam) ();
	}

	for (i = 0; i < system->displays._size; i++) {
		AL_DISPLAY **dptr = _al_vector_ref(&system->displays, i);
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
		_AL_FREE(win);
		_al_win_delete_from_vector(&win_window_list, win);
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
				mouse_set_syscursor();
				return 1;
			break;
			case WM_ACTIVATEAPP:
				if (wParam) {
					al_set_current_display((AL_DISPLAY *)d);
					_al_win_wnd = win->window;
					win_grab_input();
					win_get_window_pos(win->window, &pos);
					wnd_x = pos.left;
					wnd_y = pos.top;
					return 0;
				}
				else {
					if (_al_vector_find(&thread_handles, &lParam) < 0) {
						/*
						 * Device can be lost here, so
						 * backup textures.
						 */
						if (!(d->flags & AL_WINDOWED)) {
							d->vt->switch_out();
						}
						_al_win_ungrab_input();
						return 0;
					}
				}
				break;
			case WM_CLOSE:
				_al_event_source_lock(es);
				if (_al_event_source_needs_to_generate_event(es)) {
					AL_EVENT *event = _al_event_source_get_unused_event(es);
					if (event) {
						event->display.type = AL_EVENT_DISPLAY_CLOSE;
						event->display.timestamp = al_current_time();
						_al_event_source_emit_event(es, event);
					}
				}
				_al_event_source_unlock(es);
				return 0;
			case WM_SIZE:
				if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED) {
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
							AL_EVENT *event = _al_event_source_get_unused_event(es);
							if (event) {
								event->display.type = AL_EVENT_DISPLAY_RESIZE;
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
							AL_EVENT *event = _al_event_source_get_unused_event(es);
							if (event) {
								event->display.type = AL_EVENT_DISPLAY_EXPOSE;
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

