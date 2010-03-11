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
 * are enumerated.  For each joystick, a ALLEGRO_JOYSTICK_DIRECTX structure
 * is created and _mostly_ initialised.  An win32 Event is created
 * also created for each joystick, and DirectInput is told to set that
 * event whenever the joystick state changes.  For some devices this
 * is not possible -- they must be polled.  In that case, a Waitable
 * Timer object is used instead of a win32 Event.  Once all the
 * joysticks are set up, a dedicated background thread is started.
 *
 * 2. When al_get_joystick() is called, the remaining initialisation
 * is done on one of one of the ALLEGRO_JOYSTICK_DIRECTX structures, and
 * then the address of it is returned to the user.
 *
 * 3. The background thread waits upon the win32 Events/Waitable Timer
 * objects.  When one of them is triggered, the thread wakes up and
 * reads in buffered joystick events.  An internal ALLEGRO_JOYSTICK_STATE
 * structure (part of ALLEGRO_JOYSTICK_DIRECTX) is updated accordingly.
 * Also, any Allegro events are generated if necessary.
 *
 * 4. When the user calls al_get_joystick_state() the contents of the
 * internal ALLEGRO_JOYSTICK_STATE structure are copied to a user ALLEGRO_JOYSTICK_STATE
 * structure.
 */


#define ALLEGRO_NO_COMPATIBILITY

#define DIRECTINPUT_VERSION 0x0800

/* For waitable timers */
#define _WIN32_WINNT 0x400

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_memory.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
   #include <process.h>
   #include <dinput.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wjoy INFO: "
#define PREFIX_W                "al-wjoy WARNING: "
#define PREFIX_E                "al-wjoy ERROR: "



/* arbitrary limit to make life easier; this was the limit in Allegro 4.1.x */
#define MAX_JOYSTICKS        8

/* these limits are from DIJOYSTICK_STATE in dinput.h */
#define MAX_SLIDERS          2
#define MAX_POVS             4
#define MAX_BUTTONS          32

/* the number of joystick events that DirectInput is told to buffer */
#define DEVICE_BUFFER_SIZE   10


/* helper structure to record information through object_enum_callback */
/* all the name_* fields must be dynamically allocated or NULL */
typedef struct {
   bool have_x;      char *name_x;
   bool have_y;      char *name_y;
   bool have_z;      char *name_z;
   bool have_rx;     char *name_rx;
   bool have_ry;     char *name_ry;
   bool have_rz;     char *name_rz;
   int num_sliders;  char *name_slider[MAX_SLIDERS];
   int num_povs;     char *name_pov[MAX_POVS];
   int num_buttons;  char *name_button[MAX_BUTTONS];
} CAPS_AND_NAMES;


/* map a DirectInput axis to an Allegro (stick,axis) pair */
typedef struct {
   int stick, axis;
} AXIS_MAPPING;


typedef struct ALLEGRO_JOYSTICK_DIRECTX {
   ALLEGRO_JOYSTICK parent;          /* must be first */

   CAPS_AND_NAMES caps_and_names;

   bool gotten;
   ALLEGRO_JOYSTICK_STATE joystate;

   LPDIRECTINPUTDEVICE2 device;

   AXIS_MAPPING x_mapping;
   AXIS_MAPPING y_mapping;
   AXIS_MAPPING z_mapping;
   AXIS_MAPPING rx_mapping;
   AXIS_MAPPING ry_mapping;
   AXIS_MAPPING rz_mapping;
   AXIS_MAPPING slider_mapping[MAX_SLIDERS];
   int pov_mapping_stick[MAX_POVS];
} ALLEGRO_JOYSTICK_DIRECTX;



/* forward declarations */
static bool joydx_init_joystick(void);
static void joydx_exit_joystick(void);
static int joydx_get_num_joysticks(void);
static ALLEGRO_JOYSTICK *joydx_get_joystick(int num);
static void joydx_release_joystick(ALLEGRO_JOYSTICK *joy);
static void joydx_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state);

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
   joydx_get_num_joysticks,
   joydx_get_joystick,
   joydx_release_joystick,
   joydx_get_joystick_state
};


