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
 *      Windows mouse driver.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTINPUT_VERSION 0x0300

#include "wddraw.h"

#ifndef SCAN_DEPEND
   #include <process.h>
   #include <dinput.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


_DRIVER_INFO _mouse_driver_list[] =
{
   {MOUSE_DIRECTX, &mouse_directx, TRUE},
   {MOUSEDRV_NONE, &mousedrv_none, TRUE},
   {0, NULL, 0}
};


static int mouse_directx_init(void);
static void mouse_directx_exit(void);
static void mouse_directx_position(int x, int y);
static void mouse_directx_set_range(int x1, int y1, int x2, int y2);
static void mouse_directx_set_speed(int xspeed, int yspeed);
static void mouse_directx_get_mickeys(int *mickeyx, int *mickeyy);
static void mouse_directx_poll(void);

MOUSE_DRIVER mouse_directx =
{
   MOUSE_DIRECTX,
   empty_string,
   empty_string,
   "DirectInput",
   mouse_directx_init,
   mouse_directx_exit,
   NULL,                        /* mouse_directx_poll, */
   NULL,
   mouse_directx_position,
   mouse_directx_set_range,
   mouse_directx_set_speed,
   mouse_directx_get_mickeys,
   NULL
};


#define DINPUT_BUFFERSIZE 256
static HANDLE mouse_thread = NULL;
static HANDLE mouse_thread_stop_event = NULL;
static HANDLE mouse_input_event = NULL;
static LPDIRECTINPUT mouse_dinput = NULL;
static LPDIRECTINPUTDEVICE mouse_dinput_device = NULL;

static int dinput_buttons = 0;
static int dinput_wheel = FALSE;

static int dinput_x = 0;        /* tracked dinput positon */
static int dinput_y = 0;

static int mouse_mx = 0;        /* internal position, in mickeys */
static int mouse_my = 0;

static int mouse_sx = 2;        /* mickey -> pixel scaling factor */
static int mouse_sy = 2;

static int mouse_minx = 0;      /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;      /* for get_mouse_mickeys() */
static int mymickey_y = 0;
static int mymickey_ox = 0;
static int mymickey_oy = 0;

static int mouse_wason_valid = FALSE;
static int mouse_wason = FALSE;

#define MICKEY_TO_COORD_X(n)        ((n) / mouse_sx)
#define MICKEY_TO_COORD_Y(n)        ((n) / mouse_sy)

#define COORD_TO_MICKEY_X(n)        ((n) * mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * mouse_sy)

#define CLEAR_MICKEYS()             \
{                                   \
   dinput_x = 0;                    \
   dinput_y = 0;                    \
   mymickey_ox = 0;                 \
   mymickey_oy = 0;                 \
}



/* mouse_dinput_acquire:
 *  acquires mouse device.
 */
int mouse_dinput_acquire(void)
{
   HRESULT hr;

   _mouse_b = 0;

   if (mouse_dinput_device) {
      hr = IDirectInputDevice_Acquire(mouse_dinput_device);

      if (FAILED(hr)) {
	 _TRACE("acquire mouse = %s (%x)\n", win_err_str(hr), hr);
	 return -1;
      }

      if (mouse_wason_valid) {
	 _mouse_on = mouse_wason;
	 mouse_wason_valid = FALSE;
      }

      mouse_set_cursor();

      SetEvent(mouse_input_event);

      return 0;
   }
   else
      return -1;
}



/* mouse_dinput_unacquire:
 *  unacquires mouse device.
 */
void mouse_dinput_unacquire(void)
{
   if (mouse_dinput_device) {
      IDirectInputDevice_Unacquire(mouse_dinput_device);

      mouse_wason_valid = TRUE;
      mouse_wason = _mouse_on;
      _mouse_on = FALSE;
   }
}



/* mouse_set_cursor:
 *  Selects whatever cursor we should display.
 */
int mouse_set_cursor(void)
{
   if (((mouse_dinput_device) && (_mouse_on)) || (!wnd_windowed))
      SetCursor(NULL);
   else
      SetCursor(LoadCursor(NULL, IDC_ARROW));

   return TRUE;
}



/* mouse_sysmenu_changed:
 *  Alters the windowed mouse state when going to/from sysmenu mode.
 */
void mouse_sysmenu_changed()
{
   if (wnd_sysmenu) {
      _mouse_on = FALSE;
   }
   else {
      POINT p;

      GetCursorPos(&p);

      p.x -= wnd_x;
      p.y -= wnd_y;

      if ((p.x < mouse_minx) || (p.x > mouse_maxx) ||
	  (p.y < mouse_miny) || (p.y > mouse_maxy)) {
	 _mouse_on = FALSE;
      }
      else {
	 _mouse_x = p.x;
	 _mouse_y = p.y;
	 _mouse_on = TRUE;
      }
   } 

   mouse_set_cursor();
   _handle_mouse_input();
}



/* mouse_dinput_exit:
 *  releases DirectInput mouse device
 */
