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
 *      Omar Cornut fixed it to handle a weird peculiarity of
 *      the DirectInput joystick API.
 *
 *      Modified extensively for the new joystick API by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


/*
 * Driver operation:
 *
 * 1. When the driver is initialised all the joysticks on the system
 * are enumerated.  For each joystick, an ALLEGRO_JOYSTICK_DIRECTX structure
 * is created.  A win32 Event is also created for each joystick and DirectInput
 * is told to set that event whenever the joystick state changes. For some
 * devices this is not possible -- they must be polled.  In that case, a
 * Waitable Timer object is used instead of a win32 Event.  Once all the
 * joysticks are set up, a dedicated background thread is started.
 *
 * 2. When al_get_joystick() is called the address of one of the
 * ALLEGRO_JOYSTICK_DIRECTX structures is returned to the user.
 *
 * 3. The background thread waits upon the win32 Events/Waitable Timer
 * objects.  When one of them is triggered, the thread wakes up and
 * reads in buffered joystick events.  An internal ALLEGRO_JOYSTICK_STATE
 * structure (part of ALLEGRO_JOYSTICK_DIRECTX) is updated accordingly.
 * Also, any Allegro events are generated if necessary.
 *
 * 4. When the user calls al_get_joystick_state() the contents of the
 * internal ALLEGRO_JOYSTICK_STATE structure are copied to a user
 * ALLEGRO_JOYSTICK_STATE structure.
 *
 * 5. Every second or so, all the joysticks on the system are enumerated again.
 * We compare the GUIDs of the enumerated joysticks with the existing
 * ALLEGRO_JOYSTICK structures to tell if any new joysticks have been connected,
 * or old ones disconnected.
 */


#define ALLEGRO_NO_COMPATIBILITY

#define DIRECTINPUT_VERSION 0x0800

/* For waitable timers */
#define _WIN32_WINNT 0x0501

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_bitmap.h"


#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#ifdef ALLEGRO_MINGW32
   #undef MAKEFOURCC
#endif

#include <stdio.h>
#include <mmsystem.h>
#include <process.h>
#include <dinput.h>

/* We need XInput detection if we actually compile the XInput driver in.
*/
#ifdef ALLEGRO_CFG_XINPUT
   /* Windows XP is required. If you still use older Windows,
      XInput won't work anyway. */
   #undef WIN32_LEAN_AND_MEAN
   #include <windows.h>
   #include <winuser.h>
   #define ALLEGRO_DINPUT_FILTER_XINPUT
#endif

ALLEGRO_DEBUG_CHANNEL("dinput")

#include "allegro5/joystick.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_wjoydxnu.h"
#include "allegro5/internal/aintern_wunicode.h"



/* forward declarations */
static bool joydx_init_joystick(void);
static void joydx_exit_joystick(void);
static bool joydx_reconfigure_joysticks(void);
static int joydx_get_num_joysticks(void);
static ALLEGRO_JOYSTICK *joydx_get_joystick(int num);
static void joydx_release_joystick(ALLEGRO_JOYSTICK *joy);
static void joydx_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state);
static const char *joydx_get_name(ALLEGRO_JOYSTICK *joy);
static bool joydx_get_active(ALLEGRO_JOYSTICK *joy);

static void joydx_inactivate_joy(ALLEGRO_JOYSTICK_DIRECTX *joy);

static unsigned __stdcall joydx_thread_proc(LPVOID unused);
static void update_joystick(ALLEGRO_JOYSTICK_DIRECTX *joy);
static void handle_axis_event(ALLEGRO_JOYSTICK_DIRECTX *joy, const AXIS_MAPPING *axis_mapping, DWORD value);
static void handle_pov_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int stick, DWORD value);
static void handle_button_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int button, bool down);
static void generate_axis_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int stick, int axis, float pos);
static void generate_button_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int button, ALLEGRO_EVENT_TYPE event_type);



/* the driver vtable */
ALLEGRO_JOYSTICK_DRIVER _al_joydrv_directx =
{
   AL_JOY_TYPE_DIRECTX,
   "",
   "",
   "DirectInput joystick",
   joydx_init_joystick,
   joydx_exit_joystick,
   joydx_reconfigure_joysticks,
   joydx_get_num_joysticks,
   joydx_get_joystick,
   joydx_release_joystick,
   joydx_get_joystick_state,
   joydx_get_name,
   joydx_get_active
};


