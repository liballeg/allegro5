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
 *      See readme.txt for copyright information.
 *
 *      TODO: The new API code was just bolted straight on top of the
 *      old driver source.  It could be neater.
 */


#define ALLEGRO_NO_COMPATIBILITY

#define DIRECTINPUT_VERSION 0x0300

/* For waitable timers */
#define _WIN32_WINNT 0x400

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_events.h"
#include "allegro/internal/aintern_keyboard.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <dinput.h>
   #include <process.h>
#endif

#define PREFIX_I                "al-wkey INFO: "
#define PREFIX_W                "al-wkey WARNING: "
#define PREFIX_E                "al-wkey ERROR: "



#define DINPUT_BUFFERSIZE 256



static LPDIRECTINPUT key_dinput;
static LPDIRECTINPUTDEVICE key_dinput_device;
static HANDLE key_input_event;
static HANDLE key_autorepeat_timer;
static LARGE_INTEGER repeat_delay;
static LONG repeat_period;
static unsigned int key_modifiers;
static int key_scancode_to_repeat;

/* the one and only keyboard object and its internal state */
static AL_KEYBOARD the_keyboard;
static AL_KBDSTATE key_state;



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

   if (keystate[VK_SHIFT] & 0x80)
      modifiers |= AL_KEYMOD_SHIFT;
   if (keystate[VK_CONTROL] & 0x80)
      modifiers |= AL_KEYMOD_CTRL;

   if (keystate[VK_LMENU] & 0x80)
      modifiers |= AL_KEYMOD_ALT;
   else if (keystate[VK_RMENU] & 0x80)
      modifiers |= AL_KEYMOD_ALTGR;
   else if (keystate[VK_MENU] & 0x80)
      modifiers |= AL_KEYMOD_ALT;

   if (keystate[VK_SCROLL] & 1)
      modifiers |= AL_KEYMOD_SCROLLLOCK;
   if (keystate[VK_NUMLOCK] & 1)
      modifiers |= AL_KEYMOD_NUMLOCK;
   if (keystate[VK_CAPITAL] & 1)
      modifiers |= AL_KEYMOD_CAPSLOCK;

   /* needed for special handling in key_dinput_handle_scancode */
   key_modifiers = modifiers;

}



