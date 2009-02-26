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
 *      Rewritten to handle multiple A5 windows by Milan Mimica.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTINPUT_VERSION 0x0800

#define ALLEGRO_NO_COMPATIBILITY

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_display.h"

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

#define CLAMP _ALLEGRO_CLAMP


typedef struct AL_MOUSE_DIRECTX
{
   ALLEGRO_MOUSE parent;
   ALLEGRO_MOUSE_STATE state;
} AL_MOUSE_DIRECTX;


/* forward declarations */
static void mouse_set_syscursor(void* unused);

static bool mouse_directx_installed = false;

/* the one and only mouse object */
static AL_MOUSE_DIRECTX the_mouse;


#define DINPUT_BUFFERSIZE 256
static HANDLE mouse_input_event = NULL;
static LPDIRECTINPUT mouse_dinput = NULL;
static LPDIRECTINPUTDEVICE mouse_dinput_device = NULL;

static int dinput_buttons = 0;
static int dinput_wheel = false;

static int mouse_swap_button = false;     /* true means buttons 1 and 2 are swapped */

static int mouse_sx = 2;              /* mickey -> pixel scaling factor */
static int mouse_sy = 2;

#define MAF_DEFAULT 1                 /* mouse acceleration parameters */
static int mouse_accel_fact = MAF_DEFAULT;
static int mouse_accel_mult = MAF_DEFAULT;
static int mouse_accel_thr1 = 5 * 5;
static int mouse_accel_thr2 = 16 * 16;

/* XXX: should this be per-display? */
static int mymickey_x = 0;            /* for get_mouse_mickeys() */
static int mymickey_y = 0;
static int mymickey_ox = 0;
static int mymickey_oy = 0;

/* XXX: should this be per-display? */
static int _mouse_x = 0;
static int _mouse_y = 0;
static int dinput_x = 0;
static int dinput_y = 0;
static int mouse_mx = 0;
static int mouse_my = 0;

/* The display which has the mouse acquired. */
static ALLEGRO_DISPLAY_WIN *win_disp = NULL;

#define MICKEY_TO_COORD_X(n)        ((n) / mouse_sx)
#define MICKEY_TO_COORD_Y(n)        ((n) / mouse_sy)

#define COORD_TO_MICKEY_X(n)        ((n) * mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * mouse_sy)