ALLEGRO_DISPLAY_WIN *win_disp;

/* a handle to the DirectInput interface */
static LPDIRECTINPUT joystick_dinput = NULL;

/* these are initialised by joydx_init_joystick */
static int joydx_num_joysticks = 0;
static ALLEGRO_JOYSTICK_DIRECTX joydx_joystick[MAX_JOYSTICKS];

/* for the background thread */
static HANDLE joydx_thread = NULL;
static CRITICAL_SECTION joydx_thread_cs;

/* An array of objects that are to wake the background thread when
 * something interesting happens.  Each joystick has one such object,
 * and there is an additional Event for thread termination.
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
static char *default_name_button[MAX_BUTTONS] = {
   "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8",
   "B9", "B10", "B11", "B12", "B13", "B14", "B15", "B16",
   "B17", "B18", "B19", "B20", "B21", "B22", "B23", "B24",
   "B25", "B26", "B27", "B28", "B29", "B30", "B31", "B32"
};


#define JOY_POVFORWARD_WRAP  36000



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

   if (joystick_dinput) {
      for (i=0; i<joydx_num_joysticks; i++) {
         hr = IDirectInputDevice8_Acquire(joydx_joystick[i].device);

         if (FAILED(hr))
   TRACE(PREFIX_E "acquire joystick %d failed: %s\n", i, dinput_err_str(hr));
      }
   }
}



/* _al_win_joystick_dinput_unacquire: [window thread]
 *  Unacquires the joystick devices.
 */
