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
 *      Windows DirectInput joystick driver.
 *
 *      By Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTINPUT_VERSION 0x0500

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
   #include <dinput.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


static int joystick_dinput_init(void);
static void joystick_dinput_exit(void);
static int joystick_dinput_poll(void);


JOYSTICK_DRIVER joystick_directx =
{
   JOY_TYPE_DIRECTX,
   empty_string,
   empty_string,
   "DirectInput joystick",
   joystick_dinput_init,
   joystick_dinput_exit,
   joystick_dinput_poll,
   NULL,
   NULL,
   NULL,
   NULL
};


struct DINPUT_JOYSTICK_INFO {
   WINDOWS_JOYSTICK_INFO_MEMBERS
   LPDIRECTINPUTDEVICE2 device;
};

static LPDIRECTINPUT joystick_dinput = NULL;
static struct DINPUT_JOYSTICK_INFO dinput_joystick[MAX_JOYSTICKS];
static int dinput_joy_num = 0;



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
         strcpy(err_str, "access to the device was not granted");
         break;

      case DIERR_INVALIDPARAM:
         strcpy(err_str, "the device does not have a selected data format");
         break;

      case DIERR_OTHERAPPHASPRIO:
         strcpy(err_str, "can't acquire the device in background");
         break;

      default:
         strcpy(err_str, "unknown error");
   }

   return err_str;
}
#else
#define dinput_err_str(hr) "\0"
#endif



/* joystick_dinput_acquire: [window thread]
 *  Acquires the joystick devices.
 */
int joystick_dinput_acquire(void)
{
   HRESULT hr;
   int i;

   if (joystick_dinput) {
      for (i=0; i<dinput_joy_num; i++) {
         hr = IDirectInputDevice2_Acquire(dinput_joystick[i].device);

         if (FAILED(hr))
	    _TRACE("acquire joystick %d failed: %s\n", i, dinput_err_str(hr));
      }
   }

   return 0;
}



/* joystick_dinput_unacquire: [window thread]
 *  Unacquires the joystick devices.
 */
int joystick_dinput_unacquire(void)
{
   int i;

   if (joystick_dinput) {
      for (i=0; i<dinput_joy_num; i++) {
         IDirectInputDevice2_Unacquire(dinput_joystick[i].device);
      }
   }

   return 0;
}



/* joystick_dinput_poll: [primary thread]
 *  Polls the DirectInput joystick devices.
 */
static int joystick_dinput_poll(void)
{
   int n_joy, n_axis, n_but;
   DIJOYSTATE js;
   HRESULT hr;

   for (n_joy = 0; n_joy < dinput_joy_num; n_joy++) {
      /* poll the device */
      IDirectInputDevice2_Poll(dinput_joystick[n_joy].device);

      hr = IDirectInputDevice2_GetDeviceState(dinput_joystick[n_joy].device, sizeof(DIJOYSTATE), &js);

      if (hr == DI_OK) {
	 /* axes */
	 dinput_joystick[n_joy].axis[0] = js.lX;
	 dinput_joystick[n_joy].axis[1] = js.lY;
	 n_axis = 2;

	 if (dinput_joystick[n_joy].caps & JOYCAPS_HASZ) {
            dinput_joystick[n_joy].axis[n_axis] = js.lZ;
            n_axis++;
         }

	 if (dinput_joystick[n_joy].caps & JOYCAPS_HASR) {
            dinput_joystick[n_joy].axis[n_axis] = js.lRz;
            n_axis++;
         }

	 if (dinput_joystick[n_joy].caps & JOYCAPS_HASU) {
            dinput_joystick[n_joy].axis[n_axis] = js.rglSlider[0];
            n_axis++;
         }

	 if (dinput_joystick[n_joy].caps & JOYCAPS_HASV) {
            dinput_joystick[n_joy].axis[n_axis] = js.rglSlider[1];
            n_axis++;
         }

	 /* hat */
         if (dinput_joystick[n_joy].caps & JOYCAPS_HASPOV)
            dinput_joystick[n_joy].hat = js.rgdwPOV[0];

	 /* buttons */
	 for (n_but = 0; n_but < dinput_joystick[n_joy].num_buttons; n_but++)
	    dinput_joystick[n_joy].button[n_but] = ((js.rgbButtons[n_but] & 0x80) != 0);

	 win_update_joystick_status(n_joy, (WINDOWS_JOYSTICK_INFO *)&dinput_joystick[n_joy]);
      }
      else {
         if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
	    /* reacquire device */
	    _TRACE("joystick device not acquired or lost\n");
	    wnd_schedule_proc(joystick_dinput_acquire);
         }
         else {
            _TRACE("unexpected error while polling the joystick\n");
         }
      }
   }

   return 0;
}



