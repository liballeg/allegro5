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
 *      See readme.txt for copyright information.
 */


#define DIRECTINPUT_VERSION 0x0300

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <process.h>
   #include <dinput.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif


_DRIVER_INFO _keyboard_driver_list[] =
{
   {KEYBOARD_DIRECTX, &keyboard_directx, TRUE},
   {0, NULL, 0}
};


static int key_directx_init(void);
static void key_directx_exit(void);
static void key_directx_wait_for_input(void);
static void key_directx_stop_wait(void);


KEYBOARD_DRIVER keyboard_directx =
{
   KEYBOARD_DIRECTX,
   0,
   0,
   "DirectInput keyboard",
   1,
   key_directx_init,
   key_directx_exit,
   NULL, NULL, NULL,
   key_directx_wait_for_input,
   key_directx_stop_wait,
   _pckey_scancode_to_ascii
};


#define DINPUT_BUFFERSIZE 256
static HANDLE key_input_event = NULL;
static HANDLE key_input_processed_event = NULL;
static LPDIRECTINPUT key_dinput = NULL;
static LPDIRECTINPUTDEVICE key_dinput_device = NULL;



/* dinput_err_str:
 *  Returns a DirectInput error string.
 */
#ifdef DEBUGMODE
static char* dinput_err_str(long err)
{
   static char err_str[64];

   switch (err) {

      case DIERR_NOTACQUIRED:
         strcpy(err_str, "the device is not acquired");
         break;

      case DIERR_INPUTLOST:
         strcpy(err_str, "access to the device was lost");
         break;

      case DIERR_INVALIDPARAM:
         strcpy(err_str, "the device does not have a selected data format");
         break;

#ifdef DIERR_OTHERAPPHASPRIO
      /* this is not a legacy DirectX 3 error code */
      case DIERR_OTHERAPPHASPRIO:
         strcpy(err_str, "can't acquire the device in background");
         break;
#endif

      default:
         strcpy(err_str, "unknown error");
   }

   return err_str;
}
#else
#define dinput_err_str(hr) "\0"
#endif



/* key_dinput_handle_scancode: [input thread]
 *  Handles a single scancode.
 */
static void key_dinput_handle_scancode(unsigned char scancode, int pressed)
{
   /* three-finger salute for killing the program */
   if ((((scancode & 0x7f) == 0x4F) || ((scancode & 0x7f) == 0x53)) &&
       (three_finger_flag) && (_key_shifts & KB_CTRL_FLAG) && (_key_shifts & KB_ALT_FLAG)) {
      _TRACE("Terminating application\n");
      ExitProcess(0);
   }

   /* ignore special Windows keys (alt+tab, alt+space, (ctrl|alt)+esc, alt+F4) */
   if ((pressed) && 
       ((((scancode & 0x7f) == 0x0F) && (_key_shifts & KB_ALT_FLAG)) ||
       (((scancode & 0x7f) == 0x01) && (_key_shifts & (KB_CTRL_FLAG | KB_ALT_FLAG))) ||
       (((scancode & 0x7f) == 0x39) && (_key_shifts & KB_ALT_FLAG)) ||
       (((scancode & 0x7f) == 0x3E) && (_key_shifts & KB_ALT_FLAG))))
      return;

   /* if not foreground, filter out press codes and handle only release codes */
   if (!pressed || (app_foreground && !wnd_sysmenu)) {
      if (scancode > 0x7F) {
	 _handle_pckey(0xE0);
      }

      /* dirty hack to let Allegro for Windows use the DOS/Linux way of handling CapsLock */
      if (((scancode == DIK_CAPITAL) || (scancode == DIK_LSHIFT) || (scancode == DIK_RSHIFT))
                                              && pressed && (_key_shifts & KB_CAPSLOCK_FLAG)) {
         keybd_event(VK_CAPITAL, 0, 0, 0);
         keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
      }

      _handle_pckey((scancode & 0x7F) | (pressed ? 0 : 0x80));
   }
}



/* key_dinput_handle: [input thread]
 *  Handles queued keyboard input.
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
      _TRACE("keyboard device not acquired or lost\n");
      wnd_acquire_keyboard();
   }
   else if (FAILED(hr)) {  /* other error? */
      _TRACE("unexpected error while filling keyboard scancode buffer\n");
   }
   else {
      /* normally only this case should happen */
      for (i = 0; i < waiting_scancodes; i++) {
         key_dinput_handle_scancode((unsigned char) scancode_buffer[i].dwOfs,
                                    scancode_buffer[i].dwData & 0x80);
      }
   }

   SetEvent(key_input_processed_event);
}