/* GUID values are borrowed from Wine */
#define DEFINE_PRIVATE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
   static const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_PRIVATE_GUID(__al_GUID_XAxis, 0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_YAxis, 0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_ZAxis, 0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_RxAxis,0xA36D02F4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_RyAxis,0xA36D02F5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_RzAxis,0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_Slider,0xA36D02E4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_Button,0xA36D02F0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_PRIVATE_GUID(__al_GUID_POV,   0xA36D02F2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_PRIVATE_GUID(__al_IID_IDirectInput8W,      0xBF798031,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
DEFINE_PRIVATE_GUID(__al_IID_IDirectInput8A,      0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
DEFINE_PRIVATE_GUID(__al_IID_IDirectInputDevice8A,0x54D41080,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);
DEFINE_PRIVATE_GUID(__al_IID_IDirectInputDevice8W,0x54D41081,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);

#ifdef UNICODE
#define __al_IID_IDirectInput8 __al_IID_IDirectInput8W
#define __al_IID_IDirectInputDevice8 __al_IID_IDirectInputDevice8W
#else
#define __al_IID_IDirectInput __al_IID_IDirectInput8A
#define __al_IID_IDirectInputDevice __al_IID_IDirectInputDevice8A
#endif

/* definition of DirectInput Joystick was borrowed from Wine implementation */
#define DIDFT_AXIS              0x00000003
#define DIDFT_ANYINSTANCE       0x00FFFF00
#define DIDFT_OPTIONAL          0x80000000

static const DIOBJECTDATAFORMAT __al_dfDIJoystick[] = {
   { &__al_GUID_XAxis, DIJOFS_X, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_YAxis, DIJOFS_Y, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_ZAxis, DIJOFS_Z, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_RxAxis, DIJOFS_RX, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_RyAxis, DIJOFS_RY, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_RzAxis, DIJOFS_RZ, DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_Slider, DIJOFS_SLIDER(0), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_Slider, DIJOFS_SLIDER(1), DIDFT_OPTIONAL | DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_POV, DIJOFS_POV(0), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_POV, DIJOFS_POV(1), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_POV, DIJOFS_POV(2), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0},
   { &__al_GUID_POV, DIJOFS_POV(3), DIDFT_OPTIONAL | DIDFT_POV | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(0), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(1), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(2), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(3), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(4), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(5), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(6), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(7), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(8), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(9), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(10), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(11), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(12), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(13), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(14), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(15), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(16), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(17), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(18), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(19), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(20), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(21), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(22), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(23), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(24), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(25), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(26), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(27), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(28), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(29), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(30), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
   { NULL, DIJOFS_BUTTON(31), DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
};

static const DIDATAFORMAT __al_c_dfDIJoystick = {
   sizeof(DIDATAFORMAT),
   sizeof(DIOBJECTDATAFORMAT),
   DIDF_ABSAXIS,
   sizeof(DIJOYSTATE),
   sizeof(__al_dfDIJoystick) / sizeof(*__al_dfDIJoystick),
   (LPDIOBJECTDATAFORMAT)__al_dfDIJoystick
};
/* end of Wine code */


/* DirectInput creation prototype */
typedef HRESULT (WINAPI *DIRECTINPUT8CREATEPROC)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);

/* DirectInput module name */
static const char* _al_dinput_module_name = "dinput8.dll";

/* DirecInput module handle */
static HMODULE _al_dinput_module = NULL;

/* DirectInput creation procedure */
static DIRECTINPUT8CREATEPROC _al_dinput_create = (DIRECTINPUT8CREATEPROC)NULL;

/* a handle to the DirectInput interface */
static LPDIRECTINPUT joystick_dinput = NULL;

/* last display which acquired the devices */
static ALLEGRO_DISPLAY_WIN *win_disp;

/* number of joysticks visible to user */
static int joydx_num_joysticks;

/* the joystick structures */
static ALLEGRO_JOYSTICK_DIRECTX joydx_joystick[MAX_JOYSTICKS];

/* for the background thread */
static HANDLE joydx_thread = NULL;
static CRITICAL_SECTION joydx_thread_cs;

/* whether the DirectInput devices need to be enumerated again */
static bool need_device_enumeration = false;

/* whether the user should call al_reconfigure_joysticks */
static bool config_needs_merging = false;

/* An array of objects that are to wake the background thread when
 * something interesting happens.  The first handle is for thread
 * termination.  The rest point to joydx_joystick[i].waker_event values,
 * for each active joystick.  joydx_joystick[i].waker_event is NOT
 * necessarily at JOYSTICK_WAKER(i).
 */
static HANDLE joydx_thread_wakers[1+MAX_JOYSTICKS];
#define STOP_EVENT        (joydx_thread_wakers[0])
#define JOYSTICK_WAKER(n) (joydx_thread_wakers[1+(n)])


/* names for things in case DirectInput doesn't provide them */
static char default_name_x[] = "X";
static char default_name_y[] = "Y";
static char default_name_z[] = "Z";
static char default_name_rx[] = "RX";
static char default_name_ry[] = "RY";
static char default_name_rz[] = "RZ";
static char default_name_stick[] = "stick";
static char default_name_slider[] = "slider";
static char default_name_hat[] = "hat";
static const char *default_name_button[MAX_BUTTONS] = {
   "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8",
   "B9", "B10", "B11", "B12", "B13", "B14", "B15", "B16",
   "B17", "B18", "B19", "B20", "B21", "B22", "B23", "B24",
   "B25", "B26", "B27", "B28", "B29", "B30", "B31", "B32"
};


#define JOY_POVFORWARD_WRAP  36000


/* Returns a pointer to a static buffer, for debugging. */
static char *guid_to_string(const GUID * guid)
{
   static char buf[200];

   sprintf(buf, "%lx-%x-%x-%x%x%x%x%x%x%x%x",
      guid->Data1,
      guid->Data2,
      guid->Data3,
      guid->Data4[0],
      guid->Data4[1],
      guid->Data4[2],
      guid->Data4[3],
      guid->Data4[4],
      guid->Data4[5],
      guid->Data4[6],
      guid->Data4[7]
   );

   return buf;
}


/* Returns a pointer to a static buffer, for debugging. */
static char *joydx_guid_string(ALLEGRO_JOYSTICK_DIRECTX *joy)
{
   return guid_to_string((const GUID *)&joy->guid);
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

      case DIERR_OTHERAPPHASPRIO:
         _al_sane_strncpy(err_str, "can't acquire the device in background", sizeof(err_str));
         break;

      default:
         _al_sane_strncpy(err_str, "unknown error", sizeof(err_str));
   }

   return err_str;
}
#else
#define dinput_err_str(hr) "\0"
#endif


/* _al_win_joystick_dinput_acquire: [window thread]
 *  Acquires the joystick devices.
 */
static void joystick_dinput_acquire(void)
{
   HRESULT hr;
   int i;

   if (!joystick_dinput)
      return;

   for (i=0; i < MAX_JOYSTICKS; i++) {
      if (joydx_joystick[i].device) {
         hr = IDirectInputDevice8_Acquire(joydx_joystick[i].device);

         if (FAILED(hr))
            ALLEGRO_ERROR("acquire joystick %d failed: %s\n", i, dinput_err_str(hr));
      }
   }
}


/* _al_win_joystick_dinput_trigger_enumeration: [window thread]
 *  Let joydx_thread_proc() know to reenumerate joysticks.
 */
void _al_win_joystick_dinput_trigger_enumeration(void)
{
  if (!joystick_dinput)
    return;
  EnterCriticalSection(&joydx_thread_cs);
  need_device_enumeration = true;
  LeaveCriticalSection(&joydx_thread_cs);
}

/* _al_win_joystick_dinput_unacquire: [window thread]
 *  Unacquires the joystick devices.
 */
void _al_win_joystick_dinput_unacquire(void *unused)
{
   int i;

   (void)unused;

   if (joystick_dinput && win_disp) {
      for (i=0; i < MAX_JOYSTICKS; i++) {
         if (joydx_joystick[i].device) {
            ALLEGRO_DEBUG("Unacquiring joystick device at slot %d\n", i);
            IDirectInputDevice8_Unacquire(joydx_joystick[i].device);
         }
      }
   }
}


/* _al_win_joystick_dinput_grab: [window thread]
 * Grabs the joystick devices.
 */
void _al_win_joystick_dinput_grab(void *param)
{
   int i;

   if (!joystick_dinput)
      return;

   /* Release the input from the previous window just in case,
      otherwise set cooperative level will fail. */
   if (win_disp) {
      _al_win_wnd_call_proc(win_disp->window, _al_win_joystick_dinput_unacquire, NULL);
   }

   win_disp = (ALLEGRO_DISPLAY_WIN *)param;

   /* set cooperative level */
   for (i = 0; i < MAX_JOYSTICKS; i++) {
      HRESULT hr;

      if (!joydx_joystick[i].device)
         continue;

      hr = IDirectInputDevice8_SetCooperativeLevel(joydx_joystick[i].device, win_disp->window,
         DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
      if (FAILED(hr)) {
         ALLEGRO_ERROR("IDirectInputDevice8_SetCooperativeLevel failed.\n");
         return;
      }
   }

   joystick_dinput_acquire();
}


static ALLEGRO_JOYSTICK_DIRECTX *joydx_by_guid(const GUID guid,
   const GUID product_guid)
{
   unsigned i;

   for (i = 0; i < MAX_JOYSTICKS; i++) {
      if (
         GUID_EQUAL(joydx_joystick[i].guid, guid) &&
         GUID_EQUAL(joydx_joystick[i].product_guid, product_guid)
         /* &&
         joydx_joystick[i].config_state == STATE_ALIVE */
         )
         return &joydx_joystick[i];
   }

   return NULL;
}


static ALLEGRO_JOYSTICK_DIRECTX *joydx_allocate_structure(int *num)
{
   int i;

   for (i = 0; i < MAX_JOYSTICKS; i++) {
      if (joydx_joystick[i].config_state == STATE_UNUSED) {
         *num = i;
         return &joydx_joystick[i];
      }
   }

   *num = -1;
   return NULL;
}


/* object_enum_callback: [primary thread]
 *  Helper function to find out what objects we have on the device.
 */
static BOOL CALLBACK object_enum_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
#define GUIDTYPE_EQ(x)  GUID_EQUAL(lpddoi->guidType, x)

   CAPS_AND_NAMES *can = (CAPS_AND_NAMES *)pvRef;

   if (GUIDTYPE_EQ(__al_GUID_XAxis)) {
      can->have_x = true;
     _tcsncpy(can->name_x, lpddoi->tszName, NAME_LEN);
   }
   else if (GUIDTYPE_EQ(__al_GUID_YAxis)) {
      can->have_y = true;
      _tcsncpy(can->name_y, lpddoi->tszName, NAME_LEN);
   }
   else if (GUIDTYPE_EQ(__al_GUID_ZAxis)) {
      can->have_z = true;
      _tcsncpy(can->name_z, lpddoi->tszName, NAME_LEN);
   }
   else if (GUIDTYPE_EQ(__al_GUID_RxAxis)) {
      can->have_rx = true;
      _tcsncpy(can->name_rx, lpddoi->tszName, NAME_LEN);
   }
   else if (GUIDTYPE_EQ(__al_GUID_RyAxis)) {
      can->have_ry = true;
      _tcsncpy(can->name_ry, lpddoi->tszName, NAME_LEN);
   }
   else if (GUIDTYPE_EQ(__al_GUID_RzAxis)) {
      can->have_rz = true;
      _tcsncpy(can->name_rz, lpddoi->tszName, NAME_LEN);
   }
   else if (GUIDTYPE_EQ(__al_GUID_Slider)) {
      if (can->num_sliders < MAX_SLIDERS) {
         _tcsncpy(can->name_slider[can->num_sliders], lpddoi->tszName,
            NAME_LEN);
         can->num_sliders++;
      }
   }
   else if (GUIDTYPE_EQ(__al_GUID_POV)) {
      if (can->num_povs < MAX_POVS) {
         _tcsncpy(can->name_pov[can->num_povs], lpddoi->tszName,
            NAME_LEN);
         can->num_povs++;
      }
   }
   else if (GUIDTYPE_EQ(__al_GUID_Button)) {
      if (can->num_buttons < MAX_BUTTONS) {
         _tcsncpy(can->name_button[can->num_buttons], lpddoi->tszName,
            NAME_LEN);
         can->num_buttons++;
      }
   }

   return DIENUM_CONTINUE;

#undef GUIDTYPE_EQ
}


static char *add_string(char *buf, const TCHAR *src, int *pos, int bufsize, const char* dfl)
{
   char *dest;

   dest = buf + *pos;
   if (*pos >= bufsize - 1) {
      /* Out of space. */
      ALLEGRO_ASSERT(dest[0] == '\0');
      return dest;
   }

   if (*pos > 0) {
      /* Skip over NUL separator. */
      dest++;
      (*pos)++;
   }
   if (src) {
      dest = _twin_copy_tchar_to_utf8(dest, src, bufsize - *pos);
   } else {
      dest = _al_sane_strncpy(dest, dfl, bufsize - *pos);
   }
   ALLEGRO_ASSERT(dest != 0);
   if (dest) {
      (*pos) += strlen(dest);
      ALLEGRO_ASSERT(*pos < bufsize);
   }
   return dest;
}


/* fill_joystick_info_using_caps_and_names: [primary thread]
 *  Helper to fill in the contents of the joystick structure using the
 *  information painstakingly stored into the caps_and_names substructure.
 */
static void fill_joystick_info_using_caps_and_names(ALLEGRO_JOYSTICK_DIRECTX *joy,
   const CAPS_AND_NAMES *can)
{
   _AL_JOYSTICK_INFO *info = &joy->parent.info;
   int pos = 0;
   int i;

#define N_STICK         (info->num_sticks)
#define N_AXIS          (info->stick[N_STICK].num_axes)
#define ADD_STRING(A, dfl) \
                        (add_string(joy->all_names, (A), &pos, \
                           sizeof(joy->all_names), (dfl)))

   /* the X, Y, Z axes make up the first stick */
   if (can->have_x || can->have_y || can->have_z) {
      if (can->have_x) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = ADD_STRING(can->name_x, default_name_x);
         joy->x_mapping.stick = N_STICK;
         joy->x_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_y) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = ADD_STRING(can->name_y, default_name_y);
         joy->y_mapping.stick = N_STICK;
         joy->y_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_z) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = ADD_STRING(can->name_z, default_name_z);
         joy->z_mapping.stick = N_STICK;
         joy->z_mapping.axis = N_AXIS;
         N_AXIS++;
      }

      info->stick[N_STICK].name = ADD_STRING(NULL, default_name_stick);
      N_STICK++;
   }

   /* the Rx, Ry, Rz axes make up the next stick */
   if (can->have_rx || can->have_ry || can->have_rz) {
      if (can->have_rx) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = ADD_STRING(can->name_rx, default_name_rx);
         joy->rx_mapping.stick = N_STICK;
         joy->rx_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_ry) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = ADD_STRING(can->name_ry, default_name_ry);
         joy->ry_mapping.stick = N_STICK;
         joy->ry_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_rz) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = ADD_STRING(can->name_rz, default_name_rz);
         joy->rz_mapping.stick = N_STICK;
         joy->rz_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      info->stick[N_STICK].name = ADD_STRING(NULL, default_name_stick);
      N_STICK++;
   }

   /* sliders are assigned to one stick each */
   for (i = 0; i < can->num_sliders; i++) {
      info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
      info->stick[N_STICK].num_axes = 1;
      info->stick[N_STICK].axis[0].name = ADD_STRING(NULL, "axis");
      info->stick[N_STICK].name = ADD_STRING(can->name_slider[i], default_name_slider);
      joy->slider_mapping[i].stick = N_STICK;
      joy->slider_mapping[i].axis  = 0;
      N_STICK++;
   }

   /* POV devices are assigned to one stick each */
   for (i = 0; i < can->num_povs; i++) {
      info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL;
      info->stick[N_STICK].num_axes = 2;
      info->stick[N_STICK].axis[0].name = ADD_STRING(NULL, "left/right");
      info->stick[N_STICK].axis[1].name = ADD_STRING(NULL, "up/down");
      info->stick[N_STICK].name = ADD_STRING(can->name_pov[i], default_name_hat);
      joy->pov_mapping_stick[i] = N_STICK;
      N_STICK++;
   }

   /* buttons */
   for (i = 0; i < can->num_buttons; i++) {
      info->button[i].name = ADD_STRING(can->name_button[i], default_name_button[i]);
   }

   info->num_buttons = can->num_buttons;

   /* correct buggy MP-8866 Dual USB Joypad info received from DirectInput */
   if (strstr(joy->name, "MP-8866")) {
      /* axes were mapped weird; remap as expected */
      /* really R-stick X axis */
      joy->z_mapping.stick  = 1;
      joy->z_mapping.axis   = 0;

      /* really R-stick Y axis */
      joy->rz_mapping.stick = 1;
      joy->rz_mapping.axis  = 1;

      info->stick[0].num_axes = 2;
      info->stick[1].num_axes = 2;

      /* reuse the axis names from the first stick */
      info->stick[2].axis[0].name = info->stick[1].axis[0].name = info->stick[0].axis[0].name;
      info->stick[2].axis[1].name = info->stick[1].axis[1].name = info->stick[0].axis[1].name;

      /* first four button names contained junk; replace with valid strings */
      info->button[ 0].name = ADD_STRING(NULL, "Triangle");
      info->button[ 1].name = ADD_STRING(NULL, "Circle");
      info->button[ 2].name = ADD_STRING(NULL, "X");
      info->button[ 3].name = ADD_STRING(NULL, "Square");

      /* while we're at it, give these controls more sensible names, too */
      info->stick[0].name = ADD_STRING(NULL, "[L-stick] or D-pad");
      info->stick[1].name = ADD_STRING(NULL, "[R-stick]");
      info->stick[2].name = ADD_STRING(NULL, "[D-pad]");
   }