#define CLEAR_MICKEYS()                       \
{                                             \
   if (!(win_disp->display.flags & ALLEGRO_FULLSCREEN)) { \
      POINT p;                                \
      int wx, wy;                             \
                                              \
      GetCursorPos(&p);                       \
                                              \
      _al_win_get_window_position(win_disp->window, &wx, &wy); \
      p.x -= wx;                              \
      p.y -= wy;                              \
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
   int wx, wy;                                      \
   GetCursorPos(&p);                                \
   _al_win_get_window_position(win_disp->window, &wx, &wy); \
                                                    \
   p.x -= wx;                                       \
   p.y -= wy;                                       \
                                                    \
   if ((p.x < win_disp->mouse_range_x1) ||          \
       (p.x > win_disp->mouse_range_x2) ||          \
       (p.y < win_disp->mouse_range_y1) ||          \
       (p.y > win_disp->mouse_range_y2)) {          \
      if (win_disp->is_mouse_on) {             \
         win_disp->is_mouse_on = false;        \
         _al_win_wnd_schedule_proc(win_disp->window, mouse_set_syscursor, NULL); \
      }                                             \
   }                                                \
   else {                                           \
      if (!win_disp->is_mouse_on) {            \
         win_disp->is_mouse_on = true;         \
         _al_win_wnd_schedule_proc(win_disp->window, mouse_set_syscursor, NULL); \
      }                                             \
   }                                                \
                                                    \
   out_x = CLAMP(win_disp->mouse_range_x1,          \
                 p.x, win_disp->mouse_range_x2);    \
   out_y = CLAMP(win_disp->mouse_range_y1, p.y,     \
                 win_disp->mouse_range_y2);         \
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


/* [bgman thread] */
static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button,
                                 ALLEGRO_DISPLAY *source)
{
   ALLEGRO_EVENT event;

   if (!_al_event_source_needs_to_generate_event(&the_mouse.parent.es))
      return;

   event.mouse.type = type;
   event.mouse.timestamp = al_current_time();
   event.mouse.display = source;
   event.mouse.x = x;
   event.mouse.y = y;
   event.mouse.z = z;
   event.mouse.dx = dx;
   event.mouse.dy = dy;
   event.mouse.dz = dz;
   event.mouse.button = button;
   _al_event_source_emit_event(&the_mouse.parent.es, &event);
}


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

      new_x = CLAMP(win_disp->mouse_range_x1, new_x, win_disp->mouse_range_x2);
      new_y = CLAMP(win_disp->mouse_range_y1, new_y, win_disp->mouse_range_y2);

      if ((new_x != the_mouse.state.x) || (new_y != the_mouse.state.y)) {
         the_mouse.state.x = new_x;
         the_mouse.state.y = new_y;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            dx, dy, 0,
            0, (void*)win_disp);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_directx_motion_handler: [input thread]
 *  Called by mouse_dinput_handle().
 */
static void mouse_directx_motion_handler_abs(int x, int y)
{
   ALLEGRO_DISPLAY *d = (void*)win_disp;
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dx = x - the_mouse.state.x;
      int dy = y - the_mouse.state.y;
      the_mouse.state.x = x;
      the_mouse.state.y = y;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         dx, dy, 0,
         0, d);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_directx_wheel_motion_handler: [input thread]
 *  Called by mouse_dinput_handle().
 */
static void mouse_directx_wheel_motion_handler(int dz)
{
   ALLEGRO_DISPLAY *d = (void*)win_disp;
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      the_mouse.state.z += dz;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         0, 0, dz,
         0, d);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_directx_button_handler: [input thread]
 *  Called by mouse_dinput_handle_event().
 */
static void mouse_directx_button_handler(int unswapped_button, bool is_down)
{
   ALLEGRO_DISPLAY *d = (void*)win_disp;
   int button0;

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
         is_down ?  ALLEGRO_EVENT_MOUSE_BUTTON_DOWN : ALLEGRO_EVENT_MOUSE_BUTTON_UP,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         0, 0, 0,
         button0 + 1, d);
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* mouse_dinput_handle_event: [input thread]
 *  Handles a single event.
 */
static void mouse_dinput_handle_event(int ofs, int data)
{
   ALLEGRO_DISPLAY *d = (void*)win_disp;
   //XXX make this per display, maybe... probably
   static int last_data_x = 0;
   static int last_data_y = 0;
   static int last_was_x = 0;
   int mag;

   switch (ofs) {

      case DIMOFS_X:
         if (d->flags & ALLEGRO_FULLSCREEN) {
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
         if (d->flags & ALLEGRO_FULLSCREEN) {
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
         if (dinput_wheel && win_disp->is_mouse_on) {
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
   hr = IDirectInputDevice8_GetDeviceData(mouse_dinput_device,
                                         sizeof(DIDEVICEOBJECTDATA),
                                         message_buffer,
                                         (unsigned long int *)&waiting_messages,
                                         0);

   /* was device lost ? */
   if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST) || !win_disp) {
      /* reacquire device */
      TRACE(PREFIX_W "mouse device not acquired or lost\n");
      /*_al_win_wnd_schedule_proc(win_disp->window, mouse_dinput_acquire, NULL);*/
      /* Makes no sense to reacquire the device here becasue this usually 
       * happens when the window looses focus. */
   }
   else if (FAILED(hr)) {  /* other error? */
      TRACE(PREFIX_E "unexpected error while filling mouse message buffer\n");
   }
   else {
      /* normally only this case should happen */
      for (i = 0; i < waiting_messages && win_disp->is_mouse_on; i++) {
         mouse_dinput_handle_event(message_buffer[i].dwOfs,
                                   message_buffer[i].dwData);
      }

      if (!(win_disp->display.flags & ALLEGRO_FULLSCREEN)) {
         /* windowed input mode */
         if (!win_disp->is_in_sysmenu) {
            POINT p;
            int wx, wy;

            GetCursorPos(&p);

            _al_win_get_window_position(win_disp->window, &wx, &wy);

            p.x -= wx;
            p.y -= wy;

            if ((p.x < win_disp->mouse_range_x1) ||
                (p.x > win_disp->mouse_range_x2) ||
                (p.y < win_disp->mouse_range_y1) ||
                (p.y > win_disp->mouse_range_y2)) {
               if (win_disp->is_mouse_on) {
                  win_disp->is_mouse_on = false;
                  _al_win_wnd_schedule_proc(win_disp->window,
                                            mouse_set_syscursor,
                                            NULL);
                  mouse_directx_motion_handler_abs(
                     CLAMP(win_disp->mouse_range_x1,
                           p.x, win_disp->mouse_range_x2),
                     CLAMP(win_disp->mouse_range_y1,
                           p.y, win_disp->mouse_range_y2));
               }
            }
            else {
               if (!win_disp->is_mouse_on) {
                  win_disp->is_mouse_on = true;
                  _al_win_wnd_schedule_proc(win_disp->window, mouse_set_syscursor, NULL);
               }
               mouse_directx_motion_handler_abs(p.x, p.y);
            }
         }
      }
      else {
         /* fullscreen input mode */
         mymickey_x += dinput_x - mymickey_ox;
         mymickey_y += dinput_y - mymickey_oy;

         mymickey_ox = dinput_x;
         mymickey_oy = dinput_y;

         _mouse_x = MICKEY_TO_COORD_X(mouse_mx + dinput_x);
         _mouse_y = MICKEY_TO_COORD_Y(mouse_my + dinput_y);

         if ((_mouse_x < win_disp->mouse_range_x1) ||
             (_mouse_x > win_disp->mouse_range_x2) ||
             (_mouse_y < win_disp->mouse_range_y1) ||
             (_mouse_y > win_disp->mouse_range_y2)) {

            _mouse_x = CLAMP(win_disp->mouse_range_x1,
                             _mouse_x, win_disp->mouse_range_x2);
            _mouse_y = CLAMP(win_disp->mouse_range_y1,
                             _mouse_y, win_disp->mouse_range_y2);

            mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
            mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

            CLEAR_MICKEYS();
         }

         if (!win_disp->is_mouse_on) {
            win_disp->is_mouse_on = true;
            _al_win_wnd_schedule_proc(win_disp->window, mouse_set_syscursor, NULL);
         }
      }
   }
}


/* mouse_set_syscursor: [window thread]
 *  Selects whatever system cursor we should display.
 */
static void mouse_set_syscursor(void *unused)
{
   (void)unused;

   if ((mouse_dinput_device && win_disp->is_mouse_on) ||
       (win_disp->display.flags & ALLEGRO_FULLSCREEN)) {
      SetCursor(win_disp->mouse_selected_hcursor);
      /* Make sure the cursor is removed by the system. */
      PostMessage(win_disp->window, WM_MOUSEMOVE, 0, 0);
   }
   else
      SetCursor(LoadCursor(NULL, IDC_ARROW));
}


/* mouse_dinput_acquire: [window thread]
 *  Acquires the mouse device.
 */
static void mouse_dinput_acquire(void)
{
   HRESULT hr;

   if (mouse_dinput_device) {
      the_mouse.state.buttons = 0;

      hr = IDirectInputDevice8_Acquire(mouse_dinput_device);

      if (FAILED(hr)) {
         TRACE(PREFIX_E "acquire mouse failed: %s\n", dinput_err_str(hr));
         return;
      }

      /* The cursor may not be in the client area
       * so we don't set win_disp->is_mouse_on here.
       */
   }
}



/* _al_win_mouse_dinput_unacquire: [window thread]
 *  Unacquires the mouse device.
 */
void _al_win_mouse_dinput_unacquire(void *param)
{
   /* Unacquiring a device that is acquired by a different display. This can
    * happen if for example the unacquire command arrives after the other
    * display has already grabbed the device, since these messages are sent
    * asynchronously to the window thread message queue. */
   if (param != win_disp) {
      return;
   }

   if (mouse_dinput_device) {
      IDirectInputDevice8_Unacquire(mouse_dinput_device);
      the_mouse.state.buttons = 0;
   }

   win_disp->is_mouse_on = false;
}



/* _al_win_mouse_dinput_grab: [window thread]
 *  Grabs the mouse device, ungrabs from the previous display.
 */
void _al_win_mouse_dinput_grab(void *param)
{
   HRESULT hr;
   DWORD level;

   if (mouse_dinput_device) {
      /* Release the input from the previous window just in case,
         otherwise set cooperative level will fail. */
      if (win_disp)
         _al_win_wnd_call_proc(win_disp->window, _al_win_mouse_dinput_unacquire, win_disp);

      win_disp = param;

      if (win_disp->display.flags & ALLEGRO_FULLSCREEN) {
         level = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
      }
      else {
         level = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;
      }

      /* set cooperative level */
      hr = IDirectInputDevice8_SetCooperativeLevel(mouse_dinput_device,
                                                   win_disp->window, level);
      if (FAILED(hr)) {
         TRACE(PREFIX_E "set cooperative level failed: %s\n", dinput_err_str(hr));
         return;
      }

      mouse_dinput_acquire();

      /* update the system cursor */
      mouse_set_syscursor(NULL);
   }
   else {
      /* update the system cursor (why??)*/
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
}


/* mouse_set_sysmenu: [window thread]
 *  Changes the state of the mouse when going to/from sysmenu mode.
 */
void _al_win_mouse_set_sysmenu(bool state)
{
   POINT p;
   int x, y;

   if (mouse_dinput_device) {
      if (state) {
         if (win_disp->is_mouse_on) {
            win_disp->is_mouse_on = false;
            mouse_set_syscursor(NULL);
         }
      }
      else {
         READ_CURSOR_POS(p, x, y);
	 // XXX: what now?
      }

      //_handle_mouse_input();
   }
}



/* mouse_dinput_exit: [primary thread]
 *  Shuts down the DirectInput mouse device.
 */
static int mouse_dinput_exit(void)
{
   if (mouse_dinput_device) {
      /* unregister event handler first */
      _al_win_input_unregister_event(mouse_input_event);

      /* now it can be released */
      IDirectInputDevice8_Release(mouse_dinput_device);
      mouse_dinput_device = NULL;
   }

   /* release DirectInput interface */
   if (mouse_dinput) {
      IDirectInput8_Release(mouse_dinput);
      mouse_dinput = NULL;
   }

   /* close event handle */
   if (mouse_input_event) {
      CloseHandle(mouse_input_event);
      mouse_input_event = NULL;
   }

   return 0;
}



/* mouse_enum_callback: [primary thread]
 *  Helper function for finding out how many buttons we have.
 */
static BOOL CALLBACK mouse_enum_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
   (void)pvRef;

   if (memcmp(&lpddoi->guidType, &GUID_ZAxis, sizeof(GUID)) == 0)
      dinput_wheel = true;
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
   MAKE_UNION(&mouse_dinput, LPDIRECTINPUT *);

   /* Get DirectInput interface */
   hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, u.v, NULL);
   if (FAILED(hr))
      goto Error;

   /* Create the mouse device */
   hr = IDirectInput8_CreateDevice(mouse_dinput, &GUID_SysMouse, &mouse_dinput_device, NULL);
   if (FAILED(hr))
      goto Error;

   /* Find how many buttons */
   dinput_buttons = 0;
   dinput_wheel = false;

   hr = IDirectInputDevice8_EnumObjects(mouse_dinput_device, mouse_enum_callback, NULL, DIDFT_PSHBUTTON | DIDFT_AXIS);
   if (FAILED(hr))
      goto Error;

   /* Check to see if the buttons are swapped (left-hand) */
   mouse_swap_button = GetSystemMetrics(SM_SWAPBUTTON);   

   /* Set data format */
   hr = IDirectInputDevice8_SetDataFormat(mouse_dinput_device, &c_dfDIMouse);
   if (FAILED(hr))
      goto Error;

   /* Set buffer size */
   hr = IDirectInputDevice8_SetProperty(mouse_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr))
      goto Error;

   /* Enable event notification */
   mouse_input_event = CreateEvent(NULL, false, false, NULL);
   hr = IDirectInputDevice8_SetEventNotification(mouse_dinput_device, mouse_input_event);
   if (FAILED(hr))
      goto Error;

   /* Register event handler */
   if (!_al_win_input_register_event(mouse_input_event, mouse_dinput_handle) != 0)
      goto Error;

   return 0;

 Error:
   mouse_dinput_exit();
   return -1;
}



/* mouse_directx_init: [primary thread] [vtable entry]
 */
static bool mouse_directx_init(void)
{
   ALLEGRO_SYSTEM *system;
   size_t i;

   /* get user acceleration factor */
   mouse_accel_fact = 1; /*get_config_int(uconvert_ascii("mouse", tmp1),
                                     uconvert_ascii("mouse_accel_factor", tmp2),
                                     MAF_DEFAULT);*/

   if (mouse_dinput_init() != 0) {
      /* something has gone wrong */
      TRACE(PREFIX_E "mouse handler init failed\n");
      return false;
   }

   /* If one of our windows is the foreground window make it grab the input. */
   system = al_system_driver();
   for (i = 0; i < _al_vector_size(&system->displays); i++) {
      ALLEGRO_DISPLAY_WIN **pwin_disp = _al_vector_ref(&system->displays, i);
      ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
      if (win_disp->window == GetForegroundWindow()) {
         win_disp->is_mouse_on = true;
         _al_win_wnd_call_proc(win_disp->window,
            _al_win_mouse_dinput_grab, win_disp);
      }
   }

   memset(&the_mouse, 0, sizeof the_mouse);

   _al_event_source_init(&the_mouse.parent.es);

   mouse_directx_installed = true;

   /* the mouse handler has successfully set up */
   TRACE(PREFIX_I "mouse handler starts\n");
   return true;
}



/* mouse_directx_exit: [primary thread] [vtable entry]
 */
static void mouse_directx_exit(void)
{
   ALLEGRO_SYSTEM *system;
   size_t i;

   if (mouse_dinput_device) {
      /* command mouse handler shutdown */
      TRACE(PREFIX_I "mouse handler exits\n");

      /* If one of our windows is the foreground window release the input
         from it. */
      system = al_system_driver();
      for (i = 0; i < _al_vector_size(&system->displays); i++) {
         ALLEGRO_DISPLAY_WIN **pwin_disp = _al_vector_ref(&system->displays, i);
         ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
         if (win_disp->window == GetForegroundWindow())
            _al_win_wnd_call_proc(win_disp->window,
                                  _al_win_mouse_dinput_unacquire,
                                  win_disp);
      }

      mouse_dinput_exit();

      ASSERT(mouse_directx_installed);
      mouse_directx_installed = false;

      _al_event_source_free(&the_mouse.parent.es);
   }
}



/* mouse_directx_get_mouse: [vtable entry]
 *  Returns the address of a ALLEGRO_MOUSE structure representing the mouse.
 */
static ALLEGRO_MOUSE *mouse_directx_get_mouse(void)
{
   ASSERT(mouse_directx_installed);

   return (ALLEGRO_MOUSE *)&the_mouse;
}



/* mouse_directx_get_mouse_num_buttons: [vtable entry]
 *  Return the number of buttons on the mouse.
 */
static unsigned int mouse_directx_get_mouse_num_buttons(void)
{
   ASSERT(mouse_directx_installed);

   return dinput_buttons;
}



/* mouse_directx_get_mouse_num_axes: [vtable entry]
 *  Return the number of axes on the mouse.
 */
static unsigned int mouse_directx_get_mouse_num_axes(void)
{
   ASSERT(mouse_directx_installed);

   return (dinput_wheel) ? 3 : 2;
}



/* mouse_directx_set_mouse_xy: [vtable entry]
 *
 */
static bool mouse_directx_set_mouse_xy(int x, int y)
{
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)al_get_current_display();
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int new_x, new_y;
      int dx, dy;
      int wx, wy;

      new_x = CLAMP(win_disp->mouse_range_x1, x, win_disp->mouse_range_x2);
      new_y = CLAMP(win_disp->mouse_range_y1, y, win_disp->mouse_range_y2);

      dx = new_x - the_mouse.state.x;
      dy = new_y - the_mouse.state.y;

      if ((dx != 0) || (dy != 0)) {
         the_mouse.state.x = x;
         the_mouse.state.y = y;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            dx, dy, 0,
            0, (void*)win_disp);
      }

      _al_win_get_window_position(win_disp->window, &wx, &wy);

      if (!(win_disp->display.flags & ALLEGRO_FULLSCREEN)) {
         SetCursorPos(new_x+wx, new_y+wy);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   /* force mouse update */
   SetEvent(mouse_input_event);

   return true;
}