void _al_win_joystick_dinput_unacquire(void *unused)
{
   int i;

   (void)unused;

   if (joystick_dinput && win_disp) {
      for (i=0; i < joydx_num_joysticks; i++) {
         IDirectInputDevice8_Unacquire(joydx_joystick[i].device);
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

   win_disp = param;

   /* set cooperative level */
   for (i = 0; i < joydx_num_joysticks; i++) {
      HRESULT hr = IDirectInputDevice8_SetCooperativeLevel(
                      joydx_joystick[i].device, win_disp->window,
                      DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
      if (FAILED(hr)) {
         TRACE(PREFIX_E "IDirectInputDevice8_SetCooperativeLevel failed.\n");
         return;
      }
   }

   joystick_dinput_acquire();
}


static char *my_strdup(char const *str)
{
   char *dup = _AL_MALLOC(strlen(str) + 1);
   strcpy(dup, str);
   return dup;
}


/* object_enum_callback: [primary thread]
 *  Helper function to find out what objects we have on the device.
 */
static BOOL CALLBACK object_enum_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
#define GUIDTYPE_EQ(x) (memcmp(&lpddoi->guidType, &x, sizeof(GUID)) == 0)

   CAPS_AND_NAMES *can = pvRef;

   if (GUIDTYPE_EQ(GUID_XAxis)) {
      if (!can->have_x) {
         can->have_x = true;
         can->name_x = my_strdup(lpddoi->tszName);
      }
   }
   else if (GUIDTYPE_EQ(GUID_YAxis)) {
      if (!can->have_y) {
         can->have_y = true;
         can->name_y = my_strdup(lpddoi->tszName);
      }
   }
   else if (GUIDTYPE_EQ(GUID_ZAxis)) {
      if (!can->have_z) {
         can->have_z = true;
         can->name_z = my_strdup(lpddoi->tszName);
      }
   }
   else if (GUIDTYPE_EQ(GUID_RxAxis)) {
      if (!can->have_rx) {
         can->have_rx = true;
         can->name_rx = my_strdup(lpddoi->tszName);
      }
   }
   else if (GUIDTYPE_EQ(GUID_RyAxis)) {
      if (!can->have_ry) {
         can->have_ry = true;
         can->name_ry = my_strdup(lpddoi->tszName);
      }
   }
   else if (GUIDTYPE_EQ(GUID_RzAxis)) {
      if (!can->have_rz) {
         can->have_rz = true;
         can->name_rz = my_strdup(lpddoi->tszName);
      }
   }
   else if (GUIDTYPE_EQ(GUID_Slider)) {
      if (can->num_sliders < MAX_SLIDERS) {
         can->name_slider[can->num_sliders] = my_strdup(lpddoi->tszName);
         can->num_sliders++;
      }
   }
   else if (GUIDTYPE_EQ(GUID_POV)) {
      if (can->num_povs < MAX_POVS) {
         can->name_pov[can->num_povs] = my_strdup(lpddoi->tszName);
         can->num_povs++;
      }
   }
   else if (GUIDTYPE_EQ(GUID_Button)) {
      if (can->num_buttons < MAX_BUTTONS) {
         can->name_button[can->num_buttons] = my_strdup(lpddoi->tszName);
         can->num_buttons++;
      }
   }

   return DIENUM_CONTINUE;

#undef GUIDTYPE_EQ
}



/* fill_joystick_info: [primary thread]
 *  Helper to fill in the contents of the joystick structure using the
 *  information painstakingly stored into the caps_and_names substructure.
 */
static void fill_joystick_info_using_caps_and_names(ALLEGRO_JOYSTICK_DIRECTX *joy)
{
   _AL_JOYSTICK_INFO *info = &joy->parent.info;
   CAPS_AND_NAMES *can = &joy->caps_and_names;
   int i;

#define N_STICK   (info->num_sticks)
#define N_AXIS    (info->stick[N_STICK].num_axes)
#define OR(A, B)  ((A) ? (A) : (B))

   /* the X, Y, Z axes make up the first stick */
   if (can->have_x || can->have_y || can->have_z) {
      if (can->have_x) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = OR(can->name_x, default_name_x);
         joy->x_mapping.stick = N_STICK;
         joy->x_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_y) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = OR(can->name_y, default_name_y);
         joy->y_mapping.stick = N_STICK;
         joy->y_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_z) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = OR(can->name_z, default_name_z);
         joy->z_mapping.stick = N_STICK;
         joy->z_mapping.axis = N_AXIS;
         N_AXIS++;
      }

      info->stick[N_STICK].name = default_name_stick;
      N_STICK++;
   }

   /* the Rx, Ry, Rz axes make up the next stick */
   if (can->have_rx || can->have_ry || can->have_rz) {
      if (can->have_rx) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = OR(can->name_rx, default_name_rx);
         joy->rx_mapping.stick = N_STICK;
         joy->rx_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_ry) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = OR(can->name_ry, default_name_ry);
         joy->ry_mapping.stick = N_STICK;
         joy->ry_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      if (can->have_rz) {
         info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
         info->stick[N_STICK].axis[N_AXIS].name = OR(can->name_rz, default_name_rz);
         joy->rz_mapping.stick = N_STICK;
         joy->rz_mapping.axis  = N_AXIS;
         N_AXIS++;
      }

      info->stick[N_STICK].name = default_name_stick;
      N_STICK++;
   }

   /* sliders are assigned to one stick each */
   for (i = 0; i < can->num_sliders; i++) {
      info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL | ALLEGRO_JOYFLAG_ANALOGUE;
      info->stick[N_STICK].num_axes = 1;
      info->stick[N_STICK].axis[0].name = "";
      info->stick[N_STICK].name = OR(can->name_slider[i], default_name_slider);
      joy->slider_mapping[i].stick = N_STICK;
      joy->slider_mapping[i].axis  = 0;
      N_STICK++;
   }

   /* POV devices are assigned to one stick each */
   for (i = 0; i < can->num_povs; i++) {
      info->stick[N_STICK].flags = ALLEGRO_JOYFLAG_DIGITAL;
      info->stick[N_STICK].num_axes = 2;
      info->stick[N_STICK].axis[0].name = "left/right";
      info->stick[N_STICK].axis[1].name = "up/down";
      info->stick[N_STICK].name = OR(can->name_pov[i], default_name_hat);
      joy->pov_mapping_stick[i] = N_STICK;
      N_STICK++;
   }