#undef N_AXIS
#undef N_STICK
#undef ADD_STRING
}


#ifdef ALLEGRO_DINPUT_FILTER_XINPUT

/* A better Xinput detection method inspired by from SDL.
* The method proposed by Microsoft on their web site is problematic because it
* requires non standard compiler extensions and hard to get headers.
* This method only needs Windows XP and user32.dll.
*/
static bool dinput_is_device_xinput(const GUID *guid)
{
   PRAWINPUTDEVICELIST device_list = NULL;
   UINT amount = 0;
   UINT i;
   bool result = false;
   bool found = false;

   /* Go through RAWINPUT (WinXP and later) to find HID devices. */
   if ((GetRawInputDeviceList(NULL, &amount, sizeof (RAWINPUTDEVICELIST))
      != 0)) {
      ALLEGRO_ERROR("Could not get amount of raw input devices.\n");
      return false;  /* drat... */
   }

   if (amount < 1) {
      ALLEGRO_ERROR("Could not get any of raw input devices.\n");
      return false;  /* drat again... */
   }

   device_list = (PRAWINPUTDEVICELIST)
      al_malloc(sizeof(RAWINPUTDEVICELIST) * amount);

   if (!device_list) {
      ALLEGRO_ERROR("Could allocate memory for raw input devices.\n");
      return false;  /* No luck. */
   }

   if (GetRawInputDeviceList(device_list, &amount, sizeof(RAWINPUTDEVICELIST))
       == ((UINT)-1)) {
      ALLEGRO_ERROR("Could not retrieve %d raw input devices.\n", amount);
      al_free((void *)device_list);
      return false;
   }

   for (i = 0; i < amount; i++) {
      PRAWINPUTDEVICELIST device = device_list + i;
      RID_DEVICE_INFO rdi;
      char device_name[128];
      UINT rdi_size = sizeof (rdi);
      UINT name_size = 127;
      rdi.cbSize = sizeof (rdi);

      /* Get device info. */
      if (GetRawInputDeviceInfoA(device->hDevice, RIDI_DEVICEINFO,
         &rdi, &rdi_size) == ((UINT)-1)) {
         ALLEGRO_ERROR("Could not get raw device info for list index %d.\n", i);
         continue;
      }
      /* See if vendor and product id match. */
      if (MAKELONG(rdi.hid.dwVendorId, rdi.hid.dwProductId)
         != ((LONG)guid->Data1))
         continue;
      found = true;
      /* Get device name */
      memset(device_name, 0, 128);
      if(GetRawInputDeviceInfoA(device->hDevice, RIDI_DEVICENAME,
         device_name, &name_size) == ((UINT)-1)) {
         ALLEGRO_ERROR("Could not get raw device name for list index %d.\n", i);
         continue;
      }
      /* See if there is IG_ in the name, if it is , it's an XInput device. */
      ALLEGRO_DEBUG("Checking for XInput : %s\n", device_name);
      if (strstr(device_name, "IG_") != NULL) {
         ALLEGRO_DEBUG("Device %s is an XInput device.\n", device_name);
         result = true;
         break;
      }
   }
   if (!found) {
      ALLEGRO_ERROR("Could not find device %s in the raw device list.\n",
         guid_to_string(guid));
      result = true;
      /* Ignore "mystery" devices. Testing shows that on MinGW these are never
         valid DirectInput devices. Real ones should show up in
         the raw device list. */
   }
   al_free((void *)device_list);
   return result;
}

