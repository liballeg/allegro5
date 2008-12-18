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
 *      Windows keyboard driver.
 *
 *      By Stefan Schimanski.
 *
 *      Modified for the new keyboard API by Peter Wang.
 *
 *      Rewritten to work with multiple A5 windows by Milan Mimica.
 *
 *      See readme.txt for copyright information.
 *
 */


#define ALLEGRO_NO_COMPATIBILITY

/* For waitable timers */
#define _WIN32_WINNT 0x400

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #define DIRECTINPUT_VERSION 0x0800
   #include <dinput.h>
   #include <process.h>
   #include <wchar.h>
#endif


#define PREFIX_I                "al-wkey INFO: "
#define PREFIX_W                "al-wkey WARNING: "
#define PREFIX_E                "al-wkey ERROR: "



#define DINPUT_BUFFERSIZE 256



static LPDIRECTINPUT8 key_dinput;
static LPDIRECTINPUTDEVICE8 key_dinput_device;

static HANDLE key_input_event;
static HANDLE key_autorepeat_timer;

static LARGE_INTEGER repeat_delay;
static LONG repeat_period;
static unsigned int key_modifiers;
static int key_scancode_to_repeat;

static ALLEGRO_DISPLAY_WIN *win_disp = NULL;

/* the one and only keyboard object and its internal state */
static ALLEGRO_KEYBOARD the_keyboard;
static ALLEGRO_KEYBOARD_STATE key_state;


/* forward declarations */
static void handle_key_press(unsigned char scancode);
static void handle_key_release(unsigned char scancode);


/* dinput_err_str:
 *  Returns a DirectInput error string.
 */