/* object_enum_callback:
 *  Helper function to find out how many objects we have on the device.
 */
static BOOL CALLBACK object_enum_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
   struct DINPUT_JOYSTICK_INFO *joy = (struct DINPUT_JOYSTICK_INFO *)pvRef;
   char tmp[128];

   if (memcmp(&lpddoi->guidType, &GUID_XAxis, sizeof(GUID)) == 0) {
      joy->axis_name[0] = ustrdup(uconvert_ascii(lpddoi->tszName, tmp));
      joy->num_axes++;
   }
   else if (memcmp(&lpddoi->guidType, &GUID_YAxis, sizeof(GUID)) == 0) {
      joy->axis_name[1] = ustrdup(uconvert_ascii(lpddoi->tszName, tmp));
      joy->num_axes++;
   }
   else if (memcmp(&lpddoi->guidType, &GUID_ZAxis, sizeof(GUID)) == 0) {
      joy->axis_name[2] = ustrdup(uconvert_ascii(lpddoi->tszName, tmp));
      joy->caps |= JOYCAPS_HASZ;
      joy->num_axes++;
   }
   else if (memcmp(&lpddoi->guidType, &GUID_RzAxis, sizeof(GUID)) == 0) {
      joy->axis_name[joy->num_axes] = ustrdup(uconvert_ascii(lpddoi->tszName, tmp));
      joy->caps |= JOYCAPS_HASR;
      joy->num_axes++;
   }
   else if (memcmp(&lpddoi->guidType, &GUID_Slider, sizeof(GUID)) == 0) {
      if (joy->caps & JOYCAPS_HASV) {
         /* we support at most 2 sliders */
         return DIENUM_CONTINUE;
      }
      else {
         if (joy->caps & JOYCAPS_HASU)
            joy->caps |= JOYCAPS_HASV;
         else
            joy->caps |= JOYCAPS_HASU;

         joy->axis_name[joy->num_axes] = ustrdup(uconvert_ascii(lpddoi->tszName, tmp));
         joy->num_axes++;
      }
   }
   else if (memcmp(&lpddoi->guidType, &GUID_POV, sizeof(GUID)) == 0) {
      if (joy->caps & JOYCAPS_HASPOV) {
         /* we support at most 1 point-of-view device */
         return DIENUM_CONTINUE;
      }
      else {
         joy->hat_name = ustrdup(uconvert_ascii(lpddoi->tszName, tmp));
         joy->caps |= JOYCAPS_HASPOV;
      }
   }
   else if (memcmp(&lpddoi->guidType, &GUID_Button, sizeof(GUID)) == 0) {
      if (joy->num_buttons == MAX_JOYSTICK_BUTTONS-1) {
         return DIENUM_CONTINUE;
      }
      else {
         joy->button_name[joy->num_buttons] = ustrdup(uconvert_ascii(lpddoi->tszName, tmp));
         joy->num_buttons++;
      }
   }

   return DIENUM_CONTINUE;
}



/* joystick_enum_callback:
 *  Helper function to find out how many joysticks we have and set them up.
 */