#endif

/* joystick_enum_callback: [primary thread]
 *  Helper function to find out how many joysticks we have and set them up.
 *  At the end joydx_num_joysticks and joydx_joystick[] will be initialised.
 */
static BOOL CALLBACK joystick_enum_callback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
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
      -32767,                   // lMin
      +32767                    // lMax
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
      0,                    // dwData
   };

   DIPROPDWORD property_buffersize =
   {
      /* the header */
      {
         sizeof(DIPROPDWORD),   // diph.dwSize
         sizeof(DIPROPHEADER),  // diph.dwHeaderSize
         0,                     // diph.dwObj
         DIPH_DEVICE,           // diph.dwHow
      },

      /* the data */
      DEVICE_BUFFER_SIZE        // number of data items
   };

   LPDIRECTINPUTDEVICE _dinput_device1;
   LPDIRECTINPUTDEVICE2 dinput_device = NULL;
   HRESULT hr;
   LPVOID temp;
   CAPS_AND_NAMES caps_and_names;
   ALLEGRO_JOYSTICK_DIRECTX *joy = NULL;
   int num;

   (void)pvRef;



   /* check if the joystick already existed before
   * Aslo have to check the product GUID because devices like the Logitech
   * F710 have a backside switch that will change the product GUID,
   * but not the instance guid.
   */
   joy = joydx_by_guid(lpddi->guidInstance, lpddi->guidProduct);
   if (joy) {
      ALLEGRO_DEBUG("Device %s still exists\n", joydx_guid_string(joy));
      joy->marked = true;
      return DIENUM_CONTINUE;
   }

   /* If we are compiling the XInput driver, ignore XInput devices, those can
      go though the XInput driver, unless if the DirectInput driver
      was set explicitly in the configuration. */
   #ifdef ALLEGRO_DINPUT_FILTER_XINPUT
   if (dinput_is_device_xinput(&lpddi->guidProduct)) {
      ALLEGRO_DEBUG("Filtered out XInput device %s\n", guid_to_string(&lpddi->guidInstance));
      goto Error;
   }
   #endif

   /* create the DirectInput joystick device */
   hr = IDirectInput8_CreateDevice(joystick_dinput, lpddi->guidInstance, &_dinput_device1, NULL);
   if (FAILED(hr))
      goto Error;

   /* query the DirectInputDevice2 interface needed for the poll() method */
   hr = IDirectInputDevice8_QueryInterface(_dinput_device1, __al_IID_IDirectInputDevice8, &temp);
   IDirectInputDevice8_Release(_dinput_device1);
   if (FAILED(hr))
      goto Error;

   dinput_device = (LPDIRECTINPUTDEVICE2)temp;

   /* enumerate objects available on the device */
   memset(&caps_and_names, 0, sizeof(caps_and_names));
   hr = IDirectInputDevice8_EnumObjects(dinput_device, object_enum_callback,
      &caps_and_names, DIDFT_PSHBUTTON | DIDFT_AXIS | DIDFT_POV);
   if (FAILED(hr))
      goto Error;

   /* set data format */
   hr = IDirectInputDevice8_SetDataFormat(dinput_device, &__al_c_dfDIJoystick);
   if (FAILED(hr))
      goto Error;

   /* set the range of axes */
   hr = IDirectInputDevice8_SetProperty(dinput_device, DIPROP_RANGE, &property_range.diph);
   if (FAILED(hr))
      goto Error;

   /* set the dead zone of axes */
   hr = IDirectInputDevice8_SetProperty(dinput_device, DIPROP_DEADZONE, &property_deadzone.diph);
   if (FAILED(hr))
      goto Error;

   /* set the buffer size */
   hr = IDirectInputDevice8_SetProperty(dinput_device, DIPROP_BUFFERSIZE, &property_buffersize.diph);
   if (FAILED(hr))
      goto Error;

   /* set up the joystick structure */
   joy = joydx_allocate_structure(&num);
   if (!joy) {
      ALLEGRO_ERROR("Joystick array full\n");
      goto Error;
   }

   joy->config_state = STATE_BORN;
   joy->marked = true;
   joy->device = dinput_device;
   memcpy(&joy->guid, &lpddi->guidInstance, sizeof(GUID));
   memcpy(&joy->product_guid, &lpddi->guidProduct, sizeof(GUID));

   _twin_copy_tchar_to_utf8(joy->name, lpddi->tszInstanceName, sizeof(joy->name));

   /* fill in the joystick structure */
   fill_joystick_info_using_caps_and_names(joy, &caps_and_names);

   /* create a thread event for this joystick, unless it was already created */
   joy->waker_event = CreateEvent(NULL, false, false, NULL);

   /* tell the joystick background thread to wake up when this joystick
    * device's state changes
    */
   hr = IDirectInputDevice8_SetEventNotification(joy->device, joy->waker_event);

   if (FAILED(hr)) {
      ALLEGRO_ERROR("SetEventNotification failed for joystick %d: %s\n",
         num, dinput_err_str(hr));
      goto Error;
   }

   if (hr == DI_POLLEDDEVICE) {
      /* This joystick device must be polled -- replace the Event with
       * a Waitable Timer object.
       *
       * Theoretically all polled devices could share a single
       * waitable timer object.  But, really, how many such devices
       * are there going to be on a system?
       */

      CloseHandle(joy->waker_event);

      joy->waker_event = CreateWaitableTimer(NULL, false, NULL);
      if (joy->waker_event == NULL) {
         ALLEGRO_ERROR("CreateWaitableTimer failed for polled device.\n");
         goto Error;
      }

      {
         LARGE_INTEGER due_time = { 0 };
         due_time.QuadPart = -1; /* Start ASAP... */
         SetWaitableTimer(joy->waker_event,
                          &due_time, 15, /* ... then periodic, every 15ms*/
                          NULL, NULL, false);
      }
   }

   ALLEGRO_INFO("Joystick %d initialized, GUID: %s\n",
      num, joydx_guid_string(joy));

   config_needs_merging = true;

   return DIENUM_CONTINUE;

 Error:

   if (dinput_device)
      IDirectInputDevice8_Release(dinput_device);

   if (joy) {
      joy->device = NULL;
      joydx_inactivate_joy(joy);
   }

   return DIENUM_CONTINUE;
}