#undef N_AXIS
#undef N_STICK
#undef MAYBE_NAME

   /* buttons */
   for (i = 0; i < can->num_buttons; i++) {
      info->button[i].name = OR(can->name_button[i], default_name_button[i]);
   }

   info->num_buttons = can->num_buttons;
}



/* joystick_enum_callback: [primary thread]
 *  Helper function to find out how many joysticks we have and set them up.
 *  At the end joydx_num_joysticks and joydx_joystick[] will be initialised.
 */
static BOOL CALLBACK joystick_enum_callback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
   LPDIRECTINPUTDEVICE _dinput_device1;
   LPDIRECTINPUTDEVICE2 dinput_device = NULL;
   HRESULT hr;
   LPVOID temp;

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
      2000,                     // dwData
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

   (void)pvRef;

   ASSERT(joydx_num_joysticks >= 0 && joydx_num_joysticks < MAX_JOYSTICKS-1);
   ASSERT(!JOYSTICK_WAKER(joydx_num_joysticks));

   memset(&joydx_joystick[joydx_num_joysticks], 0, sizeof(joydx_joystick[joydx_num_joysticks]));

   /* create the DirectInput joystick device */
   hr = IDirectInput8_CreateDevice(joystick_dinput, &lpddi->guidInstance, &_dinput_device1, NULL);
   if (FAILED(hr))
      goto Error;

   /* query the DirectInputDevice2 interface needed for the poll() method */
   hr = IDirectInputDevice8_QueryInterface(_dinput_device1, &IID_IDirectInputDevice8, &temp);
   IDirectInputDevice8_Release(_dinput_device1);
   if (FAILED(hr))
      goto Error;

   dinput_device = temp;

   /* enumerate objects available on the device */
   hr = IDirectInputDevice8_EnumObjects(dinput_device, object_enum_callback, 
                                        &joydx_joystick[joydx_num_joysticks].caps_and_names,
                                        DIDFT_PSHBUTTON | DIDFT_AXIS | DIDFT_POV);
   if (FAILED(hr))
      goto Error;

   /* set data format */
   hr = IDirectInputDevice8_SetDataFormat(dinput_device, &c_dfDIJoystick);
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

   /* fill in the joystick structure */
   fill_joystick_info_using_caps_and_names(&joydx_joystick[joydx_num_joysticks]);
   joydx_joystick[joydx_num_joysticks].parent.num = joydx_num_joysticks;
   joydx_joystick[joydx_num_joysticks].device = dinput_device;
   joydx_joystick[joydx_num_joysticks].gotten = false;

   /* create a thread event for this joystick */
   JOYSTICK_WAKER(joydx_num_joysticks) = CreateEvent(NULL, false, false, NULL);

   /* tell the joystick background thread to wake up when this joystick
    * device's state changes
    */
   hr = IDirectInputDevice8_SetEventNotification(joydx_joystick[joydx_num_joysticks].device, 
                                                 JOYSTICK_WAKER(joydx_num_joysticks));

   if (FAILED(hr)) {
      TRACE(PREFIX_E "SetEventNotification failed for joystick %d: %s\n", joydx_num_joysticks, dinput_err_str(hr));
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

      CloseHandle(JOYSTICK_WAKER(joydx_num_joysticks));

      JOYSTICK_WAKER(joydx_num_joysticks) = CreateWaitableTimer(NULL, false, NULL);
      if (JOYSTICK_WAKER(joydx_num_joysticks) == NULL) {
         TRACE(PREFIX_E "CreateWaitableTimer failed in wjoydxnu.c\n");
         goto Error;
      }

      { 
         LARGE_INTEGER due_time;
         due_time.HighPart = 0;
         due_time.LowPart = 150; /* 15 ms (arbitrary) */
         SetWaitableTimer(JOYSTICK_WAKER(joydx_num_joysticks), 
                          &due_time, true, /* periodic */
                          NULL, NULL, false);
      }
   }

   joydx_num_joysticks++;

   return DIENUM_CONTINUE;

 Error:

   if (JOYSTICK_WAKER(joydx_num_joysticks)) {
      CloseHandle(JOYSTICK_WAKER(joydx_num_joysticks));
      JOYSTICK_WAKER(joydx_num_joysticks) = NULL;
   }

   if (dinput_device)
      IDirectInputDevice8_Release(dinput_device);

   return DIENUM_CONTINUE;
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

   /* make sure all the constants add up */
   /* the first two sticks are (x,y,z) and (rx,ry,rz) */
   ASSERT(_AL_MAX_JOYSTICK_STICKS >= (2 + MAX_SLIDERS + MAX_POVS));
   ASSERT(_AL_MAX_JOYSTICK_BUTTONS >= MAX_BUTTONS);

   ASSERT(!joystick_dinput);
   ASSERT(!joydx_num_joysticks);
   ASSERT(!joydx_thread);
   ASSERT(!STOP_EVENT);

   /* the DirectInput joystick interface is not part of DirectX 3 */
   /*
   if (_dx_ver < 0x0500)
      return false;
      */

   /* get the DirectInput interface */
   hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, u.v, NULL);
   if (FAILED(hr)) {
      joystick_dinput = NULL;
      return false;
   }

   /* enumerate the joysticks attached to the system */
   hr = IDirectInput8_EnumDevices(joystick_dinput, DI8DEVCLASS_GAMECTRL, joystick_enum_callback, NULL, DIEDFL_ATTACHEDONLY);
   if (FAILED(hr) || (joydx_num_joysticks == 0)) {
      IDirectInput8_Release(joystick_dinput);
      joystick_dinput = NULL;
      return false;
   }

   /* If one of our windows is the foreground window make it grab the input. */
   system = al_get_system_driver();
   for (i = 0; i < _al_vector_size(&system->displays); i++) {
      ALLEGRO_DISPLAY_WIN **pwin_disp = _al_vector_ref(&system->displays, i);
      ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
      if (win_disp->window == GetForegroundWindow()) {
         _al_win_wnd_call_proc(win_disp->window,
            _al_win_joystick_dinput_grab, win_disp);
      }
   }

   /* create the dedicated thread stopping event */
   STOP_EVENT = CreateEvent(NULL, false, false, NULL);

   /* initialise the lock for the background thread */
   InitializeCriticalSection(&joydx_thread_cs);

   /* start the background thread */
   joydx_thread = (HANDLE) _beginthreadex(NULL, 0, joydx_thread_proc, NULL, 0, NULL);

   return true;
}



