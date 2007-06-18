#include <allegro.h>
#include <winalleg.h>
#include <process.h>

#include "allegro/internal/aintern_vector.h"
#include "allegro/platform/aintwin.h"

#include "d3d.h"

//#define ALLEGRO_WND_CLASS "ALLEG"

static HWND new_window;
static int new_window_width;
static int new_window_height;
static bool new_window_resizable;
static bool new_window_fullscreen;
static ATOM window_identifier;
static WNDCLASS window_class;
// FIXME: must be deleted with display
static _AL_VECTOR thread_handles = _AL_VECTOR_INITIALIZER(DWORD);

/*
 * Find the top left position of the client area of a window.
 */
static void d3d_get_window_pos(HWND window, RECT *pos)
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

HWND _al_d3d_create_hidden_window()
{
	HWND window = CreateWindowEx(0, 
		"ALEX", "D3D Window",0,
		-100, -100, 0, 0,
		NULL,NULL,window_class.hInstance,0);
	SetWindowPos(window, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
	return window;
}

HWND _al_d3d_create_window(int width, int height, int flags)
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

	new_window = (HWND)-1;
	new_window_width = width;
	new_window_height = height;
	new_window_resizable = flags & AL_RESIZABLE;
	new_window_fullscreen = !(flags & AL_WINDOWED);

	/* Save the thread handle for later use */
	*((DWORD *)_al_vector_alloc_back(&thread_handles)) = GetCurrentThreadId();
	_al_d3d_last_created_display->thread_handle = GetCurrentThreadId();

	SystemParametersInfo(SPI_GETWORKAREA, 0, &working_area, 0);

	win_size.left = (working_area.left + working_area.right - new_window_width) / 2;
	win_size.top = (working_area.top + working_area.bottom - new_window_height) / 2;
	win_size.right = win_size.left + new_window_width;
	win_size.bottom = win_size.top + new_window_height;

	if (new_window_fullscreen) {
		style = WS_POPUP|WS_VISIBLE;
		ex_style = 0;
	}
	else {
		if  (new_window_resizable) {
			style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
			ex_style = WS_EX_APPWINDOW|WS_EX_OVERLAPPEDWINDOW;
		}
		else {
			style = WS_VISIBLE|WS_SYSMENU;
			ex_style = WS_EX_APPWINDOW;
		}
	}

	my_window = CreateWindowEx(ex_style,
		"ALEX", "D3D Window", style,
		0, 0, width, height,
		NULL,NULL,window_class.hInstance,0);

	AdjustWindowRect(&win_size, GetWindowLong(my_window, GWL_STYLE), FALSE);

	MoveWindow(my_window, win_size.left, win_size.top,
		win_size.right - win_size.left,
		win_size.bottom - win_size.top,
		TRUE);

	d3d_get_window_pos(my_window, &pos);
	wnd_x = pos.left;
	wnd_y = pos.top;

	if (!new_window_resizable) {
		/* Make the window non-resizable */
		HMENU menu;
		menu = GetSystemMenu(my_window, FALSE);
		DeleteMenu(menu, SC_SIZE, MF_BYCOMMAND);
		DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
		DrawMenuBar(my_window);
	}

	new_window = my_window;

	/*
	if (_al_d3d_keyboard_initialized) {
		AL_DISPLAY_D3D *d = (AL_DISPLAY_D3D *)_al_current_display;
		key_dinput_set_cooperative_level(new_window);
		d->keyboard_initialized = true;
	}
	*/

	_al_win_wnd = new_window;
	return new_window;
}

static void d3d_delete_thread_handle(AL_DISPLAY_D3D *display)
{
	_al_d3d_delete_from_vector(&thread_handles, &display->thread_handle);
}

/* Quit the program on close button press for now */
static LRESULT CALLBACK window_callback(HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam)
{
	AL_DISPLAY_D3D *d = NULL;
	WINDOWINFO wi;
	int w;
	int h;
	int x;
	int y;
	unsigned int i;
	int fActive;
	AL_EVENT_SOURCE *es = NULL;
	RECT pos;

	if (message == _al_win_msg_call_proc) {
		return ((int (*)(void))wParam) ();
	}

	for (i = 0; i < _al_d3d_system->system.displays._size; i++) {
		AL_DISPLAY_D3D **dptr = _al_vector_ref(&_al_d3d_system->system.displays, i);
		d = *dptr;
		if (d->window == hWnd) {
			es = &d->display.es;
			break;
		}
	}

	if (message == _al_win_msg_suicide) {
		d3d_delete_thread_handle(d);
		//SendMessage(_al_win_compat_wnd, _al_win_msg_suicide, 0, 0);
		DestroyWindow(hWnd);
		return 0;
	}


	if (i != _al_d3d_system->system.displays._size) {
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
				return 1;  /* not TRUE */
			break;
			case WM_ACTIVATEAPP:
				if (wParam) {
					al_set_current_display((AL_DISPLAY *)d);
					_al_win_wnd = d->window;
					win_grab_input();
					d3d_get_window_pos(d->window, &pos);
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
						if (!(d->display.flags & AL_WINDOWED)) {
							_al_d3d_prepare_bitmaps_for_reset();
						}
						_al_d3d_win_ungrab_input();
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
					GetWindowInfo(d->window, &wi);
					x = wi.rcClient.left;
					y = wi.rcClient.top;
					if (d->display.w != w || d->display.h != h) {
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

/* wnd_call_proc:
 *  Instructs the window thread to call the specified procedure and
 *  waits until the procedure has returned.
 */
int d3d_wnd_call_proc(int (*proc) (void))
{
   return SendMessage(_al_win_wnd, _al_win_msg_call_proc, (DWORD) proc, 0);
}

/* d3d_wnd_schedule_proc:
 *  instructs the window thread to call the specified procedure but
 *  doesn't wait until the procedure has returned.
 */
void d3d_wnd_schedule_proc(int (*proc) (void))
{
   PostMessage(_al_win_wnd, _al_win_msg_call_proc, (DWORD) proc, 0);
}

HWND d3d_win_get_window()
{
	return _al_win_wnd;
}

int _al_d3d_init_window()
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

void _al_d3d_win_ungrab_input()
{
   wnd_schedule_proc(key_dinput_unacquire);
}


#if 0
static void d3d_window_exit()
{
      /* Destroy the window: we cannot directly use DestroyWindow() because
       * we are not running in the same thread as that of the window.
       */
      // for each window
      PostMessage(allegro_wnd, _al_win_msg_suicide, 0, 0);

      /* wait until the window thread ends */
      WaitForSingleObject(d3d_wnd_thread, INFINITE);
      d3d_wnd_thread = NULL;
}
#endif