static void joydx_inactivate_joy(ALLEGRO_JOYSTICK_DIRECTX *joy)
{
   if (joy->config_state == STATE_UNUSED)
      return;
   joy->config_state = STATE_UNUSED;
   joy->marked = false;

   if (joy->device) {
      IDirectInputDevice8_SetEventNotification(joy->device, NULL);
      IDirectInputDevice8_Release(joy->device);
      joy->device = NULL;
   }

   memset(&joy->guid, 0, sizeof(GUID));

   if (joy->waker_event) {
      CloseHandle(joy->waker_event);
      joy->waker_event = NULL;
   }

   memset(&joy->parent.info, 0, sizeof(joy->parent.info));
   /* XXX the joystick name really belongs in joy->parent.info too */
   joy->name[0] = '\0';
   memset(&joy->joystate, 0, sizeof(joy->joystate));
   /* Don't forget to wipe the guids as well! */
   memset(&joy->guid, 0, sizeof(joy->guid));
   memset(&joy->product_guid, 0, sizeof(joy->product_guid));

}


static void joydx_generate_configure_event(void)
{
   ALLEGRO_EVENT event;
   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.joystick.timestamp = al_get_time();

   _al_generate_joystick_event(&event);
}


static bool joydx_scan(bool configure)
{
   HRESULT hr;
   unsigned i;

   /* Clear mark bits. */
   for (i = 0; i < MAX_JOYSTICKS; i++)
      joydx_joystick[i].marked = false;

   /* enumerate the joysticks attached to the system */
   hr = IDirectInput8_EnumDevices(joystick_dinput, DI8DEVCLASS_GAMECTRL,
      joystick_enum_callback, NULL, DIEDFL_ATTACHEDONLY);
   if (FAILED(hr)) {
      /* XXX will this ruin everything? */
      IDirectInput8_Release(joystick_dinput);
      joystick_dinput = NULL;
      return false;
   }

   /* Schedule unmarked structures to be inactivated. */
   for (i = 0; i < MAX_JOYSTICKS; i++) {
      ALLEGRO_JOYSTICK_DIRECTX *joy = &joydx_joystick[i];

      if (joy->config_state == STATE_ALIVE && !joy->marked) {
         ALLEGRO_DEBUG("Joystick %s to be inactivated\n", joydx_guid_string(joy));
         joy->config_state = STATE_DYING;
         config_needs_merging = true;
      }
   }

   if (config_needs_merging && configure)
      joydx_generate_configure_event();

   return config_needs_merging;
}