#ifdef DEBUGMODE
static char* dinput_err_str(long err)
{
   static char err_str[64];

   switch (err) {

      case DIERR_NOTACQUIRED:
         _al_sane_strncpy(err_str, "the device is not acquired", sizeof(err_str));
         break;

      case DIERR_INPUTLOST:
         _al_sane_strncpy(err_str, "access to the device was lost", sizeof(err_str));
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



/* Update the key_shifts.
 */
static void update_modifiers(BYTE *keystate)
{
   /* TODO: There must be a more efficient way to maintain key_modifiers? */
   /* Can't we just deprecate key_shifts, now that pckeys.c is gone? EP */
   unsigned int modifiers = 0;

   if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
       modifiers |= ALLEGRO_KEYMOD_SHIFT;
   if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
       modifiers |= ALLEGRO_KEYMOD_CTRL;
   if (GetAsyncKeyState(VK_MENU) & 0x8000)
       modifiers |= ALLEGRO_KEYMOD_ALT;

   if (keystate[VK_LMENU] & 0x80)
      modifiers |= ALLEGRO_KEYMOD_ALT;
   else if (keystate[VK_RMENU] & 0x80)
      modifiers |= ALLEGRO_KEYMOD_ALTGR;

   if (keystate[VK_SCROLL] & 1)
      modifiers |= ALLEGRO_KEYMOD_SCROLLLOCK;
   if (keystate[VK_NUMLOCK] & 1)
      modifiers |= ALLEGRO_KEYMOD_NUMLOCK;
   if (keystate[VK_CAPITAL] & 1)
      modifiers |= ALLEGRO_KEYMOD_CAPSLOCK;

   /* needed for special handling in key_dinput_handle_scancode */
   key_modifiers = modifiers;

}



/* lookup table for converting DIK_* scancodes into Allegro ALLEGRO_KEY_* codes */
/* this table was from pckeys.c  */
static const unsigned char hw_to_mycode[256] =
{
   /* 0x00 */  0,		  ALLEGRO_KEY_ESCAPE,     ALLEGRO_KEY_1,          ALLEGRO_KEY_2, 
   /* 0x04 */  ALLEGRO_KEY_3,          ALLEGRO_KEY_4,          ALLEGRO_KEY_5,          ALLEGRO_KEY_6,
   /* 0x08 */  ALLEGRO_KEY_7,          ALLEGRO_KEY_8,          ALLEGRO_KEY_9,          ALLEGRO_KEY_0, 
   /* 0x0C */  ALLEGRO_KEY_MINUS,      ALLEGRO_KEY_EQUALS,     ALLEGRO_KEY_BACKSPACE,  ALLEGRO_KEY_TAB,
   /* 0x10 */  ALLEGRO_KEY_Q,          ALLEGRO_KEY_W,          ALLEGRO_KEY_E,          ALLEGRO_KEY_R, 
   /* 0x14 */  ALLEGRO_KEY_T,          ALLEGRO_KEY_Y,          ALLEGRO_KEY_U,          ALLEGRO_KEY_I,
   /* 0x18 */  ALLEGRO_KEY_O,          ALLEGRO_KEY_P,          ALLEGRO_KEY_OPENBRACE,  ALLEGRO_KEY_CLOSEBRACE, 
   /* 0x1C */  ALLEGRO_KEY_ENTER,      ALLEGRO_KEY_LCTRL,	     ALLEGRO_KEY_A,          ALLEGRO_KEY_S,
   /* 0x20 */  ALLEGRO_KEY_D,          ALLEGRO_KEY_F,          ALLEGRO_KEY_G,          ALLEGRO_KEY_H, 
   /* 0x24 */  ALLEGRO_KEY_J,          ALLEGRO_KEY_K,          ALLEGRO_KEY_L,          ALLEGRO_KEY_SEMICOLON,
   /* 0x28 */  ALLEGRO_KEY_QUOTE,      ALLEGRO_KEY_TILDE,      ALLEGRO_KEY_LSHIFT,     ALLEGRO_KEY_BACKSLASH, 
   /* 0x2C */  ALLEGRO_KEY_Z,          ALLEGRO_KEY_X,          ALLEGRO_KEY_C,          ALLEGRO_KEY_V,
   /* 0x30 */  ALLEGRO_KEY_B,          ALLEGRO_KEY_N,          ALLEGRO_KEY_M,          ALLEGRO_KEY_COMMA, 
   /* 0x34 */  ALLEGRO_KEY_FULLSTOP,   ALLEGRO_KEY_SLASH,      ALLEGRO_KEY_RSHIFT,     ALLEGRO_KEY_PAD_ASTERISK,
   /* 0x38 */  ALLEGRO_KEY_ALT,        ALLEGRO_KEY_SPACE,      ALLEGRO_KEY_CAPSLOCK,   ALLEGRO_KEY_F1, 
   /* 0x3C */  ALLEGRO_KEY_F2,         ALLEGRO_KEY_F3,         ALLEGRO_KEY_F4,         ALLEGRO_KEY_F5,
   /* 0x40 */  ALLEGRO_KEY_F6,         ALLEGRO_KEY_F7,         ALLEGRO_KEY_F8,         ALLEGRO_KEY_F9, 
   /* 0x44 */  ALLEGRO_KEY_F10,        ALLEGRO_KEY_NUMLOCK,    ALLEGRO_KEY_SCROLLLOCK, ALLEGRO_KEY_PAD_7,
   /* 0x48 */  ALLEGRO_KEY_PAD_8,      ALLEGRO_KEY_PAD_9,      ALLEGRO_KEY_PAD_MINUS,  ALLEGRO_KEY_PAD_4, 
   /* 0x4C */  ALLEGRO_KEY_PAD_5,      ALLEGRO_KEY_PAD_6,      ALLEGRO_KEY_PAD_PLUS,   ALLEGRO_KEY_PAD_1,
   /* 0x50 */  ALLEGRO_KEY_PAD_2,      ALLEGRO_KEY_PAD_3,      ALLEGRO_KEY_PAD_0,      ALLEGRO_KEY_PAD_DELETE, 
   /* 0x54 */  ALLEGRO_KEY_PRINTSCREEN,0,		     ALLEGRO_KEY_BACKSLASH2, ALLEGRO_KEY_F11,
   /* 0x58 */  ALLEGRO_KEY_F12,        0,		     0,		        ALLEGRO_KEY_LWIN, 
   /* 0x5C */  ALLEGRO_KEY_RWIN,       ALLEGRO_KEY_MENU,       0,                 0,
   /* 0x60 */  0,                 0,                 0,                 0, 
   /* 0x64 */  0,                 0,                 0,                 0,
   /* 0x68 */  0,                 0,                 0,                 0, 
   /* 0x6C */  0,                 0,                 0,                 0,
   /* 0x70 */  ALLEGRO_KEY_KANA,       0,                 0,                 ALLEGRO_KEY_ABNT_C1, 
   /* 0x74 */  0,                 0,                 0,                 0,
   /* 0x78 */  0,                 ALLEGRO_KEY_CONVERT,    0,                 ALLEGRO_KEY_NOCONVERT, 
   /* 0x7C */  0,                 ALLEGRO_KEY_YEN,        0,                 0,

   /* 0x80 */  0,                 0,                 0,                 0,
   /* 0x84 */  0,                 0,                 0,                 0,
   /* 0x88 */  0,                 0,                 0,                 0,
   /* 0x8C */  0,                 0,                 0,                 0,
   /* 0x90 */  0,                 ALLEGRO_KEY_AT,         ALLEGRO_KEY_COLON2,     0,
   /* 0x94 */  ALLEGRO_KEY_KANJI,      0,                 0,                 0,
   /* 0x98 */  0,                 0,                 0,                 0,
   /* 0x9C */  ALLEGRO_KEY_PAD_ENTER,  ALLEGRO_KEY_RCTRL,      0,                 0,
   /* 0xA0 */  0,                 0,                 0,                 0,
   /* 0xA4 */  0,                 0,                 0,                 0,
   /* 0xA8 */  0,                 0,                 0,                 0,
   /* 0xAC */  0,                 0,                 0,                 0,
   /* 0xB0 */  0,                 0,                 0,                 0,
   /* 0xB4 */  0,                 ALLEGRO_KEY_PAD_SLASH,  0,                 ALLEGRO_KEY_PRINTSCREEN,
   /* 0xB8 */  ALLEGRO_KEY_ALTGR,      0,                 0,                 0,
   /* 0xBC */  0,                 0,                 0,                 0,
   /* 0xC0 */  0,                 0,                 0,                 0,
   /* 0xC4 */  0,                 ALLEGRO_KEY_PAUSE,      0,                 ALLEGRO_KEY_HOME,
   /* 0xC8 */  ALLEGRO_KEY_UP,         ALLEGRO_KEY_PGUP,       0,                 ALLEGRO_KEY_LEFT,
   /* 0xCC */  0,                 ALLEGRO_KEY_RIGHT,      0,                 ALLEGRO_KEY_END,
   /* 0xD0 */  ALLEGRO_KEY_DOWN,       ALLEGRO_KEY_PGDN,       ALLEGRO_KEY_INSERT,     ALLEGRO_KEY_DELETE,
   /* 0xD4 */  0,                 0,                 0,                 0,
   /* 0xD8 */  0,                 0,                 0,                 ALLEGRO_KEY_LWIN,
   /* 0xDC */  ALLEGRO_KEY_RWIN,       ALLEGRO_KEY_MENU,       0,                 0,
   /* 0xE0 */  0,                 0,                 0,                 0,
   /* 0xE4 */  0,                 0,                 0,                 0,
   /* 0xE8 */  0,                 0,                 0,                 0,
   /* 0xEC */  0,                 0,                 0,                 0,
   /* 0xF0 */  0,                 0,                 0,                 0,
   /* 0xF4 */  0,                 0,                 0,                 0,
   /* 0xF8 */  0,                 0,                 0,                 0,
   /* 0xFC */  0,                 0,                 0,                 0
};



/* key_dinput_handle_scancode: [input thread]
 *  Handles a single scancode.
 *  The keyboard object should be locked.
 */
static void key_dinput_handle_scancode(unsigned char scancode, int pressed)
{
   /* Windows seems to send lots of ctrl-alt-XXX key combos in response to the
    * ctrl-alt-del combo. We want to ignore them all, especially ctrl-alt-end,
    * which would cause Allegro to terminate.
    */
   static int ignore_three_finger_flag = FALSE;

   /* ignore special Windows keys (alt+tab, alt+space, (ctrl|alt)+esc) */
   if (((scancode == DIK_TAB) && (key_modifiers & (ALLEGRO_KEYMOD_ALT | ALLEGRO_KEYMOD_ALTGR)))
       || ((scancode == DIK_SPACE) && (key_modifiers & (ALLEGRO_KEYMOD_ALT | ALLEGRO_KEYMOD_ALTGR)))
       || ((scancode == DIK_ESCAPE) && (key_modifiers & (ALLEGRO_KEYMOD_CTRL | ALLEGRO_KEYMOD_ALT | ALLEGRO_KEYMOD_ALTGR))))
      return;

   /* alt+F4 triggers a WM_CLOSE under Windows */
   if ((scancode == DIK_F4) && (key_modifiers & (ALLEGRO_KEYMOD_ALT | ALLEGRO_KEYMOD_ALTGR))) {
      if (pressed)
         PostMessage(win_disp->window, WM_CLOSE, 0, 0);
      return;
   }

   /* if not foreground, filter out press codes and handle only release codes */
   if (!win_disp->is_in_sysmenu || !pressed) {
      /* three-finger salute for killing the program */
      if (_al_three_finger_flag && (key_modifiers & ALLEGRO_KEYMOD_CTRL) && (key_modifiers & ALLEGRO_KEYMOD_ALT)) {
         if (scancode == 0x00) {
            /* when pressing CTRL-ALT-DEL, Windows launches CTRL-ALT-EVERYTHING */
            ignore_three_finger_flag = TRUE;
         }
         else if (!ignore_three_finger_flag && (scancode == DIK_END || scancode == DIK_NUMPAD1)) {
            /* we can now safely assume the user hit CTRL-ALT-END as opposed to CTRL-ALT-DEL */
            TRACE(PREFIX_I "Terminating Allegro application by CTRL-ALT-END sequence\n");
            abort();
         }
         else if (ignore_three_finger_flag && scancode == 0xff) {
         /* Windows is finished with CTRL-ALT-EVERYTHING - lets return to normality */
            ignore_three_finger_flag = FALSE;
            key_modifiers = 0;
         }
      }

      if (pressed)
         handle_key_press(scancode);
      else
         handle_key_release(scancode);
   }
}



/* key_dinput_handle: [input thread]
 *  Handles queued keyboard input.
 *
 *  Ok, this may be a bit confusing: this function is called from the input
 *  thread (winput.c) when DirectInput notifies that there is data in the
 *  keyboard buffer to process (using key_input_event).  It then calls
 *  key_dinput_handle_scancode() for each scancode in the buffer.
 *  key_dinput_handle_scancode() calls _handle_pckey() from the pckeys.c
 *  module.  _handle_pckey() then calls the _pckey_cb_handle_key_press() or
 *  _pckey_cb_handle_key_release() functions which are defined later in this
 *  module.
 */
static void key_dinput_handle(void)
{
   static DIDEVICEOBJECTDATA scancode_buffer[DINPUT_BUFFERSIZE];
   long int waiting_scancodes;
   HRESULT hr;
   int i;

   /* the whole buffer is free */
   waiting_scancodes = DINPUT_BUFFERSIZE;

   /* fill the buffer */
   hr = IDirectInputDevice8_GetDeviceData(key_dinput_device,
                                         sizeof(DIDEVICEOBJECTDATA),
                                         scancode_buffer,
                                         (unsigned long int *)&waiting_scancodes,
                                         0);

   /* was device lost ? */
   if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST) || !win_disp) {
      /* reacquire device */
      TRACE(PREFIX_W "keyboard device not acquired or lost\n");
      /* Makes no sense to reacquire the device here becasue this usually 
       * happens when the window looses focus. */
      /*wnd_schedule_proc(key_dinput_acquire);*/
   }
   else if (FAILED(hr)) {  /* other error? */
      TRACE(PREFIX_E "unexpected error while filling keyboard scancode buffer\n");
   }
   else {
      /* normally only this case should happen */

      _al_event_source_lock(&the_keyboard.es);
      for (i = 0; i < waiting_scancodes; i++) {
         key_dinput_handle_scancode((unsigned char) scancode_buffer[i].dwOfs,
                                    scancode_buffer[i].dwData & 0x80);
      }
      _al_event_source_unlock(&the_keyboard.es);
   }
}