/* free_caps_and_names_strings: [primary thread]
 *  Free the dynamically allocated strings in a CAPS_AND_NAMES
 *  structure.  Incidentally, this is the only reason why the
 *  caps_and_names field is kept around in ALLEGRO_JOYSTICK_DIRECTX.
 */
static void free_caps_and_names_strings(CAPS_AND_NAMES *can)
{
#define FREE(x)         \
   do {                 \
      if (x) {          \
         _AL_FREE(x);   \
         x = NULL;      \
      }                 \
   } while(0)           \

   int k;
 
   FREE(can->name_x);
   FREE(can->name_y);
   FREE(can->name_z);
   FREE(can->name_rx);
   FREE(can->name_ry);
   FREE(can->name_rz);

   for (k=0; k<MAX_SLIDERS; k++) {
      FREE(can->name_slider[k]);
   }

   for (k=0; k<MAX_POVS; k++) {
      FREE(can->name_pov[k]);
   }

   for (k=0; k<MAX_BUTTONS; k++) {
      FREE(can->name_button[k]);
   }

#undef FREE
}



/* joydx_exit_joystick: [primary thread]
 *  Shuts down the DirectInput joystick devices.
 */
static void joydx_exit_joystick(void)
{
   int i;
   ALLEGRO_SYSTEM *system;
   size_t j;

   ASSERT(joydx_thread);

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
      ALLEGRO_DISPLAY_WIN **pwin_disp = _al_vector_ref(&system->displays, j);
      ALLEGRO_DISPLAY_WIN *win_disp = *pwin_disp;
      if (win_disp->window == GetForegroundWindow())
         _al_win_wnd_call_proc(win_disp->window,
                               _al_win_joystick_dinput_unacquire,
                               win_disp);
   }

   /* destroy the devices */
   for (i = 0; i < joydx_num_joysticks; i++) {
      ASSERT(!joydx_joystick[i].gotten);

      IDirectInputDevice8_SetEventNotification(joydx_joystick[i].device, NULL);
      IDirectInputDevice8_Release(joydx_joystick[i].device);

      free_caps_and_names_strings(&joydx_joystick[i].caps_and_names);

      CloseHandle(JOYSTICK_WAKER(i));
      JOYSTICK_WAKER(i) = NULL;
   }

   /* destroy the DirectInput interface */
   IDirectInput8_Release(joystick_dinput);
   joystick_dinput = NULL;

   joydx_num_joysticks = 0;
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
 *  number NUM.  The top-level joystick functions will not call this
 *  function if joystick number NUM was already gotten.
 *
 *  Note: event source initialisation is delayed until now to get the
 *  right semantics, i.e. when you first 'get' a joystick it is not
 *  registered to any event queues.
 */