static void joydx_merge(void)
{
   unsigned i;
   HRESULT hr;

   config_needs_merging = false;
   joydx_num_joysticks = 0;

   for (i = 0; i < MAX_JOYSTICKS; i++) {
      ALLEGRO_JOYSTICK_DIRECTX *joy = &joydx_joystick[i];

      switch (joy->config_state) {
         case STATE_UNUSED:
            break;

         case STATE_BORN:
            hr = IDirectInputDevice8_Acquire(joy->device);
            if (FAILED(hr)) {
               ALLEGRO_ERROR("acquire joystick %d failed: %s\n",
                  i, dinput_err_str(hr));
            }
            joy->config_state = STATE_ALIVE;
            /* fall through */
         case STATE_ALIVE:
            JOYSTICK_WAKER(joydx_num_joysticks) = joy->waker_event;
            joydx_num_joysticks++;
            break;

         case STATE_DYING:
            joydx_inactivate_joy(joy);
            break;
      }
   }

   ALLEGRO_INFO("Merged, num joysticks=%d\n", joydx_num_joysticks);

   joystick_dinput_acquire();
}


/* joydx_init_joystick: [primary thread]
 *
 *  Initialises the DirectInput joystick devices.
 *
 *  To avoid enumerating the the joysticks over and over, this does
 *  the enumeration once and does almost all the setting up required
 *  of the devices. joydx_get_joystick() is left with very little work
 *  to do.
 */
static bool joydx_init_joystick(void)
{
   HRESULT hr;
   ALLEGRO_SYSTEM *system;
   size_t i;

   MAKE_UNION(&joystick_dinput, LPDIRECTINPUT *);

   ALLEGRO_ASSERT(!joystick_dinput);
   ALLEGRO_ASSERT(!joydx_num_joysticks);
   ALLEGRO_ASSERT(!joydx_thread);
   ALLEGRO_ASSERT(!STOP_EVENT);

   /* load DirectInput module */
   _al_dinput_module = _al_win_safe_load_library(_al_dinput_module_name);
   if (_al_dinput_module == NULL) {
      ALLEGRO_ERROR("Failed to open '%s' library\n", _al_dinput_module_name);
      joystick_dinput = NULL;
      return false;
   }

   /* import DirectInput create proc */
   _al_dinput_create = (DIRECTINPUT8CREATEPROC)GetProcAddress(_al_dinput_module, "DirectInput8Create");
   if (_al_dinput_create == NULL) {
      ALLEGRO_ERROR("DirectInput8Create not in %s\n", _al_dinput_module_name);
      FreeLibrary(_al_dinput_module);
      joystick_dinput = NULL;
      return false;
   }

   /* get the DirectInput interface */
   hr = _al_dinput_create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, __al_IID_IDirectInput8, u.v, NULL);
   if (FAILED(hr)) {
      ALLEGRO_ERROR("Failed to create DirectInput interface\n");
      FreeLibrary(_al_dinput_module);
      joystick_dinput = NULL;
      return false;
   }

   /* initialise the lock for the background thread */
   InitializeCriticalSection(&joydx_thread_cs);

   // This initializes present joystick state
   joydx_scan(false);

   // This copies present state to user state
   joydx_merge();

   /* create the dedicated thread stopping event */
   STOP_EVENT = CreateEvent(NULL, false, false, NULL);

   /* If one of our windows is the foreground window make it grab the input. */
   system = al_get_system_driver();
   for (i = 0; i < _al_vector_size(&system->displays); i++) {
      ALLEGRO_DISPLAY_WIN **pwin_disp = (ALLEGRO_DISPLAY_WIN **)_al_vector_ref(&system->displays, i);
      ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
      if (win_disp->window == GetForegroundWindow()) {
         _al_win_wnd_call_proc(win_disp->window,
            _al_win_joystick_dinput_grab, win_disp);
      }
   }

   /* start the background thread */
   joydx_thread = (HANDLE) _beginthreadex(NULL, 0, joydx_thread_proc, NULL, 0, NULL);

   return true;
}



/* joydx_exit_joystick: [primary thread]
 *  Shuts down the DirectInput joystick devices.
 */
static void joydx_exit_joystick(void)
{
   int i;
   ALLEGRO_SYSTEM *system;
   size_t j;

   ALLEGRO_DEBUG("Entering joydx_exit_joystick\n");

   ALLEGRO_ASSERT(joydx_thread);

   /* stop the thread */
   SetEvent(STOP_EVENT);
   WaitForSingleObject(joydx_thread, INFINITE);
   CloseHandle(joydx_thread);
   joydx_thread = NULL;

   /* free thread resources */
   CloseHandle(STOP_EVENT);
   STOP_EVENT = NULL;
   DeleteCriticalSection(&joydx_thread_cs);

   /* The toplevel display is assumed to have the input acquired. Release it. */
   system = al_get_system_driver();
   for (j = 0; j < _al_vector_size(&system->displays); j++) {
      ALLEGRO_DISPLAY_WIN **pwin_disp = (ALLEGRO_DISPLAY_WIN **)_al_vector_ref(&system->displays, j);
      ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
      if (win_disp->window == GetForegroundWindow()) {
         ALLEGRO_DEBUG("Requesting window unacquire joystick devices\n");
         _al_win_wnd_call_proc(win_disp->window,
                               _al_win_joystick_dinput_unacquire,
                               win_disp);
      }
   }

   /* destroy the devices */
   for (i = 0; i < MAX_JOYSTICKS; i++) {
      joydx_inactivate_joy(&joydx_joystick[i]);
   }
   joydx_num_joysticks = 0;

   for (i = 0; i < MAX_JOYSTICKS; i++) {
      JOYSTICK_WAKER(i) = NULL;
   }

   /* destroy the DirectInput interface */
   IDirectInput8_Release(joystick_dinput);
   joystick_dinput = NULL;

   /* release module handle */
   FreeLibrary(_al_dinput_module);
   _al_dinput_module = NULL;

   ALLEGRO_DEBUG("Leaving joydx_exit_joystick\n");
}