/* key_dinput_repeat: [input thread]
 *  Called when the autorepeat timer enters the signaled state.
 */
static void key_dinput_repeat(void)
{
   _al_event_source_lock(&the_keyboard.es);
   key_dinput_handle_scancode(key_scancode_to_repeat, TRUE);
   _al_event_source_unlock(&the_keyboard.es);
}



/* get_autorepeat_parameters: [window thread]
 *  Get the system autorepeat delay and speed. This is done just before
 *  the keyboard is acquired.
 *
 *  I couldn't find any definitive reference for what the values
 *  returned by SPI_GETKEYBOARDDELAY and SPI_GETKEYBOARDSPEED
 *  represent. --pw
 */
static void get_autorepeat_parameters(void)
{
   DWORD delay, speed;

   SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &delay, 0);
   delay = MIN(delay, 3);
   SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &speed, 0);
   speed = MIN(speed, 31);

   /* units are 100e-9 seconds; negative means relative time */
   /* 0 ==> 0.25 seconds, 3 ==> 1.0 second */
   repeat_delay.QuadPart = (LONG_LONG) -10000000 * (delay+1)/4;

   /* units are milliseconds */
   /* 0 ==> 2.5 repetitions/sec, 31 ==> 31 repetitions/sec */
   repeat_period = 1000 / ((28.5/31 * speed) + 2.5);
}



