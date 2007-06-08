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

#define ALLEGRO_NO_COMPATIBILITY

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_mouse.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <process.h>
   #include <dinput.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wmouse INFO: "
#define PREFIX_W                "al-wmouse WARNING: "
#define PREFIX_E                "al-wmouse ERROR: "



typedef struct AL_MOUSE_DIRECTX
{
   AL_MOUSE parent;
   AL_MSESTATE state;
   int min_x, min_y;
   int max_x, max_y;
} AL_MOUSE_DIRECTX;



static bool mouse_directx_installed = false;

/* the one and only mouse object */
static AL_MOUSE_DIRECTX the_mouse;



/* forward declarations */
static bool mouse_directx_init(void);
static void mouse_directx_exit(void);
static AL_MOUSE *mouse_directx_get_mouse(void);
static unsigned int mouse_directx_get_mouse_num_buttons(void);
static unsigned int mouse_directx_get_mouse_num_axes(void);
static bool mouse_directx_set_mouse_xy(int x, int y);
static bool mouse_directx_set_mouse_axis(int which, int z);
static bool mouse_directx_set_mouse_range(int x1, int y1, int x2, int y2);
static void mouse_directx_get_state(AL_MSESTATE *ret_state);

static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button);



/* the driver vtable */
#define MOUSEDRV_XWIN  AL_ID('X','W','I','N')

static AL_MOUSE_DRIVER mousedrv_directx =
{
   MOUSE_DIRECTX,
   empty_string,
   empty_string,
   "DirectInput mouse",
   mouse_directx_init,
   mouse_directx_exit,
   mouse_directx_get_mouse,
   mouse_directx_get_mouse_num_buttons,
   mouse_directx_get_mouse_num_axes,
   mouse_directx_set_mouse_xy,
   mouse_directx_set_mouse_axis,
   mouse_directx_set_mouse_range,
   mouse_directx_get_state
};



_DRIVER_INFO _al_mouse_driver_list[] =
{
   {MOUSE_DIRECTX, &mousedrv_directx, TRUE},
   {0, NULL, 0}
};



HCURSOR _win_hcursor = NULL;	/* Hardware cursor to display */


//XXX
static int _mouse_on;
//XXX end


#define DINPUT_BUFFERSIZE 256
static HANDLE mouse_input_event = NULL;
static LPDIRECTINPUT mouse_dinput = NULL;
static LPDIRECTINPUTDEVICE mouse_dinput_device = NULL;

static int dinput_buttons = 0;
static int dinput_wheel = FALSE;

static int mouse_swap_button = FALSE;     /* TRUE means buttons 1 and 2 are swapped */

static int mouse_sx = 2;              /* mickey -> pixel scaling factor */
static int mouse_sy = 2;

#define MAF_DEFAULT 1                 /* mouse acceleration parameters */
static int mouse_accel_fact = MAF_DEFAULT;
static int mouse_accel_mult = MAF_DEFAULT;
static int mouse_accel_thr1 = 5 * 5;
static int mouse_accel_thr2 = 16 * 16;

static int mouse_minx = 0;            /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;            /* for get_mouse_mickeys() */
static int mymickey_y = 0;
static int mymickey_ox = 0;
static int mymickey_oy = 0;

#define MICKEY_TO_COORD_X(n)        ((n) / mouse_sx)
#define MICKEY_TO_COORD_Y(n)        ((n) / mouse_sy)

#define COORD_TO_MICKEY_X(n)        ((n) * mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * mouse_sy)

#define CLEAR_MICKEYS()                       \
{                                             \
   if (gfx_driver && gfx_driver->windowed) {  \
      POINT p;                                \
                                              \
      GetCursorPos(&p);                       \
                                              \
      p.x -= wnd_x;                           \
      p.y -= wnd_y;                           \
                                              \
      mymickey_ox = p.x;                      \
      mymickey_oy = p.y;                      \
   }                                          \
   else {                                     \
      dinput_x = 0;                           \
      dinput_y = 0;                           \
      mymickey_ox = 0;                        \
      mymickey_oy = 0;                        \
   }                                          \
}