/* key_dinput_acquire: [window thread]
 *  Acquires the keyboard device. This must be called after a
 *  window switch for example if the device is in foreground
 *  cooperative level.
 */
int key_dinput_acquire(void)
{
   HRESULT hr;
   int mask, state;
   char key_state[256];

   if (key_dinput_device) {
      mask = KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG;
      state = 0;

      /* Read the current Windows keyboard state */
      GetKeyboardState(key_state);

      if (key_state[VK_SCROLL] & 1)
         state |= KB_SCROLOCK_FLAG;

      if (key_state[VK_NUMLOCK] & 1)
         state |= KB_NUMLOCK_FLAG;

      if (key_state[VK_CAPITAL] & 1)
         state |= KB_CAPSLOCK_FLAG;

      _key_shifts = (_key_shifts & ~mask) | (state & mask);
 
      hr = IDirectInputDevice_Acquire(key_dinput_device);

      if (FAILED(hr)) {
         _TRACE("acquire keyboard failed: %s\n", dinput_err_str(hr));
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
         if ((key != 0x61) && (key != 0xE1))  /* PAUSE */
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
      /* unregister event handler first */
      wnd_unregister_event(key_input_event);

      /* unacquire device */
      wnd_unacquire_keyboard();

      /* now it can be released */
      IDirectInputDevice_Release(key_dinput_device);
      key_dinput_device = NULL;
   }

   /* release DirectInput interface */
   if (key_dinput) {
      IDirectInput_Release(key_dinput);
      key_dinput = NULL;
   }

   /* close event handle */
   if (key_input_event) {
      CloseHandle(key_input_event);
      key_input_event = NULL;
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

   /* Get DirectInput interface */
   hr = DirectInputCreate(allegro_inst, DIRECTINPUT_VERSION, &key_dinput, NULL);
   if (FAILED(hr))
      goto Error;

   /* Create the keyboard device */
   hr = IDirectInput_CreateDevice(key_dinput, &GUID_SysKeyboard, &key_dinput_device, NULL);
   if (FAILED(hr))
      goto Error;

   /* Set data format */
   hr = IDirectInputDevice_SetDataFormat(key_dinput_device, &c_dfDIKeyboard);
   if (FAILED(hr))
      goto Error;

   /* Set buffer size */
   hr = IDirectInputDevice_SetProperty(key_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr))
      goto Error;

   /* Set cooperative level */
   hr = IDirectInputDevice_SetCooperativeLevel(key_dinput_device, allegro_wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
   if (FAILED(hr))
      goto Error;

   /* Enable event notification */
   key_input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   hr = IDirectInputDevice_SetEventNotification(key_dinput_device, key_input_event);
   if (FAILED(hr))
      goto Error;

   /* Register event handler */
   if (wnd_register_event(key_input_event, key_dinput_handle) != 0)
      goto Error;

   /* Acquire the device */
   wnd_acquire_keyboard();

   return 0;

 Error:
   key_dinput_exit();
   return -1;
}



/* key_directx_init: [primary thread]
 */
static int key_directx_init(void)
{
   /* Because DirectInput uses the same scancodes as the pc keyboard
    * controller, the DirectX keyboard driver passes them into the
    * pckeys translation routines.
    */
   _pckeys_init();

   /* keyboard input is handled by the window thread */
   key_input_processed_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   if (key_dinput_init() != 0) {
      /* something has gone wrong */
      _TRACE("keyboard handler init failed\n");
      CloseHandle(key_input_processed_event);
      return -1;
   }

   /* the keyboard handler has successfully set up */
   _TRACE("keyboard handler starts\n");
   return 0;
}



/* key_directx_exit: [primary thread]
 */
static void key_directx_exit(void)
{
   if (key_dinput_device) {
      /* command keyboard handler shutdown */
      _TRACE("keyboard handler exits\n");
      key_dinput_exit();

      /* now we can free all resources */
      CloseHandle(key_input_processed_event);
   }
}



/* key_directx_wait_for_input: [primary thread]
 */
static void key_directx_wait_for_input(void)
{
   WaitForSingleObject(key_input_processed_event, INFINITE);
}



/* static void key_directx_stop_wait: [primary thread]
 */
static void key_directx_stop_wait(void)
{
   SetEvent(key_input_processed_event);
}