static BOOL CALLBACK joystick_enum_callback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
   LPDIRECTINPUTDEVICE _dinput_device1;
   LPDIRECTINPUTDEVICE2 dinput_device = NULL;
   HRESULT hr;

   DIPROPRANGE property_range =
   {
      /* the header */
      {
	 sizeof(DIPROPRANGE),   // diph.dwSize
	 sizeof(DIPROPHEADER),  // diph.dwHeaderSize
	 0,                     // diph.dwObj
	 DIPH_DEVICE,           // diph.dwHow
      },

      /* the data */
      0,                        // lMin
      256                       // lMax
   };

   DIPROPDWORD property_deadzone =
   {
      /* the header */
      {
	 sizeof(DIPROPDWORD),   // diph.dwSize
	 sizeof(DIPROPHEADER),  // diph.dwHeaderSize
	 0,                     // diph.dwObj
	 DIPH_DEVICE,           // diph.dwHow
      },

      /* the data */
      2000,                     // dwData
   };

   if (dinput_joy_num == MAX_JOYSTICKS-1) {
      _TRACE("The system supports more than %d joysticks\n", MAX_JOYSTICKS);
      return DIENUM_STOP;
   }

   /* create the DirectInput joystick device */
   hr = IDirectInput_CreateDevice(joystick_dinput, &lpddi->guidInstance, &_dinput_device1, NULL);
   if (FAILED(hr))
      goto Error;

   /* query the DirectInputDevice2 interface needed for the poll() method */
   hr = IDirectInputDevice_QueryInterface(_dinput_device1, &IID_IDirectInputDevice2, (LPVOID *)&dinput_device);
   IDirectInputDevice_Release(_dinput_device1);
   if (FAILED(hr))
      goto Error;

   /* set cooperative level */
   hr = IDirectInputDevice2_SetCooperativeLevel(dinput_device, allegro_wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
   if (FAILED(hr))
      goto Error;

   /* enumerate objects available on the device */
   memset(&dinput_joystick[dinput_joy_num], 0, sizeof(struct DINPUT_JOYSTICK_INFO));
   hr = IDirectInputDevice2_EnumObjects(dinput_device, object_enum_callback, &dinput_joystick[dinput_joy_num],
                                        DIDFT_PSHBUTTON | DIDFT_AXIS | DIDFT_POV);
   if (FAILED(hr))
      goto Error;

   /* set data format */
   hr = IDirectInputDevice2_SetDataFormat(dinput_device, &c_dfDIJoystick);
   if (FAILED(hr))
      goto Error;

   /* set the range of axes */
   hr = IDirectInputDevice2_SetProperty(dinput_device, DIPROP_RANGE, &property_range.diph);
   if (FAILED(hr))
      goto Error;

   /* set the dead zone of axes */
   hr = IDirectInputDevice2_SetProperty(dinput_device, DIPROP_DEADZONE, &property_deadzone.diph);
   if (FAILED(hr))
      goto Error;

   /* register this joystick */
   dinput_joystick[dinput_joy_num].device = dinput_device;
   if (win_add_joystick((WINDOWS_JOYSTICK_INFO *)&dinput_joystick[dinput_joy_num]) != 0)
      return DIENUM_STOP;

   dinput_joy_num++;

   return DIENUM_CONTINUE;

 Error:
   if (dinput_device)
      IDirectInputDevice2_Release(dinput_device);

   return DIENUM_CONTINUE;
}



/* joystick_dinput_init: [primary thread]
 *  Initialises the DirectInput joystick devices.
 */
static int joystick_dinput_init(void)
{
   HRESULT hr;

   /* the DirectInput joystick interface is not part of DirectX 3 */
   if (_dx_ver < 0x0500)
      return -1;

   /* get the DirectInput interface */
   hr = DirectInputCreate(allegro_inst, DIRECTINPUT_VERSION, &joystick_dinput, NULL);
   if (FAILED(hr))
      return -1;

   /* enumerate the joysticks attached to the system */
   hr = IDirectInput_EnumDevices(joystick_dinput, DIDEVTYPE_JOYSTICK, joystick_enum_callback, NULL, DIEDFL_ATTACHEDONLY);
   if (FAILED(hr)) {
      IDirectInput_Release(joystick_dinput);
      return -1;
   }

   /* acquire the devices */
   wnd_call_proc(joystick_dinput_acquire);

   return (dinput_joy_num == 0);
}



/* joystick_dinput_exit: [primary thread]
 *  Shuts down the DirectInput joystick devices.
 */
static void joystick_dinput_exit(void)
{
   int i, j;

   /* unacquire the devices */
   wnd_call_proc(joystick_dinput_unacquire);

   /* destroy the devices */
   for (i=0; i<dinput_joy_num; i++) {
      IDirectInputDevice2_Release(dinput_joystick[i].device);

      for (j=0; j<dinput_joystick[i].num_axes; j++) {
         if (dinput_joystick[i].axis_name[j])
            free(dinput_joystick[i].axis_name[j]);
      }

      if (dinput_joystick[i].caps & JOYCAPS_HASPOV)
         free(dinput_joystick[i].hat_name);

      for (j=0; j<dinput_joystick[i].num_buttons; j++)
         free(dinput_joystick[i].button_name[j]);
   }

   /* destroy the DirectInput interface */
   IDirectInput_Release(joystick_dinput);

   win_remove_all_joysticks();
   dinput_joy_num = 0;
}