static void mouse_dinput_exit(void)
{
   _TRACE("mouse_dinput_exit\n");

   /* release mouse device */
   if (mouse_dinput_device) {
      /* unacquire device first */
      mouse_dinput_unacquire();

      /* now it can be released */
      IDirectInputDevice_Release(mouse_dinput_device);
      mouse_dinput_device = NULL;
   }

   /* release DirectInput interface */
   if (mouse_dinput) {
      IDirectInput_Release(mouse_dinput);
      mouse_dinput = NULL;
   }

   /* close event handle */
   if (mouse_input_event) {
      CloseHandle(mouse_input_event);
      mouse_input_event = NULL;
   }
}



/* mouse_enum_ccallback:
 *  For finding out how many buttons we have.
 */
BOOL CALLBACK mouse_enum_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
   if (memcmp(&lpddoi->guidType, &GUID_ZAxis, sizeof(GUID)) == 0)
      dinput_wheel = TRUE;
   else if (memcmp(&lpddoi->guidType, &GUID_Button, sizeof(GUID)) == 0)
      dinput_buttons++;

   return DIENUM_CONTINUE;
}



/* mouse_dinput_init:
 *  setup DirectInput mouse device
 */
static int mouse_dinput_init(void)
{
   HRESULT hr;
   DIPROPDWORD property_buf_size =
   {
   // the header
      {
	 sizeof(DIPROPDWORD),   // diph.dwSize
	 sizeof(DIPROPHEADER), // diph.dwHeaderSize
	 0,                    // diph.dwObj
	 DIPH_DEVICE,          // diph.dwHow
      },

   // the data
      DINPUT_BUFFERSIZE,        // dwData
   };

   _TRACE("mouse_dinput_init\n");

   /* Get DirectInput interface */
   hr = DirectInputCreate(allegro_inst, DIRECTINPUT_VERSION, &mouse_dinput, NULL);
   if (FAILED(hr))
      goto Error;

   /* Create the mouse device */
   hr = IDirectInput_CreateDevice(mouse_dinput, &GUID_SysMouse, &mouse_dinput_device, NULL);
   if (FAILED(hr))
      goto Error;

   /* Find how many buttons */
   dinput_buttons = 0;
   dinput_wheel = FALSE;

   hr = IDirectInputDevice_EnumObjects(mouse_dinput_device, mouse_enum_callback, NULL, DIDFT_PSHBUTTON | DIDFT_AXIS);

   /* Set data format */
   hr = IDirectInputDevice_SetDataFormat(mouse_dinput_device, &c_dfDIMouse);
   if (FAILED(hr))
      goto Error;

   /* Set buffer size */
   hr = IDirectInputDevice_SetProperty(mouse_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr))
      goto Error;

   /* Set cooperative level */
   hr = IDirectInputDevice_SetCooperativeLevel(mouse_dinput_device, allegro_wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
   if (FAILED(hr))
      goto Error;

   /* Enable event notification */
   mouse_input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   hr = IDirectInputDevice_SetEventNotification(mouse_dinput_device, mouse_input_event);
   if (FAILED(hr))
      goto Error;

   /* Acquire the created device */
   wnd_acquire_mouse();

   return 0;

 Error:
   mouse_dinput_exit();
   return -1;
}



/* mouse_directx_poll:
 *  handles queued mouse input
 */