#define READ_CURSOR_POS(p, out_x, out_y)            \
{                                                   \
   GetCursorPos(&p);                                \
                                                    \
   p.x -= wnd_x;                                    \
   p.y -= wnd_y;                                    \
                                                    \
   if ((p.x < mouse_minx) || (p.x > mouse_maxx) ||  \
       (p.y < mouse_miny) || (p.y > mouse_maxy)) {  \
      if (_mouse_on) {                              \
         _mouse_on = FALSE;                         \
         wnd_schedule_proc(mouse_set_syscursor);    \
      }                                             \
   }                                                \
   else {                                           \
      if (!_mouse_on) {                             \
         _mouse_on = TRUE;                          \
         wnd_schedule_proc(mouse_set_syscursor);    \
      }                                             \
   }                                                \
                                                    \
   out_x = MID(mouse_minx, p.x, mouse_maxx);        \
   out_y = MID(mouse_miny, p.y, mouse_maxy);        \
}



/* dinput_err_str:
 *  Returns a DirectInput error string.
 */
#ifdef DEBUGMODE
static char* dinput_err_str(long err)
{
   static char err_str[64];

   switch (err) {

      case DIERR_ACQUIRED:
         _al_sane_strncpy(err_str, "the device is acquired", sizeof(err_str));
         break;

      case DIERR_NOTACQUIRED:
         _al_sane_strncpy(err_str, "the device is not acquired", sizeof(err_str));
         break;

      case DIERR_INPUTLOST:
         _al_sane_strncpy(err_str, "access to the device was not granted", sizeof(err_str));
         break;

      case DIERR_INVALIDPARAM:
         _al_sane_strncpy(err_str, "the device does not have a selected data format", sizeof(err_str));
         break;

#ifdef DIERR_OTHERAPPHASPRIO
      /* this is not a legacy DirectX 3 error code */
      case DIERR_OTHERAPPHASPRIO:
         _al_sane_strncpy(err_str, "can't acquire the device in background", sizeof(err_str));
         break;
#endif

      default:
         _al_sane_strncpy(err_str, "unknown error", sizeof(err_str));
   }

   return err_str;
}
#else
#define dinput_err_str(hr) "\0"
#endif



/* mouse_directx_motion_handler: [input thread]
 *  Called by mouse_dinput_handle_event().
 */