/* mouse_directx_set_mouse_axis: [vtable entry]
 *
 */
static bool mouse_directx_set_mouse_axis(int which, int z)
{
   //XXX Which display to event?
   ALLEGRO_DISPLAY *disp = al_get_current_display();

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
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            0, 0, dz,
            0, disp);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* mouse_directx_set_mouse_range: [vtable entry]
 *
 */
static bool mouse_directx_set_mouse_range(int x1, int y1, int x2, int y2)
{
   //XXX Decide which display does al_set_mouse_range() affect.
   ALLEGRO_DISPLAY_WIN *win_disp = (void*)al_get_current_display();

   ASSERT(mouse_directx_installed);
   
   _al_event_source_lock(&the_mouse.parent.es);
   {
      int new_x, new_y;
      int dx, dy;

      win_disp->mouse_range_x1 = x1;
      win_disp->mouse_range_y1 = y1;
      win_disp->mouse_range_x2 = x2;
      win_disp->mouse_range_y2 = y2;

      new_x = CLAMP(win_disp->mouse_range_x1, the_mouse.state.x, win_disp->mouse_range_x2);
      new_y = CLAMP(win_disp->mouse_range_y1, the_mouse.state.y, win_disp->mouse_range_y2);

      dx = new_x - the_mouse.state.x;
      dy = new_y - the_mouse.state.y;

      if ((dx != 0) && (dy != 0)) {
         the_mouse.state.x = new_x;
         the_mouse.state.y = new_y;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            dx, dy, 0,
            0, (void*)win_disp);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* mouse_directx_get_state: [vtable entry]
 *  Copy the current mouse state into RET_STATE, with any necessary locking.
 */
static void mouse_directx_get_state(ALLEGRO_MOUSE_STATE *ret_state)
{
   ASSERT(mouse_directx_installed);

   _al_event_source_lock(&the_mouse.parent.es);
   {
      if (win_disp)
         the_mouse.state.display = win_disp->is_mouse_on ? (void*)win_disp : NULL;
      else
         the_mouse.state.display = NULL;

      *ret_state = the_mouse.state;
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* the driver vtable */
#define MOUSEDRV_XWIN  AL_ID('X','W','I','N')

static ALLEGRO_MOUSE_DRIVER mousedrv_directx =
{
   MOUSE_DIRECTX,
   "",
   "",
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
   {MOUSE_DIRECTX, &mousedrv_directx, true},
   {0, NULL, 0}
};


/* vim: set sts=3 sw=3 et: */