/* key_dinput_acquire: [window thread]
 *  Acquires the keyboard device. This must be called after a
 *  window switch for example if the device is in foreground
 *  cooperative level.
 */
static int key_dinput_acquire(void)
{
   HRESULT hr;

   if (key_dinput_device) {
      get_autorepeat_parameters();
 
      hr = IDirectInputDevice8_Acquire(key_dinput_device);

      if (FAILED(hr)) {
         TRACE(PREFIX_E "acquire keyboard failed: %s\n", dinput_err_str(hr));
         return -1;
      }

      /* Initialize keyboard state */
      SetEvent(key_input_event);

      return 0;
   }
   else
      return -1;
}



/* key_dinput_unacquire: [window thread]
 *  Unacquires the keyboard device.
 */
void _al_win_key_dinput_unacquire(void *unused)
{
   int key;

   (void)unused;

   if (key_dinput_device && win_disp) {
      IDirectInputDevice8_Unacquire(key_dinput_device);

      /* release all keys */
      for (key=0; key<256; key++)
         if (key != DIK_PAUSE)
            key_dinput_handle_scancode((unsigned char) key, FALSE);
   }
}


void _al_win_key_dinput_grab(void *param)
{
   HRESULT hr;

   if (!key_dinput_device)
      return;

   /* Release the input from the previous window just in case,
      otherwise set cooperative level will fail. */
   if (win_disp)
      _al_win_wnd_call_proc(win_disp->window, _al_win_key_dinput_unacquire, NULL);

   win_disp = param;

   /* Set cooperative level */
   hr = IDirectInputDevice8_SetCooperativeLevel(key_dinput_device,
            win_disp->window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
   if (FAILED(hr)) {
      TRACE(PREFIX_E "IDirectInputDevice8_SetCooperativeLevel failed.\n");
      return;
   }

   key_dinput_acquire();
}



/* key_dinput_exit: [primary thread]
 *  Shuts down the DirectInput keyboard device.
 */
static int key_dinput_exit(void)
{
   if (key_dinput_device) {
      /* unregister event handlers first */
      _al_win_input_unregister_event(key_input_event);
      _al_win_input_unregister_event(key_autorepeat_timer);

      /* now it can be released */
      IDirectInputDevice8_Release(key_dinput_device);
      key_dinput_device = NULL;
   }

   /* release DirectInput interface */
   if (key_dinput) {
      IDirectInput8_Release(key_dinput);
      key_dinput = NULL;
   }

   /* close event handles */
   if (key_input_event) {
      CloseHandle(key_input_event);
      key_input_event = NULL;
   }

   if (key_autorepeat_timer) {
      CloseHandle(key_autorepeat_timer);
      key_autorepeat_timer = NULL;
   }

   return 0;
}



/* key_dinput_init: [primary thread]
 *  Sets up the DirectInput keyboard device.
 */
static int key_dinput_init(void)
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
      DINPUT_BUFFERSIZE,         // dwData
   };
   MAKE_UNION(&key_dinput, LPDIRECTINPUT8 *);

   /* Get DirectInput interface */
   hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, u.v, NULL);
   if (FAILED(hr)) {
      TRACE(PREFIX_E "DirectInputCreate failed.\n");
      goto Error;
   }

   /* Create the keyboard device */
   hr = IDirectInput8_CreateDevice(key_dinput, &GUID_SysKeyboard, &key_dinput_device, NULL);
   if (FAILED(hr)) {
      TRACE(PREFIX_E "IDirectInput8_CreateDevice failed.\n");
      goto Error;
   }

   /* Set data format */
   hr = IDirectInputDevice8_SetDataFormat(key_dinput_device, &c_dfDIKeyboard);
   if (FAILED(hr)) {
      TRACE(PREFIX_E "IDirectInputDevice8_SetDataFormat failed.\n");
      goto Error;
   }

   /* Set buffer size */
   hr = IDirectInputDevice8_SetProperty(key_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr)) {
      TRACE(PREFIX_E "IDirectInputDevice8_SetProperty failed.\n");
      goto Error;
   }

   /* Enable event notification */
   key_input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   hr = IDirectInputDevice8_SetEventNotification(key_dinput_device, key_input_event);
   if (FAILED(hr)) {
      TRACE(PREFIX_E "IDirectInputDevice8_SetEventNotification failed.\n");
      goto Error;
   }

   /* Set up the timer for autorepeat emulation */
   key_autorepeat_timer = CreateWaitableTimer(NULL, FALSE, NULL);
   if (!key_autorepeat_timer) {
      TRACE(PREFIX_E "CreateWaitableTimer failed.\n");
      goto Error;
   }

   /* Register event handlers */
   if (!_al_win_input_register_event(key_input_event, key_dinput_handle) ||
       !_al_win_input_register_event(key_autorepeat_timer, key_dinput_repeat)) {
      TRACE(PREFIX_E "Registering dinput keyboard event handlers failed.\n");
      goto Error;
   }

   return 0;

 Error:
   key_dinput_exit();
   return -1;
}



