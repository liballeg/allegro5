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



/* key_dinput_handle_scancode: [input thread]
 *  Handles a single scancode.
 */
static void key_dinput_handle_scancode(unsigned char scancode, int pressed)
{
   /* ignore special Windows keys (alt+tab, alt+space, (ctrl|alt)+esc) */
   if (((scancode == DIK_TAB) && (_key_shifts & KB_ALT_FLAG))
       || ((scancode == DIK_SPACE) && (_key_shifts & KB_ALT_FLAG))
       || ((scancode == DIK_ESCAPE) && (_key_shifts & (KB_CTRL_FLAG | KB_ALT_FLAG))))
      return;

   /* alt+F4 triggers a WM_CLOSE under Windows */
   if ((scancode == DIK_F4) && (_key_shifts & KB_ALT_FLAG)) {
      if (pressed)
         PostMessage(allegro_wnd, WM_CLOSE, 0, 0);
      return;
   }

   /* if not foreground, filter out press codes and handle only release codes */
   if (!wnd_sysmenu || !pressed) {

      /* three-finger salute for killing the program */
      if (((scancode == DIK_END) || (scancode == DIK_NUMPAD1))
          && ((_key_shifts & KB_CTRL_FLAG) && (_key_shifts & KB_ALT_FLAG))
          && three_finger_flag) {
         _TRACE("Terminating application\n");
         ExitProcess(0);  /* unsafe */
      }

      /* emulate PAUSE scancode sequence */
      if (scancode == DIK_PAUSE) {
         int i;
         _handle_pckey(0xE1);
         for (i=0; i < 5; i++)
            _handle_pckey(0);
         return;
      }

      /* dirty hack to let Allegro for Windows use the DOS/Linux way of handling CapsLock */
      if (((scancode == DIK_CAPITAL) || (scancode == DIK_LSHIFT) || (
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
   if (input_register_event(key_input_event, key_dinput_handle) != 0)
      goto Error;

   /* Acquire the device */
   wnd_call_proc(key_dinput_acquire);

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