/* lookup table for converting DIK_* scancodes into Allegro AL_KEY_* codes */
/* this table was from pckeys.c  */
static const unsigned char hw_to_mycode[256] =
{
   /* 0x00 */  0,		  AL_KEY_ESCAPE,     AL_KEY_1,          AL_KEY_2, 
   /* 0x04 */  AL_KEY_3,          AL_KEY_4,          AL_KEY_5,          AL_KEY_6,
   /* 0x08 */  AL_KEY_7,          AL_KEY_8,          AL_KEY_9,          AL_KEY_0, 
   /* 0x0C */  AL_KEY_MINUS,      AL_KEY_EQUALS,     AL_KEY_BACKSPACE,  AL_KEY_TAB,
   /* 0x10 */  AL_KEY_Q,          AL_KEY_W,          AL_KEY_E,          AL_KEY_R, 
   /* 0x14 */  AL_KEY_T,          AL_KEY_Y,          AL_KEY_U,          AL_KEY_I,
   /* 0x18 */  AL_KEY_O,          AL_KEY_P,          AL_KEY_OPENBRACE,  AL_KEY_CLOSEBRACE, 
   /* 0x1C */  AL_KEY_ENTER,      AL_KEY_LCTRL,	     AL_KEY_A,          AL_KEY_S,
   /* 0x20 */  AL_KEY_D,          AL_KEY_F,          AL_KEY_G,          AL_KEY_H, 
   /* 0x24 */  AL_KEY_J,          AL_KEY_K,          AL_KEY_L,          AL_KEY_SEMICOLON,
   /* 0x28 */  AL_KEY_QUOTE,      AL_KEY_TILDE,      AL_KEY_LSHIFT,     AL_KEY_BACKSLASH, 
   /* 0x2C */  AL_KEY_Z,          AL_KEY_X,          AL_KEY_C,          AL_KEY_V,
   /* 0x30 */  AL_KEY_B,          AL_KEY_N,          AL_KEY_M,          AL_KEY_COMMA, 
   /* 0x34 */  AL_KEY_FULLSTOP,   AL_KEY_SLASH,      AL_KEY_RSHIFT,     AL_KEY_PAD_ASTERISK,
   /* 0x38 */  AL_KEY_ALT,        AL_KEY_SPACE,      AL_KEY_CAPSLOCK,   AL_KEY_F1, 
   /* 0x3C */  AL_KEY_F2,         AL_KEY_F3,         AL_KEY_F4,         AL_KEY_F5,
   /* 0x40 */  AL_KEY_F6,         AL_KEY_F7,         AL_KEY_F8,         AL_KEY_F9, 
   /* 0x44 */  AL_KEY_F10,        AL_KEY_NUMLOCK,    AL_KEY_SCROLLLOCK, AL_KEY_PAD_7,
   /* 0x48 */  AL_KEY_PAD_8,      AL_KEY_PAD_9,      AL_KEY_PAD_MINUS,  AL_KEY_PAD_4, 
   /* 0x4C */  AL_KEY_PAD_5,      AL_KEY_PAD_6,      AL_KEY_PAD_PLUS,   AL_KEY_PAD_1,
   /* 0x50 */  AL_KEY_PAD_2,      AL_KEY_PAD_3,      AL_KEY_PAD_0,      AL_KEY_PAD_DELETE, 
   /* 0x54 */  AL_KEY_PRINTSCREEN,0,		     AL_KEY_BACKSLASH2, AL_KEY_F11,
   /* 0x58 */  AL_KEY_F12,        0,		     0,		        AL_KEY_LWIN, 
   /* 0x5C */  AL_KEY_RWIN,       AL_KEY_MENU,       0,                 0,
   /* 0x60 */  0,                 0,                 0,                 0, 
   /* 0x64 */  0,                 0,                 0,                 0,
   /* 0x68 */  0,                 0,                 0,                 0, 
   /* 0x6C */  0,                 0,                 0,                 0,
   /* 0x70 */  AL_KEY_KANA,       0,                 0,                 AL_KEY_ABNT_C1, 
   /* 0x74 */  0,                 0,                 0,                 0,
   /* 0x78 */  0,                 AL_KEY_CONVERT,    0,                 AL_KEY_NOCONVERT, 
   /* 0x7C */  0,                 AL_KEY_YEN,        0,                 0,

   /* 0x80 */  0,                 0,                 0,                 0,
   /* 0x84 */  0,                 0,                 0,                 0,
   /* 0x88 */  0,                 0,                 0,                 0,
   /* 0x8C */  0,                 0,                 0,                 0,
   /* 0x90 */  0,                 AL_KEY_AT,         AL_KEY_COLON2,     0,
   /* 0x94 */  AL_KEY_KANJI,      0,                 0,                 0,
   /* 0x98 */  0,                 0,                 0,                 0,
   /* 0x9C */  AL_KEY_PAD_ENTER,  AL_KEY_RCTRL,      0,                 0,
   /* 0xA0 */  0,                 0,                 0,                 0,
   /* 0xA4 */  0,                 0,                 0,                 0,
   /* 0xA8 */  0,                 0,                 0,                 0,
   /* 0xAC */  0,                 0,                 0,                 0,
   /* 0xB0 */  0,                 0,                 0,                 0,
   /* 0xB4 */  0,                 AL_KEY_PAD_SLASH,  0,                 AL_KEY_PRINTSCREEN,
   /* 0xB8 */  AL_KEY_ALTGR,      0,                 0,                 0,
   /* 0xBC */  0,                 0,                 0,                 0,
   /* 0xC0 */  0,                 0,                 0,                 0,
   /* 0xC4 */  0,                 AL_KEY_PAUSE,      0,                 AL_KEY_HOME,
   /* 0xC8 */  AL_KEY_UP,         AL_KEY_PGUP,       0,                 AL_KEY_LEFT,
   /* 0xCC */  0,                 AL_KEY_RIGHT,      0,                 AL_KEY_END,
   /* 0xD0 */  AL_KEY_DOWN,       AL_KEY_PGDN,       AL_KEY_INSERT,     AL_KEY_DELETE,
   /* 0xD4 */  0,                 0,                 0,                 0,
   /* 0xD8 */  0,                 0,                 0,                 AL_KEY_LWIN,
   /* 0xDC */  AL_KEY_RWIN,       AL_KEY_MENU,       0,                 0,
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
   HWND allegro_wnd = win_get_window();
   /* Windows seems to send lots of ctrl-alt-XXX key combos in response to the
    * ctrl-alt-del combo. We want to ignore them all, especially ctrl-alt-end,
    * which would cause Allegro to terminate.
    */
   static int ignore_three_finger_flag = FALSE;

   /* ignore special Windows keys (alt+tab, alt+space, (ctrl|alt)+esc) */
   if (((scancode == DIK_TAB) && (key_modifiers & (AL_KEYMOD_ALT | AL_KEYMOD_ALTGR)))
       || ((scancode == DIK_SPACE) && (key_modifiers & (AL_KEYMOD_ALT | AL_KEYMOD_ALTGR)))
       || ((scancode == DIK_ESCAPE) && (key_modifiers & (AL_KEYMOD_CTRL | AL_KEYMOD_ALT | AL_KEYMOD_ALTGR))))
      return;

   /* alt+F4 triggers a WM_CLOSE under Windows */
   if ((scancode == DIK_F4) && (key_modifiers & (AL_KEYMOD_ALT | AL_KEYMOD_ALTGR))) {
      if (pressed)
         PostMessage(allegro_wnd, WM_CLOSE, 0, 0);
      return;
   }

   /* if not foreground, filter out press codes and handle only release codes */
   if (!wnd_sysmenu || !pressed) {
      /* three-finger salute for killing the program */
      if (_al_three_finger_flag && (key_modifiers & AL_KEYMOD_CTRL) && (key_modifiers & AL_KEYMOD_ALT)) {
         if (scancode == 0x00) {
            /* when pressing CTRL-ALT-DEL, Windows launches CTRL-ALT-EVERYTHING */
            ignore_three_finger_flag = TRUE;
         }
         else if (!ignore_three_finger_flag && (scancode == DIK_END || scancode == DIK_NUMPAD1)) {
            /* we can now safely assume the user hit CTRL-ALT-END as opposed to CTRL-ALT-DEL */
            _TRACE(PREFIX_I "Terminating Allegro application by CTRL-ALT-END sequence\n");
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
   hr = IDirectInputDevice_GetDeviceData(key_dinput_device,
                                         sizeof(DIDEVICEOBJECTDATA),
                                         scancode_buffer,
                                         &waiting_scancodes,
                                         0);

   /* was device lost ? */
   if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
      /* reacquire device */
      _TRACE(PREFIX_W "keyboard device not acquired or lost\n");
      wnd_schedule_proc(key_dinput_acquire);
   }
   else if (FAILED(hr)) {  /* other error? */
      _TRACE(PREFIX_E "unexpected error while filling keyboard scancode buffer\n");
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
int key_dinput_acquire(void)
{
   HRESULT hr;

   if (key_dinput_device) {
      get_autorepeat_parameters();
 
      hr = IDirectInputDevice_Acquire(key_dinput_device);

      if (FAILED(hr)) {
         _TRACE(PREFIX_E "acquire keyboard failed: %s\n", dinput_err_str(hr));
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
int key_dinput_unacquire(void)
{
   int key;

   if (key_dinput_device) {
      IDirectInputDevice_Unacquire(key_dinput_device);

      /* release all keys */
      for (key=0; key<256; key++)
         if (key != DIK_PAUSE)
            key_dinput_handle_scancode((unsigned char) key, FALSE);

      return 0;
   }
   else
      return -1;
}



/* key_dinput_exit: [primary thread]
 *  Shuts down the DirectInput keyboard device.
 */
static int key_dinput_exit(void)
{
   if (key_dinput_device) {
      /* unregister event handlers first */
      _win_input_unregister_event(key_input_event);
      _win_input_unregister_event(key_autorepeat_timer);

      /* unacquire device */
      wnd_call_proc(key_dinput_unacquire);

      /* now it can be released */
      IDirectInputDevice_Release(key_dinput_device);
      key_dinput_device = NULL;
   }

   /* release DirectInput interface */
   if (key_dinput) {
      IDirectInput_Release(key_dinput);
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
   HWND allegro_wnd = win_get_window();
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

   /* Get DirectInput interface */
   hr = DirectInputCreate(allegro_inst, DIRECTINPUT_VERSION, &key_dinput, NULL);
   if (FAILED(hr)) {
      TRACE("DirectInputCreate failed.\n");
      goto Error;
   }

   /* Create the keyboard device */
   hr = IDirectInput_CreateDevice(key_dinput, &GUID_SysKeyboard, &key_dinput_device, NULL);
   if (FAILED(hr)) {
      TRACE("IDirectInput_CreateDevice failed.\n");
      goto Error;
   }

   /* Set data format */
   hr = IDirectInputDevice_SetDataFormat(key_dinput_device, &c_dfDIKeyboard);
   if (FAILED(hr)) {
      TRACE("IDirectInputDevice_SetDataFormat failed.\n");
      goto Error;
   }

   /* Set buffer size */
   hr = IDirectInputDevice_SetProperty(key_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr)) {
      TRACE("IDirectInputDevice_SetProperty failed.\n");
      goto Error;
   }

/*
   if (key_dinput_set_cooperation_level(allegro_wnd)) {
      goto Error;
   }
*/

   /* Enable event notification */
   key_input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   hr = IDirectInputDevice_SetEventNotification(key_dinput_device, key_input_event);
   if (FAILED(hr)) {
      TRACE("IDirectInputDevice_SetEventNotification failed.\n");
      goto Error;
   }

   /* Set up the timer for autorepeat emulation */
   key_autorepeat_timer = CreateWaitableTimer(NULL, FALSE, NULL);
   if (!key_autorepeat_timer) {
      TRACE("CreateWaitableTimer failed.\n");
      goto Error;
   }

   /* Register event handlers */
   if (_win_input_register_event(key_input_event, key_dinput_handle) != 0 ||
       _win_input_register_event(key_autorepeat_timer, key_dinput_repeat) != 0) {
      TRACE("Registering dinput keyboard event handlers failed.\n");
      goto Error;
   }

   /* Acquire the device */
   wnd_call_proc(key_dinput_acquire);

   return 0;

 Error:
   key_dinput_exit();
   return -1;
}



/*----------------------------------------------------------------------*/

/* forward declarations */
static bool wkeybd_init_keyboard(void);
static void wkeybd_exit_keyboard(void);
static AL_KEYBOARD *wkeybd_get_keyboard(void);
static void wkeybd_get_keyboard_state(AL_KBDSTATE *ret_state);



/* the driver vtable */
#define KEYBOARD_DIRECTX        AL_ID('D','X',' ',' ')

static AL_KEYBOARD_DRIVER keyboard_directx =
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
   if (key_dinput_init() != 0)
      return false;

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
   _al_event_source_free(&the_keyboard.es);

   key_dinput_exit();

   /* This may help catch bugs in the user program, since the pointer
    * we return to the user is always the same.
    */
   memset(&the_keyboard, 0, sizeof the_keyboard);
}



/* wkeybd_get_keyboard:
 *  Returns the address of a AL_KEYBOARD structure representing the keyboard.
 */
static AL_KEYBOARD *wkeybd_get_keyboard(void)
{
   return &the_keyboard;
}



/* wkeybd_get_keyboard_state: [primary thread]
 *  Copy the current keyboard state into RET_STATE, with any necessary locking.
 */
static void wkeybd_get_keyboard_state(AL_KBDSTATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.es);
   {
      *ret_state = key_state;
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
   AL_EVENT_TYPE event_type;
   AL_EVENT *event;
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

   is_repeat = _AL_KBDSTATE_KEY_DOWN(key_state, mycode);

   /* Maintain the key_down array. */
   _AL_KBDSTATE_SET_KEY_DOWN(key_state, mycode);

   /* Generate key press/repeat events if necessary. */   
   event_type = is_repeat ? AL_EVENT_KEY_REPEAT : AL_EVENT_KEY_DOWN;
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event = _al_event_source_get_unused_event(&the_keyboard.es);
   if (!event)
      return;

   event->keyboard.type = event_type;
   event->keyboard.timestamp = al_current_time();
   event->keyboard.__display__dont_use_yet__ = NULL;
   event->keyboard.keycode = mycode;
   event->keyboard.unichar = unicode;
   event->keyboard.modifiers = key_modifiers;

   _al_event_source_emit_event(&the_keyboard.es, event);

   /* Set up auto-repeat emulation. */
   if ((!is_repeat) && (mycode < AL_KEY_MODIFIERS)) {
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
   AL_EVENT *event;

   mycode = hw_to_mycode[scancode];
   if (mycode == 0)
      return;

   {
      BYTE keystate[256];
      GetKeyboardState(keystate);
      update_modifiers(keystate);
   }

   if (!_AL_KBDSTATE_KEY_DOWN(key_state, mycode))
      return;

   /* Maintain the key_down array. */
   _AL_KBDSTATE_CLEAR_KEY_DOWN(key_state, mycode);

   /* Stop autorepeating if the key to autorepeat was just released. */
   if (scancode == key_scancode_to_repeat) {
      CancelWaitableTimer(key_autorepeat_timer);
      key_scancode_to_repeat = 0;
   }

   /* Generate key release events if necessary. */
   if (!_al_event_source_needs_to_generate_event(&the_keyboard.es))
      return;

   event = _al_event_source_get_unused_event(&the_keyboard.es);
   if (!event)
      return;

   event->keyboard.type = AL_EVENT_KEY_UP;
   event->keyboard.timestamp = al_current_time();
   event->keyboard.__display__dont_use_yet__ = NULL;
   event->keyboard.keycode = mycode;
   event->keyboard.unichar = 0;
   event->keyboard.modifiers = 0;

   _al_event_source_emit_event(&the_keyboard.es, event);
}

int key_dinput_set_cooperative_level(HWND wnd)
{
   /* Set cooperative level */
   HRESULT hr = IDirectInputDevice_SetCooperativeLevel(key_dinput_device, wnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
   if (FAILED(hr)) {
      TRACE("IDirectInputDevice_SetCooperativeLevel failed.\n");
      return 1;
   }
   return 0;
}

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