/*----------------------------------------------------------------------*/

/* forward declarations */
static bool wkeybd_init_keyboard(void);
static void wkeybd_exit_keyboard(void);
static ALLEGRO_KEYBOARD *wkeybd_get_keyboard(void);
static void wkeybd_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state);



/* the driver vtable */
#define KEYBOARD_DIRECTX        AL_ID('D','X',' ',' ')

static ALLEGRO_KEYBOARD_DRIVER keyboard_directx =
{
   KEYBOARD_DIRECTX,
   0,
   0,
   "DirectInput keyboard",
   wkeybd_init_keyboard,
   wkeybd_exit_keyboard,
   wkeybd_get_keyboard,
   NULL, /* bool set_leds(int leds) */
   NULL, /* const char *keycode_to_name(int keycode) */
   wkeybd_get_keyboard_state
};



/* list the available drivers */
_DRIVER_INFO _al_keyboard_driver_list[] =
{
   {  KEYBOARD_DIRECTX, &keyboard_directx, TRUE  },
   {  0,                NULL,              0     }
};



/* wkeybd_init_keyboard: [primary thread]
 *  Initialise the keyboard driver.
 */
static bool wkeybd_init_keyboard(void)
{
   ALLEGRO_SYSTEM *system;
   size_t i;

   if (key_dinput_init() != 0) {
      TRACE(PREFIX_E "Keyboard init failed.\n");
      return false;
   }

   system = al_system_driver();
   for (i = 0; i < _al_vector_size(&system->displays); i++) {
      ALLEGRO_DISPLAY_WIN **pwin_disp = _al_vector_ref(&system->displays, i);
      ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
      if (win_disp->window == GetForegroundWindow()) {
         _al_win_wnd_call_proc(win_disp->window,
            _al_win_key_dinput_grab, win_disp);
      }
   }

   memset(&the_keyboard, 0, sizeof the_keyboard);
   memset(&key_state, 0, sizeof key_state);

   /* Initialise the keyboard object for use as an event source. */
   _al_event_source_init(&the_keyboard.es);

   return true;
}