static void mouse_directx_motion_handler(int dx, int dy)
{
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int new_x = the_mouse.state.x + dx;
      int new_y = the_mouse.state.y + dy;

      new_x = MID(mouse_minx, new_x, mouse_maxx);
      new_y = MID(mouse_miny, new_y, mouse_maxy);

      if ((new_x != the_mouse.state.x) || (new_y != the_mouse.state.y)) {
	 the_mouse.state.x = new_x;
	 the_mouse.state.y = new_y;

	 generate_mouse_event(
	    AL_EVENT_MOUSE_AXES,
	    the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
	    dx, dy, 0,
	    0);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_directx_motion_handler: [input thread]
 *  Called by mouse_dinput_handle().
 */
static void mouse_directx_motion_handler_abs(int x, int y)
{
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dx = x - the_mouse.state.x;
      int dy = y - the_mouse.state.y;
      the_mouse.state.x = x;
      the_mouse.state.y = y;

      generate_mouse_event(
         AL_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         dx, dy, 0,
         0);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_directx_wheel_motion_handler: [input thread]
 *  Called by mouse_dinput_handle().
 */
static void mouse_directx_wheel_motion_handler(int dz)
{
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      the_mouse.state.z += dz;

      generate_mouse_event(
         AL_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         0, 0, dz,
         0);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_directx_button_handler: [input thread]
 *  Called by mouse_dinput_handle_event().
 */
static void mouse_directx_button_handler(int unswapped_button, bool is_down)
{
   int button0;
   int dz;

   ASSERT(mouse_directx_installed);

   if (mouse_swap_button) {
      if (unswapped_button == 0)
	 button0 = 1;
      else if (unswapped_button == 1)
	 button0 = 0;
      else
	 button0 = unswapped_button;
   }
   else {
      button0 = unswapped_button;
   }

   _al_event_source_lock(&the_mouse.parent.es);
   {
      if (is_down) {
	 the_mouse.state.buttons |= (1 << button0);
      }
      else {
	 the_mouse.state.buttons &=~ (1 << button0);
      }

      generate_mouse_event(
	 is_down ?  AL_EVENT_MOUSE_BUTTON_DOWN : AL_EVENT_MOUSE_BUTTON_UP,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         0, 0, 0,
         button0 + 1);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_dinput_handle_event: [input thread]
 *  Handles a single event.
 */
static void mouse_dinput_handle_event(int ofs, int data)
{
   static int last_data_x = 0;
   static int last_data_y = 0;
   static int last_was_x = 0;
   int mag;

   switch (ofs) {

      case DIMOFS_X:
         if (!gfx_driver || !gfx_driver->windowed) {
            if (last_was_x)
               last_data_y = 0;
            last_data_x = data;
            last_was_x = 1;
            if (mouse_accel_mult) {
               mag = last_data_x*last_data_x + last_data_y*last_data_y;
               if (mag >= mouse_accel_thr2)
                  data *= (mouse_accel_mult<<1);
               else if (mag >= mouse_accel_thr1) 
                  data *= mouse_accel_mult;
            }

	    mouse_directx_motion_handler(data, 0);
         }
         break;

      case DIMOFS_Y:
         if (!gfx_driver || !gfx_driver->windowed) {
            if (!last_was_x)
               last_data_x = 0;
            last_data_y = data;
            last_was_x = 0;
            if (mouse_accel_mult) {
               mag = last_data_x*last_data_x + last_data_y*last_data_y;
               if (mag >= mouse_accel_thr2)
                  data *= (mouse_accel_mult<<1);
               else if (mag >= mouse_accel_thr1) 
                  data *= mouse_accel_mult;
            }

	    mouse_directx_motion_handler(0, data);
         }
         break;

      case DIMOFS_Z:
	 // XXX: untested yet as the mouse wheel doesn't want to work on my
	 // laptop
         if (dinput_wheel && _mouse_on) {
	    mouse_directx_wheel_motion_handler(data/120);
	 }
         break;

      case DIMOFS_BUTTON0:
	 mouse_directx_button_handler(0, data & 0x80);
         break;

      case DIMOFS_BUTTON1:
	 mouse_directx_button_handler(1, data & 0x80);
         break;

      case DIMOFS_BUTTON2:
	 mouse_directx_button_handler(2, data & 0x80);
         break;

      case DIMOFS_BUTTON3:
	 mouse_directx_button_handler(3, data & 0x80);
         break;

      default:
	 TRACE(PREFIX_I "unknown event at mouse_dinput_handle_event %d\n", ofs);
	 break;
   }
}



/* mouse_dinput_handle: [input thread]
 *  Handles queued mouse input.
 */
static void mouse_dinput_handle(void)
{
   static DIDEVICEOBJECTDATA message_buffer[DINPUT_BUFFERSIZE];
   long int waiting_messages;
   HRESULT hr;
   int i;

   /* the whole buffer is free */
   waiting_messages = DINPUT_BUFFERSIZE;

   /* fill the buffer */
   hr = IDirectInputDevice_GetDeviceData(mouse_dinput_device,
                                         sizeof(DIDEVICEOBJECTDATA),
                                         message_buffer,
                                         &waiting_messages,
                                         0);

   /* was device lost ? */
   if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
      /* reacquire device */
      _TRACE(PREFIX_W "mouse device not acquired or lost\n");
      wnd_schedule_proc(mouse_dinput_acquire);
   }
   else if (FAILED(hr)) {  /* other error? */
      _TRACE(PREFIX_E "unexpected error while filling mouse message buffer\n");
   }
   else {
      /* normally only this case should happen */
      for (i = 0; i < waiting_messages; i++) {
         mouse_dinput_handle_event(message_buffer[i].dwOfs,
                                   message_buffer[i].dwData);
      }

      if (gfx_driver && gfx_driver->windowed) {
         /* windowed input mode */
         if (!wnd_sysmenu) {
            POINT p;
	    int x, y;

	    GetCursorPos(&p);

	    p.x -= wnd_x;
	    p.y -= wnd_y;

	    if ((p.x < mouse_minx) || (p.x > mouse_maxx) ||
		(p.y < mouse_miny) || (p.y > mouse_maxy)) {
	       if (_mouse_on) {
		  _mouse_on = FALSE;
		  wnd_schedule_proc(mouse_set_syscursor);
		  mouse_directx_motion_handler_abs(
		     MID(mouse_minx, p.x, mouse_maxx),
		     MID(mouse_miny, p.y, mouse_maxy));
	       }
	    }
	    else {
	       if (!_mouse_on) {
		  _mouse_on = TRUE;
		  wnd_schedule_proc(mouse_set_syscursor);
	       }
	       mouse_directx_motion_handler_abs(p.x, p.y);
	    }
         }
      }
#if 0
      else {
         /* fullscreen input mode */
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

         if (!_mouse_on) {
            _mouse_on = TRUE;
            wnd_schedule_proc(mouse_set_syscursor);
         }

         _handle_mouse_input();
      }
#endif
   }
}



/* mouse_dinput_acquire: [window thread]
 *  Acquires the mouse device.
 */
int mouse_dinput_acquire(void)
{
   HRESULT hr;

   if (mouse_dinput_device) {
      //_mouse_b = 0;

      hr = IDirectInputDevice_Acquire(mouse_dinput_device);

      if (FAILED(hr)) {
	 _TRACE(PREFIX_E "acquire mouse failed: %s\n", dinput_err_str(hr));
	 return -1;
      }

      /* The cursor may not be in the client area
       * so we don't set _mouse_on here.
       */

      return 0;
   }
   else {
      return -1;
   }
}



/* mouse_dinput_unacquire: [window thread]
 *  Unacquires the mouse device.
 */
int mouse_dinput_unacquire(void)
{
   if (mouse_dinput_device) {
      IDirectInputDevice_Unacquire(mouse_dinput_device);

      //_mouse_b = 0;
      _mouse_on = FALSE;

      return 0;
   }
   else {
      return -1;
   }
}



/* mouse_dinput_grab: [window thread]
 *  Grabs the mouse device.
 */
int mouse_dinput_grab(void)
{
   HRESULT hr;
   DWORD level;
   HWND allegro_wnd = win_get_window();

   if (mouse_dinput_device) {
      /* necessary in order to set the cooperative level */
      mouse_dinput_unacquire();

      if (gfx_driver && !gfx_driver->windowed) {
         level = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
         _TRACE(PREFIX_I "foreground exclusive cooperative level requested for mouse\n");
      }
      else {
         level = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;
         _TRACE(PREFIX_I "foreground non-exclusive cooperative level requested for mouse\n");
      }

      /* set cooperative level */
      hr = IDirectInputDevice_SetCooperativeLevel(mouse_dinput_device, allegro_wnd, level);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "set cooperative level failed: %s\n", dinput_err_str(hr));
         return -1;
      }

      mouse_dinput_acquire();

      /* update the system cursor */
      mouse_set_syscursor();

      return 0;
   }
   else {
      /* update the system cursor */
      mouse_set_syscursor();

      return -1;
   }
}



/* mouse_set_syscursor: [window thread]
 *  Selects whatever system cursor we should display.
 */
int mouse_set_syscursor(void)
{
   HWND allegro_wnd = win_get_window();
   if ((mouse_dinput_device && _mouse_on) || (gfx_driver && !gfx_driver->windowed)) {
      SetCursor(_win_hcursor);
      /* Make sure the cursor is removed by the system. */
      PostMessage(allegro_wnd, WM_MOUSEMOVE, 0, 0);
   }
   else
      SetCursor(LoadCursor(NULL, IDC_ARROW));

   return 0;
}



/* mouse_set_sysmenu: [window thread]
 *  Changes the state of the mouse when going to/from sysmenu mode.
 */
int mouse_set_sysmenu(int state)
{
   POINT p;
   int x, y;

   if (mouse_dinput_device) {
      if (state == TRUE) {
         if (_mouse_on) {
            _mouse_on = FALSE;
            mouse_set_syscursor();
         }
      }
      else {
         READ_CURSOR_POS(p, x, y);
	 // XXX: what now?
      }

      //_handle_mouse_input();
   }

   return 0;
}



/* mouse_dinput_exit: [primary thread]
 *  Shuts down the DirectInput mouse device.
 */
static int mouse_dinput_exit(void)
{
   if (mouse_dinput_device) {
      /* unregister event handler first */
      _win_input_unregister_event(mouse_input_event);

      /* unacquire device */
      wnd_call_proc(mouse_dinput_unacquire);

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

   /* update the system cursor */
   wnd_schedule_proc(mouse_set_syscursor);

   return 0;
}



/* mouse_enum_callback: [primary thread]
 *  Helper function for finding out how many buttons we have.
 */
static BOOL CALLBACK mouse_enum_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
   if (memcmp(&lpddoi->guidType, &GUID_ZAxis, sizeof(GUID)) == 0)
      dinput_wheel = TRUE;
   else if (memcmp(&lpddoi->guidType, &GUID_Button, sizeof(GUID)) == 0)
      dinput_buttons++;

   return DIENUM_CONTINUE;
}



/* mouse_dinput_init: [primary thread]
 *  Sets up the DirectInput mouse device.
 */
static int mouse_dinput_init(void)
{
   HRESULT hr;
   DIPROPDWORD property_buf_size =
   {
      /* the header */
      {
	 sizeof(DIPROPDWORD),   // diph.dwSize
	 sizeof(DIPROPHEADER),  // diph.dwHeaderSize
	 0,                     // diph.dwObj
	 DIPH_DEVICE,           // diph.dwHow
      },

      /* the data */
      DINPUT_BUFFERSIZE,        // dwData
   };

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
   if (FAILED(hr))
      goto Error;

   /* Check to see if the buttons are swapped (left-hand) */
   mouse_swap_button = GetSystemMetrics(SM_SWAPBUTTON);   

   /* Set data format */
   hr = IDirectInputDevice_SetDataFormat(mouse_dinput_device, &c_dfDIMouse);
   if (FAILED(hr))
      goto Error;

   /* Set buffer size */
   hr = IDirectInputDevice_SetProperty(mouse_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr))
      goto Error;

   /* Enable event notification */
   mouse_input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   hr = IDirectInputDevice_SetEventNotification(mouse_dinput_device, mouse_input_event);
   if (FAILED(hr))
      goto Error;

   /* Register event handler */
   if (_win_input_register_event(mouse_input_event, mouse_dinput_handle) != 0)
      goto Error;

   /* Grab the device */
   _mouse_on = TRUE;
   wnd_call_proc(mouse_dinput_grab);

   return 0;

 Error:
   mouse_dinput_exit();
   return -1;
}



/* mouse_directx_init: [primary thread]
 */
static bool mouse_directx_init(void)
{
   char tmp1[64], tmp2[128];

   /* get user acceleration factor */
   mouse_accel_fact = get_config_int(uconvert_ascii("mouse", tmp1),
                                     uconvert_ascii("mouse_accel_factor", tmp2),
                                     MAF_DEFAULT);

   if (mouse_dinput_init() != 0) {
      /* something has gone wrong */
      _TRACE(PREFIX_E "mouse handler init failed\n");
      return false;
   }

   memset(&the_mouse, 0, sizeof the_mouse);

   _al_event_source_init(&the_mouse.parent.es);

   mouse_directx_installed = true;

   /* the mouse handler has successfully set up */
   _TRACE(PREFIX_I "mouse handler starts\n");
   return true;
}



/* mouse_directx_exit: [primary thread]
 */
static void mouse_directx_exit(void)
{
   if (mouse_dinput_device) {
      /* command mouse handler shutdown */
      _TRACE(PREFIX_I "mouse handler exits\n");
      mouse_dinput_exit();
      ASSERT(mouse_directx_installed);
      mouse_directx_installed = false;
      _al_event_source_free(&the_mouse.parent.es);
   }
}



/* mouse_directx_get_mouse:
 *  Returns the address of a AL_MOUSE structure representing the mouse.
 */
static AL_MOUSE *mouse_directx_get_mouse(void)
{
   ASSERT(mouse_directx_installed);

   return (AL_MOUSE *)&the_mouse;
}



/* mouse_directx_get_mouse_num_buttons:
 *  Return the number of buttons on the mouse.
 */
static unsigned int mouse_directx_get_mouse_num_buttons(void)
{
   ASSERT(mouse_directx_installed);

   return dinput_buttons;
}



/* mouse_directx_get_mouse_num_axes:
 *  Return the number of axes on the mouse.
 */
static unsigned int mouse_directx_get_mouse_num_axes(void)
{
   ASSERT(mouse_directx_installed);

   return (dinput_wheel) ? 3 : 2;
}



/* mouse_directx_set_mouse_xy:
 *
 */
static bool mouse_directx_set_mouse_xy(int x, int y)
{
   ASSERT(mouse_directx_installed);

   _enter_critical();

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int new_x, new_y;
      int dx, dy;

      new_x = MID(mouse_minx, x, mouse_maxx);
      new_y = MID(mouse_miny, y, mouse_maxy);

      dx = new_x - the_mouse.state.x;
      dy = new_y - the_mouse.state.y;

      if ((dx != 0) || (dy != 0)) {
	 the_mouse.state.x = x;
	 the_mouse.state.y = y;

	 generate_mouse_event(
	    AL_EVENT_MOUSE_AXES,
	    the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
	    dx, dy, 0,
	    0);
      }

      if (gfx_driver && gfx_driver->windowed) {
	 SetCursorPos(new_x+wnd_x, new_y+wnd_y);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   /* force mouse update */
   SetEvent(mouse_input_event);

   _exit_critical();

   return true;
}



/* mouse_directx_set_mouse_axis:
 *
 */
static bool mouse_directx_set_mouse_axis(int which, int z)
{
   if (which != 2) {
      return false;
   }

   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dz = (z - the_mouse.state.z);

      if (dz != 0) {
         the_mouse.state.z = z;

         generate_mouse_event(
            AL_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            0, 0, dz,
            0);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* mouse_directx_set_mouse_range:
 *
 */
static bool mouse_directx_set_mouse_range(int x1, int y1, int x2, int y2)
{
   ASSERT(mouse_directx_installed);
   
   _al_event_source_lock(&the_mouse.parent.es);
   {
      int new_x, new_y;
      int dx, dy;

      mouse_minx = x1;
      mouse_miny = y1;
      mouse_maxx = x2;
      mouse_maxy = y2;

      new_x = MID(mouse_minx, the_mouse.state.x, mouse_maxx);
      new_y = MID(mouse_miny, the_mouse.state.y, mouse_maxy);

      dx = new_x - the_mouse.state.x;
      dy = new_y - the_mouse.state.y;

      if ((dx != 0) && (dy != 0)) {
         the_mouse.state.x = new_x;
         the_mouse.state.y = new_y;

         generate_mouse_event(
            AL_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            dx, dy, 0,
            0);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* mouse_directx_get_state:
 *  Copy the current mouse state into RET_STATE, with any necessary locking.
 */
static void mouse_directx_get_state(AL_MSESTATE *ret_state)
{
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      *ret_state = the_mouse.state;
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* [bgman thread] */
static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button)
{
   AL_EVENT *event;

   if (!_al_event_source_needs_to_generate_event(&the_mouse.parent.es))
      return;

   event = _al_event_source_get_unused_event(&the_mouse.parent.es);
   if (!event)
      return;

   event->mouse.type = type;
   event->mouse.timestamp = al_current_time();
   event->mouse.__display__dont_use_yet__ = NULL; /* TODO */
   event->mouse.x = x;
   event->mouse.y = y;
   event->mouse.z = z;
   event->mouse.dx = dx;
   event->mouse.dy = dy;
   event->mouse.dz = dz;
   event->mouse.button = button;
   _al_event_source_emit_event(&the_mouse.parent.es, event);
}


//---------------------------------------------------------------------//
//---------------------------------------------------------------------//

/* mouse_directx_position: [primary thread]
 */
#if 0
static void mouse_directx_position(int x, int y)
{
   _enter_critical();

   _mouse_x = x;
   _mouse_y = y;

   mouse_mx = COORD_TO_MICKEY_X(x);
   mouse_my = COORD_TO_MICKEY_Y(y);

   if (gfx_driver && gfx_driver->windowed)
      SetCursorPos(x+wnd_x, y+wnd_y);

   CLEAR_MICKEYS();

   mymickey_x = mymickey_y = 0;

   /* force mouse update */
   SetEvent(mouse_input_event);

   _exit_critical();
}
#endif



/* mouse_directx_set_range: [primary thread]
 */
#if 0
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

   /* scale up the acceleration multiplier to the range */
   mouse_accel_mult = mouse_accel_fact * MAX(x2-x1+1, y2-y1+1)/320;
}
#endif /* 0 */



/* mouse_directx_set_speed: [primary thread]
 */
#if 0
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
#endif /* 0 */



/* mouse_directx_get_mickeys: [primary thread]
 */
#if 0
static void mouse_directx_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}
#endif



/* mouse_directx_enable_hardware_cursor:
 *  enable the hardware cursor; actually a no-op in Windows, but we need to
 *  put something in the vtable.
 */
#if 0
static void mouse_directx_enable_hardware_cursor(int mode)
{
   /* Do nothing */
   (void)mode;
}
#endif



/* mouse_directx_select_system_cursor:
 *  Select an OS native cursor 
 */
#if 0
static int mouse_directx_select_system_cursor (AL_CONST int cursor)
{
   HCURSOR wc;
   HWND allegro_wnd = win_get_window();
   
   wc = NULL;
   switch(cursor) {
      case MOUSE_CURSOR_ARROW:
         wc = LoadCursor(NULL, IDC_ARROW);
         break;
      case MOUSE_CURSOR_BUSY:
         wc = LoadCursor(NULL, IDC_WAIT);
         break;
      case MOUSE_CURSOR_QUESTION:
         wc = LoadCursor(NULL, IDC_HELP);
         break;
      case MOUSE_CURSOR_EDIT:
         wc = LoadCursor(NULL, IDC_IBEAM);
         break;
      default:
         return 0;
   }

   _win_hcursor = wc;
   SetCursor(_win_hcursor);
   PostMessage(allegro_wnd, WM_MOUSEMOVE, 0, 0);
   
   return cursor;
}
#endif