/* joydx_reconfigure_joysticks: [primary thread]
 */
static bool joydx_reconfigure_joysticks(void)
{
   bool ret = false;

   EnterCriticalSection(&joydx_thread_cs);

   if (config_needs_merging) {
      joydx_merge();
      ret = true;
   }

   LeaveCriticalSection(&joydx_thread_cs);

   return ret;
}



/* joydx_get_num_joysticks: [primary thread]
 *  Return the number of joysticks available on the system.
 */
static int joydx_get_num_joysticks(void)
{
   return joydx_num_joysticks;
}



/* joydx_get_joystick: [primary thread]
 *
 *  Returns the address of a ALLEGRO_JOYSTICK structure for the device
 *  number NUM.
 */
static ALLEGRO_JOYSTICK *joydx_get_joystick(int num)
{
   ALLEGRO_JOYSTICK *ret = NULL;
   unsigned i;
   ALLEGRO_ASSERT(num >= 0);

   EnterCriticalSection(&joydx_thread_cs);

   for (i = 0; i < MAX_JOYSTICKS; i++) {
      ALLEGRO_JOYSTICK_DIRECTX *joy = &joydx_joystick[i];

      if (ACTIVE_STATE(joy->config_state)) {
         if (num == 0) {
            ret = (ALLEGRO_JOYSTICK *)joy;
            break;
         }
         num--;
      }
   }
   /* Must set the driver of the joystick for the wrapper driver */
   ret->driver = &_al_joydrv_directx;

   LeaveCriticalSection(&joydx_thread_cs);

#if 0
   /* is this needed? */
   if (ret) {
      ALLEGRO_DISPLAY *display = al_get_current_display();
      if (display)
         _al_win_joystick_dinput_grab(display);
   }
#endif

   return ret;
}



/* joydx_release_joystick: [primary thread]
 *  Releases a previously gotten joystick.
 */
static void joydx_release_joystick(ALLEGRO_JOYSTICK *joy)
{
   (void)joy;
}


/* joydx_get_joystick_state: [primary thread]
 *  Copy the internal joystick state to a user-provided structure.
 */
static void joydx_get_joystick_state(ALLEGRO_JOYSTICK *joy_, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ALLEGRO_JOYSTICK_DIRECTX *joy = (ALLEGRO_JOYSTICK_DIRECTX *)joy_;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   _al_event_source_lock(es);
   {
      *ret_state = joy->joystate;
   }
   _al_event_source_unlock(es);
}


static const char *joydx_get_name(ALLEGRO_JOYSTICK *joy_)
{
   ALLEGRO_JOYSTICK_DIRECTX *joy = (ALLEGRO_JOYSTICK_DIRECTX *)joy_;
   return joy->name;
}


static bool joydx_get_active(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_JOYSTICK_DIRECTX *joydx = (ALLEGRO_JOYSTICK_DIRECTX *)joy;

   return ACTIVE_STATE(joydx->config_state);
}


/* joydx_thread_proc: [joystick thread]
 *  Thread loop function for the joystick thread.
 */
static unsigned __stdcall joydx_thread_proc(LPVOID unused)
{
   double last_update = al_get_time();

   /* XXX is this needed? */
   _al_win_thread_init();

   while (true) {
      DWORD result;
      result = WaitForMultipleObjects(joydx_num_joysticks + 1, /* +1 for STOP_EVENT */
                                      joydx_thread_wakers,
                                      false,       /* wait for any */
                                      1000);   /* 1 second wait */

      if (result == WAIT_OBJECT_0)
         break; /* STOP_EVENT */

      EnterCriticalSection(&joydx_thread_cs);
      {
         /* handle hot plugging */
         if (al_get_time() > last_update+1 || result == WAIT_TIMEOUT) {
            if (need_device_enumeration) {
              joydx_scan(true);
              need_device_enumeration = false;
            }
            last_update = al_get_time();
         }

         if (result != WAIT_TIMEOUT) {
            int waker_num = result - WAIT_OBJECT_0 - 1; /* -1 for STOP_EVENT */
            HANDLE waker = JOYSTICK_WAKER(waker_num);
            unsigned i;

            for (i = 0; i < MAX_JOYSTICKS; i++) {
               if (waker == joydx_joystick[i].waker_event) {
                  update_joystick(&joydx_joystick[i]);
                  break;
               }
            }

            if (i == MAX_JOYSTICKS) {
               ALLEGRO_WARN("unable to match event to joystick\n");
            }
         }
      }
      LeaveCriticalSection(&joydx_thread_cs);
   }

   _al_win_thread_exit();

   (void)unused;
   return 0;
}



/* update_joystick: [joystick thread]
 *  Reads in data for a single DirectInput joystick device, updates
 *  the internal ALLEGRO_JOYSTICK_STATE structure, and generates any Allegro
 *  events required.
 */