/* wkeybd_exit_keyboard: [primary thread]
 *  Shut down the keyboard driver.
 */
static void wkeybd_exit_keyboard(void)
{
   ALLEGRO_SYSTEM *system;
   size_t i;

   _al_event_source_free(&the_keyboard.es);

   key_dinput_exit();

   /* The toplevel display is assumed to have the input acquired. Release it. */
   system = al_system_driver();
   for (i = 0; i < _al_vector_size(&system->displays); i++) {
      ALLEGRO_DISPLAY_WIN **pwin_disp = _al_vector_ref(&system->displays, i);
      ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
      if (win_disp->window == GetForegroundWindow())
         _al_win_wnd_call_proc(win_disp->window,
                               _al_win_key_dinput_unacquire,
                               win_disp);
   }

   /* This may help catch bugs in the user program, since the pointer
    * we return to the user is always the same.
    */
   memset(&the_keyboard, 0, sizeof the_keyboard);
}



/* wkeybd_get_keyboard:
 *  Returns the address of a ALLEGRO_KEYBOARD structure representing the keyboard.
 */
static ALLEGRO_KEYBOARD *wkeybd_get_keyboard(void)
{
   return &the_keyboard;
}



/* wkeybd_get_keyboard_state: [primary thread]
 *  Copy the current keyboard state into RET_STATE, with any necessary locking.
 */