static ALLEGRO_JOYSTICK *joydx_get_joystick(int num)
{
   ALLEGRO_JOYSTICK_DIRECTX *joy = &joydx_joystick[num];

   ASSERT(!joy->gotten);

   EnterCriticalSection(&joydx_thread_cs);
   {
      _al_event_source_init(&joy->parent.es);
      joy->gotten = true;
   }
   LeaveCriticalSection(&joydx_thread_cs);

   return (ALLEGRO_JOYSTICK *)joy;
}



/* joydx_release_joystick: [primary thread]
 *  Releases a previously gotten joystick.
 */
static void joydx_release_joystick(ALLEGRO_JOYSTICK *joy_)
{
   ALLEGRO_JOYSTICK_DIRECTX *joy = (ALLEGRO_JOYSTICK_DIRECTX *)joy_;

   ASSERT(joy->gotten);

   EnterCriticalSection(&joydx_thread_cs);
   {
      joy->gotten = false;
      _al_event_source_free(&joy->parent.es);
   }
   LeaveCriticalSection(&joydx_thread_cs);
}



/* joydx_get_joystick_state: [primary thread]
 *  Copy the internal joystick state to a user-provided structure.
 */
static void joydx_get_joystick_state(ALLEGRO_JOYSTICK *joy_, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ALLEGRO_JOYSTICK_DIRECTX *joy = (ALLEGRO_JOYSTICK_DIRECTX *)joy_;

   _al_event_source_lock(&joy->parent.es);
   {
      *ret_state = joy->joystate;
   }
   _al_event_source_unlock(&joy->parent.es);
}



/* joydx_thread_proc: [joystick thread]
 *  Thread loop function for the joystick thread.
 */