static void update_joystick(ALLEGRO_JOYSTICK_DIRECTX *joy)
{
   DIDEVICEOBJECTDATA buffer[DEVICE_BUFFER_SIZE];
   DWORD num_items = DEVICE_BUFFER_SIZE;
   HRESULT hr;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   /* some devices require polling */
   IDirectInputDevice8_Poll(joy->device);

   /* get device data into buffer */
   hr = IDirectInputDevice8_GetDeviceData(joy->device, sizeof(DIDEVICEOBJECTDATA), buffer, &num_items, 0);

   if (hr != DI_OK && hr != DI_BUFFEROVERFLOW) {
      if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
         ALLEGRO_WARN("joystick device not acquired or lost\n");
      }
      else {
         ALLEGRO_ERROR("unexpected error while polling the joystick\n");
      }
      return;
   }

   /* don't bother locking the event source if there's no work to do */
   /* this happens a lot for polled devices */
   if (num_items == 0)
      return;

   _al_event_source_lock(es);
   {
      unsigned int i;

      for (i = 0; i < num_items; i++) {
         const DIDEVICEOBJECTDATA *item = &buffer[i];
         const int dwOfs    = item->dwOfs;
         const DWORD dwData = item->dwData;

         if (dwOfs == DIJOFS_X)
            handle_axis_event(joy, &joy->x_mapping, dwData);
         else if (dwOfs == DIJOFS_Y)
            handle_axis_event(joy, &joy->y_mapping, dwData);
         else if (dwOfs == DIJOFS_Z)
            handle_axis_event(joy, &joy->z_mapping, dwData);
         else if (dwOfs == DIJOFS_RX)
            handle_axis_event(joy, &joy->rx_mapping, dwData);
         else if (dwOfs == DIJOFS_RY)
            handle_axis_event(joy, &joy->ry_mapping, dwData);
         else if (dwOfs == DIJOFS_RZ)
            handle_axis_event(joy, &joy->rz_mapping, dwData);
         else if ((unsigned int)dwOfs == DIJOFS_SLIDER(0))
            handle_axis_event(joy, &joy->slider_mapping[0], dwData);
         else if ((unsigned int)dwOfs == DIJOFS_SLIDER(1))
            handle_axis_event(joy, &joy->slider_mapping[1], dwData);
         else if ((unsigned int)dwOfs == DIJOFS_POV(0))
            handle_pov_event(joy, joy->pov_mapping_stick[0], dwData);
         else if ((unsigned int)dwOfs == DIJOFS_POV(1))
            handle_pov_event(joy, joy->pov_mapping_stick[1], dwData);
         else if ((unsigned int)dwOfs == DIJOFS_POV(2))
            handle_pov_event(joy, joy->pov_mapping_stick[2], dwData);
         else if ((unsigned int)dwOfs == DIJOFS_POV(3))
            handle_pov_event(joy, joy->pov_mapping_stick[3], dwData);
         else {
            /* buttons */
            if ((dwOfs >= DIJOFS_BUTTON0) &&
                (dwOfs <  DIJOFS_BUTTON(joy->parent.info.num_buttons)))
            {
               int num = (dwOfs - DIJOFS_BUTTON0) / (DIJOFS_BUTTON1 - DIJOFS_BUTTON0);
               handle_button_event(joy, num, (dwData & 0x80));
            }
         }
      }
   }
   _al_event_source_unlock(es);
}



/* handle_axis_event: [joystick thread]
 *  Helper function to handle a state change in a non-POV axis.
 *  The joystick must be locked BEFORE entering this function.
 */
static void handle_axis_event(ALLEGRO_JOYSTICK_DIRECTX *joy, const AXIS_MAPPING *axis_mapping, DWORD value)
{
   const int stick = axis_mapping->stick;
   const int axis  = axis_mapping->axis;
   float pos;

   if (stick < 0 || stick >= joy->parent.info.num_sticks)
      return;

   if (axis < 0 || axis >= joy->parent.info.stick[stick].num_axes)
      return;

   pos = (int)value / 32767.0;
   joy->joystate.stick[stick].axis[axis] = pos;
   generate_axis_event(joy, stick, axis, pos);
}



/* handle_pov_event: [joystick thread]
 *  Helper function to handle a state change in a POV device.
 *  The joystick must be locked BEFORE entering this function.
 */
static void handle_pov_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int stick, DWORD _value)
{
   int value = _value;
   float old_p0, old_p1;
   float p0, p1;

   if (stick < 0 || stick >= joy->parent.info.num_sticks)
      return;

   old_p0 = joy->joystate.stick[stick].axis[0];
   old_p1 = joy->joystate.stick[stick].axis[1];

   /* left */
   if ((value > JOY_POVBACKWARD) && (value < JOY_POVFORWARD_WRAP))
      joy->joystate.stick[stick].axis[0] = p0 = -1.0;
   /* right */
   else if ((value > JOY_POVFORWARD) && (value < JOY_POVBACKWARD))
      joy->joystate.stick[stick].axis[0] = p0 = +1.0;
   else
      joy->joystate.stick[stick].axis[0] = p0 = 0.0;

   /* forward */
   if (((value > JOY_POVLEFT) && (value <= JOY_POVFORWARD_WRAP)) ||
       ((value >= JOY_POVFORWARD) && (value < JOY_POVRIGHT)))
      joy->joystate.stick[stick].axis[1] = p1 = -1.0;
   /* backward */
   else if ((value > JOY_POVRIGHT) && (value < JOY_POVLEFT))
      joy->joystate.stick[stick].axis[1] = p1 = +1.0;
   else
      joy->joystate.stick[stick].axis[1] = p1 = 0.0;

   if (old_p0 != p0) {
      generate_axis_event(joy, stick, 0, p0);
   }

   if (old_p1 != p1) {
      generate_axis_event(joy, stick, 1, p1);
   }
}



/* handle_button_event: [joystick thread]
 *  Helper function to handle a state change in a button.
 *  The joystick must be locked BEFORE entering this function.
 */
static void handle_button_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int button, bool down)
{
   if (button < 0 && button >= joy->parent.info.num_buttons)
      return;

   if (down) {
      joy->joystate.button[button] = 32767;
      generate_button_event(joy, button, ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN);
   }
   else {
      joy->joystate.button[button] = 0;
      generate_button_event(joy, button, ALLEGRO_EVENT_JOYSTICK_BUTTON_UP);
   }
}



/* generate_axis_event: [joystick thread]
 *  Helper to generate an event after an axis is moved.
 *  The joystick must be locked BEFORE entering this function.
 */
static void generate_axis_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int stick, int axis, float pos)
{
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!_al_event_source_needs_to_generate_event(es))
      return;

   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
   event.joystick.timestamp = al_get_time();
   event.joystick.id = (ALLEGRO_JOYSTICK *)joy;
   event.joystick.stick = stick;
   event.joystick.axis = axis;
   event.joystick.pos = pos;
   event.joystick.button = 0;

   _al_event_source_emit_event(es, &event);
}



/* generate_button_event: [joystick thread]
 *  Helper to generate an event after a button is pressed or released.
 *  The joystick must be locked BEFORE entering this function.
 */
static void generate_button_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int button, ALLEGRO_EVENT_TYPE event_type)
{
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!_al_event_source_needs_to_generate_event(es))
      return;

   event.joystick.type = event_type;
   event.joystick.timestamp = al_get_time();
   event.joystick.id = (ALLEGRO_JOYSTICK *)joy;
   event.joystick.stick = 0;
   event.joystick.axis = 0;
   event.joystick.pos = 0.0;
   event.joystick.button = button;

   _al_event_source_emit_event(es, &event);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