static void wkeybd_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.es);
   {
      *ret_state = key_state;
      ret_state->display = (ALLEGRO_DISPLAY*)win_disp;
   }
   _al_event_source_unlock(&the_keyboard.es);
}



/* handle_key_press: [input thread] 
 *  Does stuff when a key is pressed.  The keyboard event source
 *  should be locked.
 */
static void handle_key_press(unsigned char scancode)
{
   int mycode;
   int unicode;
   bool is_repeat;
   ALLEGRO_EVENT_TYPE event_type;
   ALLEGRO_EVENT event;
   UINT vkey;
   BYTE keystate[256];
   WCHAR chars[16];
   int n;

   vkey = MapVirtualKey(scancode, 1);
   GetKeyboardState(keystate);
   update_modifiers(keystate);


   /* TODO: shouldn't we base the mapping on vkey? */
   mycode = hw_to_mycode[scancode];
   if (mycode == 0)
      return;

   /* TODO: The below is probably unneeded.. we should not ignore the
    * numlock state.
    */

   /* MapVirtualKey always returns the arrow key VKEY, so adjust
      it if num lock is on */
   if (keystate[VK_NUMLOCK]) {
      switch (scancode) {
         case DIK_NUMPAD0:
            vkey = VK_NUMPAD0;
            break;
         case DIK_NUMPAD1:
            vkey = VK_NUMPAD1;
            break;
         case DIK_NUMPAD2:
            vkey = VK_NUMPAD2;
            break;
         case DIK_NUMPAD3:
            vkey = VK_NUMPAD3;
            break;
         case DIK_NUMPAD4:
            vkey = VK_NUMPAD4;
            break;
         case DIK_NUMPAD5:
            vkey = VK_NUMPAD5;
            break;
         case DIK_NUMPAD6:
            vkey = VK_NUMPAD6;
            break;
         case DIK_NUMPAD7:
            vkey = VK_NUMPAD7;
            break;
         case DIK_NUMPAD8:
            vkey = VK_NUMPAD8;
            break;
         case DIK_NUMPAD9:
            vkey = VK_NUMPAD9;
            break;
         case DIK_DECIMAL:
            vkey = VK_DECIMAL;
            break;
     }
   }

   /* what's life without a few special cases */
   switch (scancode) {
      case DIK_DIVIDE:
         vkey = VK_DIVIDE;
         break;
      case DIK_MULTIPLY:
         vkey = VK_MULTIPLY;
         break;
      case DIK_SUBTRACT:
         vkey = VK_SUBTRACT;
         break;
      case DIK_ADD:
         vkey = VK_ADD;
         break;
      case DIK_NUMPADENTER:
         vkey = VK_RETURN;
   }

   /* TODO: is there an advantage using ToUnicode? maybe it would allow
    * Chinese and so on characters? For now, always ToAscii is used. */
   //n = ToUnicode(vkey, scancode, keystate, chars, 16, 0);
   n = ToAscii(vkey, scancode, keystate, (WORD *)chars, 0);
   if (n == 1)
   {
      WCHAR wstr[2];
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)chars, n, wstr, 2);
      unicode = wstr[0];
   }
   else
   {
      unicode = 0;
   }

   /* Uppercase if shift/caps lock is on */
   if ((key_modifiers & ALLEGRO_KEYMOD_SHIFT) || (key_modifiers & ALLEGRO_KEY_CAPSLOCK)) {
      unicode = towupper(unicode);
   }

   is_repeat = _AL_KEYBOARD_STATE_KEY_DOWN(key_state, mycode);

   /* Maintain the key_down array. */
   _AL_KEYBOARD_STATE_SET_KEY_DOWN(key_state, mycode);

   /* Generate key press/repeat events if necessary. */   
   event_type = is_repeat ? ALLEGRO_EVENT_KEY_REPEAT : ALLEGRO_EVENT_KEY_DOWN;
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event.keyboard.type = event_type;
   event.keyboard.timestamp = al_current_time();
   event.keyboard.display = (void*)win_disp;
   event.keyboard.keycode = mycode;
   event.keyboard.unichar = unicode;
   event.keyboard.modifiers = key_modifiers;

   _al_event_source_emit_event(&the_keyboard.es, &event);

   /* Set up auto-repeat emulation. */
   if ((!is_repeat) && (mycode < ALLEGRO_KEY_MODIFIERS)) {
      key_scancode_to_repeat = scancode;
      SetWaitableTimer(key_autorepeat_timer, &repeat_delay, repeat_period,
                       NULL, NULL, /* completion routine and userdata */
                       FALSE);     /* don't wake suspended machine */
   }
}



/* handle_key_release: [input thread]
 *  Does stuff when a key is released.  The keyboard event source
 *  should be locked.
 */
static void handle_key_release(unsigned char scancode)
{
   int mycode;
   ALLEGRO_EVENT event;

   mycode = hw_to_mycode[scancode];
   if (mycode == 0)
      return;

   {
      BYTE keystate[256];
      GetKeyboardState(keystate);
      update_modifiers(keystate);
   }

   if (!_AL_KEYBOARD_STATE_KEY_DOWN(key_state, mycode))
      return;

   /* Maintain the key_down array. */
   _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(key_state, mycode);

   /* Stop autorepeating if the key to autorepeat was just released. */
   if (scancode == key_scancode_to_repeat) {
      CancelWaitableTimer(key_autorepeat_timer);
      key_scancode_to_repeat = 0;
   }

   /* Generate key release events if necessary. */
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event.keyboard.type = ALLEGRO_EVENT_KEY_UP;
   event.keyboard.timestamp = al_current_time();
   event.keyboard.display = (void*)win_disp;
   event.keyboard.keycode = mycode;
   event.keyboard.unichar = 0;
   event.keyboard.modifiers = 0;

   _al_event_source_emit_event(&the_keyboard.es, &event);
}

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