static void mouse_directx_poll(void)
{
   int waiting_messages;
   HRESULT hr;
   int current;
   static DIDEVICEOBJECTDATA message_buffer[DINPUT_BUFFERSIZE];
   int ofs, data;

   if (!app_foreground)
   {
      thread_switch_out();
      return;
   }

   while (1) {
      /* the whole buffer is free */
      waiting_messages = DINPUT_BUFFERSIZE;

      /* fill the buffer */
      hr = IDirectInputDevice_GetDeviceData(mouse_dinput_device,
					    sizeof(DIDEVICEOBJECTDATA),
					    message_buffer,
					    &waiting_messages,
					    0);

      /* was device lost */
      if (hr == DIERR_INPUTLOST) {
	 /* reaqcuire device, if this fails, stop polling */
	 _TRACE("mouse device was lost\n");
	 wnd_acquire_mouse();
	 break;
      }
      else if (FAILED(hr)) {
	 /* any other error will also stop polling */
	 _TRACE("unexpected error while filling mouse message buffer\n"); 
	 break;
      }
      else {
	 /* normally only this case should happen */
	 for (current = 0; current < waiting_messages; current++) {
	    ofs = message_buffer[current].dwOfs;
	    data = message_buffer[current].dwData;

	    switch (ofs) {

	       case DIMOFS_X:
		  if (!wnd_windowed)
		     dinput_x += data;
		  break;

	       case DIMOFS_Y:
		  if (!wnd_windowed)
		     dinput_y += data;
		  break;

	       case DIMOFS_Z:
		  if ((dinput_wheel) && (_mouse_on))
		     _mouse_z += data/120;
		  break;

	       case DIMOFS_BUTTON0:
		  if (data & 0x80) {
		     if (_mouse_on)
			_mouse_b |= 1;
		  }
		  else
		     _mouse_b &= ~1;
		  break;

	       case DIMOFS_BUTTON1:
		  if (data & 0x80) {
		     if (_mouse_on)
			_mouse_b |= 2;
		  }
		  else
		     _mouse_b &= ~2;
		  break;

	       case DIMOFS_BUTTON2:
		  if (data & 0x80) {
		     if (_mouse_on)
			_mouse_b |= 4;
		  }
		  else
		     _mouse_b &= ~4;
		  break;
	    }
	 }

	 if (wnd_windowed) {
	    /* windowed input mode */
	    if (!wnd_sysmenu) {
	       POINT p;

	       GetCursorPos(&p);

	       p.x -= wnd_x;
	       p.y -= wnd_y;

	       mymickey_x += p.x - _mouse_x;
	       mymickey_y += p.y - _mouse_y;

	       if ((p.x < mouse_minx) || (p.x > mouse_maxx) ||
		   (p.y < mouse_miny) || (p.y > mouse_maxy)) {
		  if (_mouse_on) {
		     _mouse_on = FALSE;
		     wnd_set_cursor();
		  }
	       }
	       else {
		  _mouse_x = p.x;
		  _mouse_y = p.y;

		  if (!_mouse_on) {
		     _mouse_on = TRUE;
		     wnd_set_cursor();
		  }
	       }

	       _handle_mouse_input();
	    }
	 }
	 else {
	    /* normal fullscreen input mode */
	    mymickey_x += dinput_x - mymickey_ox;
	    mymickey_y += dinput_y - mymickey_oy;

	    mymickey_ox = dinput_x;
	    mymickey_oy = dinput_y;

	    _mouse_x = MICKEY_TO_COORD_X(mouse_mx + dinput_x);
	    _mouse_y = MICKEY_TO_COORD_Y(mouse_my + dinput_y);

	    if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx) ||
		(_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {

	       _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
	       _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

	       mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
	       mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

	       CLEAR_MICKEYS();
	    }

	    _mouse_on = TRUE;
	    _handle_mouse_input();
	 }

	 break;
      }
   }
}



/* mouse_directx_thread:
 */
static void mouse_directx_thread(HANDLE setup_event)
{
   HANDLE events[2];
   DWORD result;

   /* setup DirectInput */
   if (mouse_dinput_init())
      return;

   /* now the thread it running successfully, let's acknowledge */
   SetEvent(setup_event);

   /* setup event array */
   events[0] = mouse_thread_stop_event;
   events[1] = mouse_input_event;

   /* input loop */
   while (1) {
      result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

      switch (result) {

	 case WAIT_OBJECT_0 + 0:        /* thread should stop */
	    _TRACE("mouse thread exits\n");
	    mouse_dinput_exit();
	    return;

	 case WAIT_OBJECT_0 + 1:        /* mouse input is queued */ 
	    mouse_directx_poll(); 
	    break;
      }
   }
}



/* mouse_directx_init:
 */
static int mouse_directx_init(void)
{
   HANDLE events[2];
   DWORD result;

   /* Mouse input is handled by a additional thread */
   mouse_thread_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
   events[1] = (HANDLE) _beginthread(mouse_directx_thread, 0, events[0]);

   /* Wait until thread quits or acknowledge that it is up */
   result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
   CloseHandle(events[0]);

   if (result == WAIT_OBJECT_0) {       /* the thread has started successfully */
      mouse_thread = events[1];
      SetThreadPriority(mouse_thread, THREAD_PRIORITY_ABOVE_NORMAL);
      return dinput_buttons;
   }
   else {                       /* something has gone wrong */
      return -1;
   }
}



/* mouse_directx_exit:
 */
static void mouse_directx_exit(void)
{
   if (mouse_thread) {
      /* set stop event to stop the input thread */
      SetEvent(mouse_thread_stop_event);

      /* wait for thread shutdown */
      WaitForSingleObject(mouse_thread, INFINITE);

      /* now we can free all resource */
      CloseHandle(mouse_thread_stop_event);
      mouse_thread = NULL;
   }
}



/* mouse_directx_position:
 */
static void mouse_directx_position(int x, int y)
{
   _enter_critical();

   _mouse_x = x;
   _mouse_y = y;

   mouse_mx = COORD_TO_MICKEY_X(x);
   mouse_my = COORD_TO_MICKEY_Y(y);

   CLEAR_MICKEYS();

   mymickey_x = mymickey_y = 0;

   if (wnd_windowed)
      SetCursorPos(x+wnd_x, y+wnd_y);

   _exit_critical();
}



/* mouse_directx_set_range:
 */
static void mouse_directx_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   _enter_critical();

   _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   CLEAR_MICKEYS();

   _exit_critical();
}



/* mouse_directx_set_speed:
 */
static void mouse_directx_set_speed(int xspeed, int yspeed)
{
   _enter_critical();

   mouse_sx = MAX(1, xspeed);
   mouse_sy = MAX(1, yspeed);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   CLEAR_MICKEYS();

   _exit_critical();
}



/* mouse_directx_get_mickeys:
 */
static void mouse_directx_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}

