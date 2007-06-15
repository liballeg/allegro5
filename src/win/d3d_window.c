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

static void d3d_destroy_window(AL_DISPLAY_D3D *display)
{
	_al_vector_find_and_delete(&thread_handles, &display->thread_handle);
}

/* wnd_thread_proc:
 *  Thread function that handles the messages of the directx window.
 */
static void d3d_wnd_thread_proc(HANDLE unused)
{
	DWORD result;
	MSG msg;
	unsigned int i;
	HWND my_window;
	RECT working_area;
	RECT win_size;
	RECT pos;

	/* Save the thread handle for later use */
	*((DWORD *)_al_vector_alloc_back(&thread_handles)) = GetCurrentThreadId();
	_al_d3d_last_created_display->thread_handle = GetCurrentThreadId();

	SystemParametersInfo(SPI_GETWORKAREA, 0, &working_area, 0);

	win_size.left = (working_area.left + working_area.right - new_window_width) / 2;
	win_size.top = (working_area.top + working_area.bottom - new_window_height) / 2;
	win_size.right = win_size.left + new_window_width;
	win_size.bottom = win_size.top + new_window_height;

	// Create Window 
	my_window = CreateWindowEx(0, 
		"ALEX", "D3D Window", 
		WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		100, 100, 100, 100, //new_window_width, new_window_height,
		NULL,NULL,window_class.hInstance,0);

	AdjustWindowRect(&win_size, GetWindowLong(my_window, GWL_STYLE), FALSE);

	MoveWindow(my_window, win_size.left, win_size.top,
		win_size.right - win_size.left,
		win_size.bottom - win_size.top,
		TRUE);

	d3d_get_window_pos(my_window, &pos);
	wnd_x = pos.left;
	wnd_y = pos.top;

	new_window = my_window;

	//_win_thread_init();

	for (;;) {
		result = MsgWaitForMultipleObjects(_win_input_events, _win_input_event_id, FALSE, INFINITE, QS_ALLINPUT);
		if (result < (DWORD) WAIT_OBJECT_0 + _win_input_events) {
			/* one of the registered events is in signaled state */
			(*_win_input_event_handler[result - WAIT_OBJECT_0])();
		}
		else if (result == (DWORD) WAIT_OBJECT_0 + _win_input_events) {
			/* messages are waiting in the queue */
			while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
				if (GetMessage(&msg, NULL, 0, 0)) {
					DispatchMessage(&msg);
				}
				else {
					goto End;
				}
			}
		}
	}

End:
   _TRACE("window thread exits\n");

   //_win_thread_exit();
}

HWND _al_d3d_create_hidden_window()
{
	return CreateWindowEx(0, 
		"ALEX", "D3D Window",
		0,
		100, 100, 100, 100,
		NULL,NULL,window_class.hInstance,0);
}

HWND _al_d3d_create_window(int width, int height)
{
	HANDLE hThread;

	new_window = (HWND)-1;
	new_window_width = width;
	new_window_height = height;

	hThread = (HANDLE) _beginthread(d3d_wnd_thread_proc, 0, 0);

	while (new_window == (HWND)-1)
		al_rest(1);
	/*
	if (_al_d3d_keyboard_initialized) {
		AL_DISPLAY_D3D *d = (AL_DISPLAY_D3D *)_al_current_display;
		//key_dinput_set_cooperation_level(new_window);
		d->keyboard_initialized = true;
	}
	*/

	_al_win_wnd = new_window;
	return new_window;
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

	if (message == _al_win_msg_suicide) {
		SendMessage(_al_win_compat_wnd, _al_win_msg_suicide, 0, 0);
		DestroyWindow(hWnd);
		return 0;
	}


	for (i = 0; i < _al_d3d_system->system.displays._size; i++) {
		AL_DISPLAY_D3D **dptr = _al_vector_ref(&_al_d3d_system->system.displays, i);
		d = *dptr;
		if (d->window == hWnd) {
			es = &d->display.es;
			break;
		}
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
				if (wParam == TRUE) {
					al_change_current_display((AL_DISPLAY *)d);
					_al_win_wnd = d->window;
					win_grab_input();
					d3d_get_window_pos(d->window, &pos);
					wnd_x = pos.left;
					wnd_y = pos.top;
				}
				else {
					if (_al_vector_find(&thread_handles, &lParam) < 0) {
						_al_d3d_win_ungrab_input();
					}
				}
				return 0;
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