static unsigned __stdcall joydx_thread_proc(LPVOID unused)
{
   /* XXX is this needed? */
   _al_win_thread_init();

   while (true) {
      DWORD result;
      int num;

      result = WaitForMultipleObjects(joydx_num_joysticks+1, /* +1 for STOP_EVENT */
                                      joydx_thread_wakers,
                                      false,       /* wait for any */
                                      INFINITE);   /* indefinite wait */

      if (result == WAIT_OBJECT_0)
         break;  /* thread suicide */

      num = result - WAIT_OBJECT_0 - 1; /* -1 for STOP_EVENT */
      if (num < 0 || num >= joydx_num_joysticks)
         continue;

      EnterCriticalSection(&joydx_thread_cs);
      {
         if (joydx_joystick[num].gotten)
            update_joystick(&joydx_joystick[num]);
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



   /* some devices require polling */
   IDirectInputDevice8_Poll(joy->device);

   /* get device data into buffer */
   hr = IDirectInputDevice8_GetDeviceData(joy->device, sizeof(DIDEVICEOBJECTDATA), buffer, &num_items, 0);

   if (hr != DI_OK && hr != DI_BUFFEROVERFLOW) {
      if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
         TRACE(PREFIX_W "joystick device not acquired or lost\n");
      }
      else {
         TRACE(PREFIX_E "unexpected error while polling the joystick\n");
      }
      return;
   }

   /* don't bother locking the event source if there's no work to do */
   /* this happens a lot for polled devices */
   if (num_items == 0)
      return;

   _al_event_source_lock(&joy->parent.es);
   {
      unsigned int i;

      for (i = 0; i < num_items; i++) {
         const DIDEVICEOBJECTDATA *item = &buffer[i];
         const int dwOfs    = item->dwOfs;
         const DWORD dwData = item->dwData;

         switch (dwOfs) {
         case DIJOFS_X:         handle_axis_event(joy, &joy->x_mapping, dwData); break;
         case DIJOFS_Y:         handle_axis_event(joy, &joy->y_mapping, dwData); break;
         case DIJOFS_Z:         handle_axis_event(joy, &joy->z_mapping, dwData); break;
         case DIJOFS_RX:        handle_axis_event(joy, &joy->rx_mapping, dwData); break;
         case DIJOFS_RY:        handle_axis_event(joy, &joy->ry_mapping, dwData); break;
         case DIJOFS_RZ:        handle_axis_event(joy, &joy->rz_mapping, dwData); break;
         case DIJOFS_SLIDER(0): handle_axis_event(joy, &joy->slider_mapping[0], dwData); break;
         case DIJOFS_SLIDER(1): handle_axis_event(joy, &joy->slider_mapping[1], dwData); break;
         case DIJOFS_POV(0):    handle_pov_event(joy, joy->pov_mapping_stick[0], dwData); break;
         case DIJOFS_POV(1):    handle_pov_event(joy, joy->pov_mapping_stick[1], dwData); break;
         case DIJOFS_POV(2):    handle_pov_event(joy, joy->pov_mapping_stick[2], dwData); break;
         case DIJOFS_POV(3):    handle_pov_event(joy, joy->pov_mapping_stick[3], dwData); break;
         default:
            /* buttons */
            if ((dwOfs >= DIJOFS_BUTTON0) &&
                (dwOfs <  DIJOFS_BUTTON(joy->parent.info.num_buttons)))
            {
               int num = (dwOfs - DIJOFS_BUTTON0) / (DIJOFS_BUTTON1 - DIJOFS_BUTTON0);
               handle_button_event(joy, num, (dwData & 0x80));
            }
            break;
         }
      }
   }
   _al_event_source_unlock(&joy->parent.es);
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

   if (old_p0 != p0)
      generate_axis_event(joy, stick, 0, p0);

   if (old_p1 != p1)
      generate_axis_event(joy, stick, 1, p1);
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

   if (!_al_event_source_needs_to_generate_event(&joy->parent.es))
      return;

   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
   event.joystick.timestamp = al_current_time();
   event.joystick.stick = stick;
   event.joystick.axis = axis;
   event.joystick.pos = pos;
   event.joystick.button = 0;

   _al_event_source_emit_event(&joy->parent.es, &event);
}



/* generate_button_event: [joystick thread]
 *  Helper to generate an event after a button is pressed or released.
 *  The joystick must be locked BEFORE entering this function.
 */
static void generate_button_event(ALLEGRO_JOYSTICK_DIRECTX *joy, int button, ALLEGRO_EVENT_TYPE event_type)
{
   ALLEGRO_EVENT event;

   if (!_al_event_source_needs_to_generate_event(&joy->parent.es))
      return;

   event.joystick.type = event_type;
   event.joystick.timestamp = al_current_time();
   event.joystick.stick = 0;
   event.joystick.axis = 0;
   event.joystick.pos = 0.0;
   event.joystick.button = button;

   _al_event_source_emit_event(&joy->parent.es, &event);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
