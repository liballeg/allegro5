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
 *      Windows XInput joystick driver.
 *
 *      By Beoran.
 *
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_NO_COMPATIBILITY

/* For waitable timers */
#define _WIN32_WINNT 0x0501

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_bitmap.h"


#ifdef ALLEGRO_CFG_XINPUT
/* Don't compile this lot if xinput isn't supported. */

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#ifdef ALLEGRO_MINGW32
   #undef MAKEFOURCC
#endif

/* Poll connected joysticks frequently and non-connected ones infrequently. */
#ifndef ALLEGRO_XINPUT_POLL_DELAY
#define ALLEGRO_XINPUT_POLL_DELAY 0.01
#endif

#ifndef ALLEGRO_XINPUT_DISCONNECTED_POLL_DELAY
#define ALLEGRO_XINPUT_DISCONNECTED_POLL_DELAY 1.50
#endif


#include <stdio.h>
#include <allegro5/joystick.h>
#include <mmsystem.h>
#include <process.h>

/* The official DirectX xinput.h uses SAL annotations.
 * Need the sal.h header to get rid of them. On some other platforms 
 * such as MinGW on Linux this header is lacking because it is not needed there. 
 * So, simply try to include sal.h IF we have it , and if not, hope 
 * for the best. 
 * This does no harm on msys2 either, they have a sal.h header.
 */ 
#ifdef ALLEGRO_HAVE_SAL_H 
#include <sal.h> 
#endif

#include <xinput.h>

ALLEGRO_DEBUG_CHANNEL("xinput")

#include "allegro5/joystick.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_wjoyxi.h"


/* forward declarations */
static bool joyxi_init_joystick(void);
static void joyxi_exit_joystick(void);
static bool joyxi_reconfigure_joysticks(void);
static int joyxi_get_num_joysticks(void);
static ALLEGRO_JOYSTICK *joyxi_get_joystick(int num);
static void joyxi_release_joystick(ALLEGRO_JOYSTICK *joy);
static void joyxi_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state);
static const char *joyxi_get_name(ALLEGRO_JOYSTICK *joy);
static bool joyxi_get_active(ALLEGRO_JOYSTICK *joy);


/* the driver vtable */
ALLEGRO_JOYSTICK_DRIVER _al_joydrv_xinput =
{
   AL_JOY_TYPE_XINPUT,
   "",
   "",
   "XInput Joystick",
   joyxi_init_joystick,
   joyxi_exit_joystick,
   joyxi_reconfigure_joysticks,
   joyxi_get_num_joysticks,
   joyxi_get_joystick,
   joyxi_release_joystick,
   joyxi_get_joystick_state,
   joyxi_get_name,
   joyxi_get_active
};

#define XINPUT_MIN_VERSION   3
#define XINPUT_MAX_VERSION   4

typedef void (WINAPI *XInputEnablePROC)(BOOL);
typedef DWORD (WINAPI *XInputSetStatePROC)(DWORD, XINPUT_VIBRATION*);
typedef DWORD (WINAPI *XInputGetStatePROC)(DWORD, XINPUT_STATE*);
typedef DWORD (WINAPI *XInputGetCapabilitiesPROC)(DWORD, DWORD, XINPUT_CAPABILITIES*);

static HMODULE _imp_xinput_module = 0;

static XInputEnablePROC _imp_XInputEnable = NULL;
static XInputGetStatePROC _imp_XInputGetState = NULL;
static XInputGetCapabilitiesPROC _imp_XInputGetCapabilities = NULL;
XInputSetStatePROC _al_imp_XInputSetState = NULL;

/* the joystick structures */
static ALLEGRO_JOYSTICK_XINPUT joyxi_joysticks[MAX_JOYSTICKS];

/* For the background threads. */
static ALLEGRO_THREAD *joyxi_thread = NULL;
static ALLEGRO_THREAD *joyxi_disconnected_thread = NULL;
static ALLEGRO_MUTEX  *joyxi_mutex = NULL;
/* Use condition variables to put the thread to sleep and prevent too
   frequent polling*/
static ALLEGRO_COND   *joyxi_cond = NULL;
static ALLEGRO_COND   *joyxi_disconnected_cond = NULL;

/* Names for things in because XInput doesn't provide them. */

/* Names of the sticks.*/
static char *const joyxi_stick_names[MAX_STICKS] = {
   "Left Thumbstick",
   "Right Thumbstick",
   "Left Trigger",
   "Right Trigger",
};

/* Names of the axis */
static char *const joyxi_axis_names[MAX_STICKS][MAX_AXES] = {
   { "X", "Y" },
   { "X", "Y" },
   { "Ramp", "Error" },
   { "Ramp", "Error" },
};

/* Sticks per axis  */
static const int joyxi_axis_per_stick[MAX_STICKS] = {
   2, 2, 1, 1,
};


/* Struct to help mapping. */
struct _AL_XINPUT_BUTTON_MAPPING
{
   int flags;
   int button;
   const char *name;
};

/* The data in this array helps to map from XINPUT button input to
   ALLEGRO's. */
static const struct _AL_XINPUT_BUTTON_MAPPING
   joyxi_button_mapping[MAX_BUTTONS] = {
   { XINPUT_GAMEPAD_A, 0, "A" },
   { XINPUT_GAMEPAD_B, 1, "B" },
   { XINPUT_GAMEPAD_X, 2, "X" },
   { XINPUT_GAMEPAD_Y, 3, "Y" },
   { XINPUT_GAMEPAD_RIGHT_SHOULDER, 4, "RB" },
   { XINPUT_GAMEPAD_LEFT_SHOULDER, 5, "LB" },
   { XINPUT_GAMEPAD_RIGHT_THUMB, 6, "RT" },
   { XINPUT_GAMEPAD_LEFT_THUMB, 7, "LT" },
   { XINPUT_GAMEPAD_BACK, 8, "BACK" },
   { XINPUT_GAMEPAD_START, 9, "START" },
   { XINPUT_GAMEPAD_DPAD_RIGHT, 10, "RIGHT DPAD" },
   { XINPUT_GAMEPAD_DPAD_LEFT, 11, "LEFT DPAD" },
   { XINPUT_GAMEPAD_DPAD_DOWN, 12, "DOWN DPAD" },
   { XINPUT_GAMEPAD_DPAD_UP, 13, "UP DPAD" },
};

static void unload_xinput_module(void)
{
   FreeLibrary(_imp_xinput_module);
   _imp_xinput_module = NULL;
}

static bool _imp_load_xinput_module_version(int version)
{
   char module_name[16];

   sprintf(module_name, "xinput1_%d.dll", version);

   _imp_xinput_module = _al_win_safe_load_library(module_name);
   if (NULL == _imp_xinput_module)
      return false;

   _imp_XInputEnable = (XInputEnablePROC)GetProcAddress(_imp_xinput_module, "XInputEnable");
   if (NULL == _imp_XInputEnable) {
      FreeLibrary(_imp_xinput_module);
      _imp_xinput_module = NULL;
      return false;
   }
   _imp_XInputGetState = (XInputGetStatePROC)GetProcAddress(_imp_xinput_module, "XInputGetState");
   _imp_XInputGetCapabilities = (XInputGetCapabilitiesPROC)GetProcAddress(_imp_xinput_module, "XInputGetCapabilities");
   _al_imp_XInputSetState = (XInputSetStatePROC)GetProcAddress(_imp_xinput_module, "XInputSetState");

   ALLEGRO_INFO("Module \"%s\" loaded.\n", module_name);

   return true;
}

static bool load_xinput_module(void)
{
   ALLEGRO_CONFIG *cfg;
   long version;

   if (_imp_xinput_module) {
      return true;
   }

   cfg = al_get_system_config();
   if (cfg) {
      char const *value = al_get_config_value(cfg,
         "joystick", "force_xinput_version");
      if (value) {
         errno = 0;
         version = strtol(value, NULL, 10);
         if (errno) {
            ALLEGRO_ERROR("Failed to override XInput version. \"%s\" is not valid integer number.", value);
            return false;
         }
         else
            return _imp_load_xinput_module_version((int)version);
      }
   }

   // Iterate over all valid versions.
   for (version = XINPUT_MAX_VERSION; version >= XINPUT_MIN_VERSION; version--)
      if (_imp_load_xinput_module_version((int)version))
         return true;

   ALLEGRO_ERROR("Failed to load XInput library. Library is not installed.");

   return false;
}

/* generate_axis_event: [joystick thread]
 *  Helper to generate an event when reconfiguration is needed.
 *  The joystick must be locked BEFORE entering this function.
 */

static void joyxi_generate_reconfigure_event(void)
{
   ALLEGRO_EVENT_SOURCE *source = al_get_joystick_event_source();
   ALLEGRO_EVENT event;
   if (!_al_event_source_needs_to_generate_event(source))
      return;
   event.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.any.timestamp = al_get_time();
   event.any.source = source;
   _al_event_source_emit_event(source, &event);
}

/*  Helper to generate an event after an axis is moved.
 *  The joystick must be locked BEFORE entering this function.
 */
static void joyxi_generate_axis_event(ALLEGRO_JOYSTICK_XINPUT *joy, int stick, int axis, float pos)
{
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!_al_event_source_needs_to_generate_event(es))
      return;

   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
   event.joystick.timestamp = al_get_time();
   event.joystick.id = (ALLEGRO_JOYSTICK *)&joy->parent;
   event.joystick.stick = stick;
   event.joystick.axis = axis;
   event.joystick.pos = pos;
   event.joystick.button = 0;
   ALLEGRO_DEBUG("Generating an axis event on stick %d axis %d value %f:\n", stick, axis, pos);

   _al_event_source_emit_event(es, &event);
}


/* Helper to generate an event after a button is pressed or released.
 *  The joystick must be locked BEFORE entering this function.
 */
static void joyxi_generate_button_event(ALLEGRO_JOYSTICK_XINPUT *joy, int button, ALLEGRO_EVENT_TYPE event_type)
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
   ALLEGRO_DEBUG("Generating an button event on button %d type %d:\n", button, event_type);

   _al_event_source_emit_event(es, &event);
}


/* Converts an XINPUT axis value to one suitable for Allegro's axes.*/
static float joyxi_convert_axis(SHORT value)
{
   if (value >= 0) return(((float)value) / 32767.0);
   return(((float)value) / 32768.0);
}

/* Converts an XINPUT trigger value to one suitable for Allegro's axes.*/
static float joyxi_convert_trigger(BYTE value)
{
   return(((float)value) / 255.0);
}

/* Converts an XInput state to an Allegro joystick state. */
static void joyxi_convert_state(ALLEGRO_JOYSTICK_STATE *alstate, XINPUT_STATE *xistate)
{
   int index;
   /* Wipe the allegro state clean. */
   memset(alstate, 0, sizeof(*alstate));

   /* Map the buttons. Make good use of the mapping data. */
   for (index = 0; index < MAX_BUTTONS; index++) {
      const struct _AL_XINPUT_BUTTON_MAPPING *mapping = joyxi_button_mapping + index;
      if (xistate->Gamepad.wButtons & mapping->flags) {
         alstate->button[mapping->button] = 32767;
      }
      else {
         alstate->button[mapping->button] = 0;
      }
   }
   /* Map the x and y axes of both sticks. */
   alstate->stick[0].axis[0] = joyxi_convert_axis(xistate->Gamepad.sThumbLX);
   alstate->stick[0].axis[1] = -joyxi_convert_axis(xistate->Gamepad.sThumbLY);
   alstate->stick[1].axis[0] = joyxi_convert_axis(xistate->Gamepad.sThumbRX);
   alstate->stick[1].axis[1] = -joyxi_convert_axis(xistate->Gamepad.sThumbRY);
   /* Map the triggers as two individual sticks and axes each . */
   alstate->stick[2].axis[0] = joyxi_convert_trigger(xistate->Gamepad.bLeftTrigger);
   alstate->stick[3].axis[0] = joyxi_convert_trigger(xistate->Gamepad.bRightTrigger);
   return;
}


/* Emits joystick events for the difference between the new and old joystick state. */
static void joyxi_emit_events(
   ALLEGRO_JOYSTICK_XINPUT *xjoy,
   ALLEGRO_JOYSTICK_STATE *newstate, ALLEGRO_JOYSTICK_STATE *oldstate)
{
   int index, subdex;
   /* Send events for buttons. */
   for (index = 0; index < MAX_BUTTONS; index++) {
      int newbutton = newstate->button[index];
      int oldbutton = oldstate->button[index];
      if (newbutton != oldbutton) {
         int type = (oldbutton > newbutton ?
                     ALLEGRO_EVENT_JOYSTICK_BUTTON_UP :
                     ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN);
         joyxi_generate_button_event(xjoy, index, type);
      }
   }
   /* Send events for the thumb pad axes and triggers . */
   for (index = 0; index < MAX_STICKS; index++) {
      for (subdex = 0; subdex < joyxi_axis_per_stick[index]; subdex++) {
         float oldaxis = oldstate->stick[index].axis[subdex];
         float newaxis = newstate->stick[index].axis[subdex];
         if (oldaxis != newaxis) {
            joyxi_generate_axis_event(xjoy, index, subdex, newaxis);
         }
      }
   }
}


/* Polling function for a joystick that is currently active. */
static void joyxi_poll_connected_joystick(ALLEGRO_JOYSTICK_XINPUT *xjoy)
{
   XINPUT_STATE xistate;
   ALLEGRO_JOYSTICK_STATE alstate;
   DWORD res = _imp_XInputGetState(xjoy->index, &xistate);
   if (res != ERROR_SUCCESS) {
      /* Assume joystick was disconnected, need to reconfigure. */
      joyxi_generate_reconfigure_event();
      return;
   }

   /* Check if the sequence is different. If not, no state change so
      don't do anything. */
   if (xistate.dwPacketNumber == xjoy->state.dwPacketNumber)
      return;

   ALLEGRO_DEBUG("XInput joystick state change detected.\n");

   /* If we get here translate the state and send the needed events. */
   joyxi_convert_state(&alstate, &xistate);
   joyxi_emit_events(xjoy, &alstate, &xjoy->joystate);

   /* Finally copy over the states. */
   xjoy->state = xistate;
   xjoy->joystate = alstate;
}


/* Polling function for a joystick that is currently not active.  Care is taken to do this infrequently so
   performance doesn't suffer too much. */
static void joyxi_poll_disconnected_joystick(ALLEGRO_JOYSTICK_XINPUT *xjoy)
{
   XINPUT_CAPABILITIES xicapas;
   DWORD res;
   res = _imp_XInputGetCapabilities(xjoy->index, 0, &xicapas);
   if (res == ERROR_SUCCESS) {
      /* Got capabilities, joystick was connected, need to reconfigure. */
      joyxi_generate_reconfigure_event();
      return;
   }
   /* Nothing to do if we get here. */
}


/** Polls all connected joysticks. */
static void joyxi_poll_connected_joysticks(void)
{
   int index;

   for (index = 0; index < MAX_JOYSTICKS; index++) {
      if ((joyxi_joysticks + index)->active) {
         joyxi_poll_connected_joystick(joyxi_joysticks + index);
      }
   }
}

/** Polls all disconnected joysticks. */
static void joyxi_poll_disconnected_joysticks(void)
{
   int index;

   for (index = 0; index < MAX_JOYSTICKS; index++) {
      if (!((joyxi_joysticks + index)->active)) {
         joyxi_poll_disconnected_joystick(joyxi_joysticks + index);
      }
   }
}

/** Thread function that polls the active xinput joysticks. */
static void *joyxi_poll_thread(ALLEGRO_THREAD *thread, void *arg)
{
   ALLEGRO_TIMEOUT timeout;
   al_lock_mutex(joyxi_mutex);
   /* Poll once every so much time, 10ms by default. */
   while (!al_get_thread_should_stop(thread)) {
      al_init_timeout(&timeout, ALLEGRO_XINPUT_POLL_DELAY);
      /* Wait for the condition for the polling time in stead of using
         al_rest to allows the polling thread to be awoken when needed. */
      al_wait_cond_until(joyxi_cond, joyxi_mutex, &timeout);
      /* If we get here poll joystick for new input or connection
       * and dispatch events. The mutexhas always been locked
       * so this should be OK. */
      joyxi_poll_connected_joysticks();
   }
   al_unlock_mutex(joyxi_mutex);
   return arg;
}

/** Thread function that polls the disconnected joysticks. */
static void *joyxi_poll_disconnected_thread(ALLEGRO_THREAD *thread, void *arg)
{
   ALLEGRO_TIMEOUT timeout;
   al_lock_mutex(joyxi_mutex);
   /* Poll once every so much time, 10ms by default. */
   while (!al_get_thread_should_stop(thread)) {
      al_init_timeout(&timeout, ALLEGRO_XINPUT_DISCONNECTED_POLL_DELAY);
      /* Wait for the condition for the polling time in stead of using
         al_rest to allows the polling thread to be awoken when needed. */
      al_wait_cond_until(joyxi_disconnected_cond, joyxi_mutex, &timeout);
      /* If we get here poll joystick for new input or connection
       * and dispatch events. The mutex has always been locked
       * so this should be OK. */
      joyxi_poll_disconnected_joysticks();
   }
   al_unlock_mutex(joyxi_mutex);
   return arg;
}



/* Initializes the info part of the joystick. */
static void joyxi_init_joystick_info(ALLEGRO_JOYSTICK_XINPUT *xjoy)
{
   int index, subdex;
   _AL_JOYSTICK_INFO *info = &xjoy->parent.info;
   /* Map xinput to 4 sticks: 2 thumb pads and 2 triggers. */
   info->num_sticks = 4;
   /* Map xinput to 14 buttons */
   info->num_buttons = MAX_BUTTONS;
   /* Map button names. */
   for (index = 0; index < MAX_BUTTONS; index++) {
      info->button[index].name = joyxi_button_mapping[index].name;
   }
   /* Map stick and axis names. */
   for (index = 0; index < MAX_STICKS; index++) {
      info->stick[index].name = joyxi_stick_names[index];
      info->stick[index].num_axes = joyxi_axis_per_stick[index];
      info->stick[index].flags = ALLEGRO_JOYFLAG_ANALOGUE;
      for (subdex = 0; subdex < joyxi_axis_per_stick[index]; subdex++) {
         info->stick[index].axis[subdex].name = joyxi_axis_names[index][subdex];
      }
   }
}

/* Initialization API function. */
static bool joyxi_init_joystick(void)
{
   int index;

   load_xinput_module();

   /* Create the mutex and two condition variables. */
   joyxi_mutex = al_create_mutex_recursive();
   if (!joyxi_mutex)
      return false;
   joyxi_cond = al_create_cond();
   if (!joyxi_cond)
      return false;
   joyxi_disconnected_cond = al_create_cond();
   if (!joyxi_disconnected_cond)
      return false;

   al_lock_mutex(joyxi_mutex);

   /* Fill in the joystick structs */
   for (index = 0; index < MAX_JOYSTICKS; index++) {
      joyxi_joysticks[index].active = false;
      sprintf(joyxi_joysticks[index].name, "XInput Joystick %d", index);
      joyxi_joysticks[index].index = (DWORD)index;
      joyxi_init_joystick_info(joyxi_joysticks + index);
   }
   /* Now, enable XInput*/
   _imp_XInputEnable(TRUE);
   /* Now check which joysticks are enabled and poll them for the first time
    * but without sending any events.
    */
   for (index = 0; index < MAX_JOYSTICKS; index++) {
      DWORD res = _imp_XInputGetCapabilities(joyxi_joysticks[index].index, 0, &joyxi_joysticks[index].capabilities);
      joyxi_joysticks[index].active = (res == ERROR_SUCCESS);
      if (joyxi_joysticks[index].active) {
         res = _imp_XInputGetState(joyxi_joysticks[index].index, &joyxi_joysticks[index].state);
         joyxi_joysticks[index].active = (res == ERROR_SUCCESS);
      }
   }
   /* Now start two polling background thread, since XInput is a polled API.
    * The first thread will poll the active joysticks frequently, the second
    * thread will poll the inactive joysticks infrequently.
    * This is done like this to preserve performance.
    */
   joyxi_thread = al_create_thread(joyxi_poll_thread, NULL);
   joyxi_disconnected_thread = al_create_thread(joyxi_poll_disconnected_thread, NULL);

   al_unlock_mutex(joyxi_mutex);

   if (joyxi_thread) al_start_thread(joyxi_thread);
   if (joyxi_disconnected_thread) al_start_thread(joyxi_disconnected_thread);

   return (joyxi_thread != NULL) && (joyxi_disconnected_thread != NULL);
}


static void joyxi_exit_joystick(void)
{
   int index;
   void *ret_value = NULL;
   if (!joyxi_mutex) return;
   if (!joyxi_cond) return;
   if (!joyxi_thread) return;
   /* Request the event thread to shut down, signal the condition, then join the thread. */
   al_set_thread_should_stop(joyxi_thread);
   al_signal_cond(joyxi_cond);
   al_join_thread(joyxi_thread, &ret_value);
   al_set_thread_should_stop(joyxi_disconnected_thread);
   al_signal_cond(joyxi_disconnected_cond);
   al_join_thread(joyxi_disconnected_thread, &ret_value);

   /* clean it all up. */
   al_destroy_thread(joyxi_disconnected_thread);
   al_destroy_cond(joyxi_disconnected_cond);
   al_destroy_thread(joyxi_thread);
   al_destroy_cond(joyxi_cond);

   al_lock_mutex(joyxi_mutex);
   /* Disable xinput */
   _imp_XInputEnable(FALSE);
   /* Wipe the joystick structs */
   for (index = 0; index < MAX_JOYSTICKS; index++) {
      joyxi_joysticks[index].active = false;
   }
   al_unlock_mutex(joyxi_mutex);
   al_destroy_mutex(joyxi_mutex);

   unload_xinput_module();
}


static bool joyxi_reconfigure_joysticks(void)
{
   int index;
   al_lock_mutex(joyxi_mutex);
   for (index = 0; index < MAX_JOYSTICKS; index++) {
      DWORD res = _imp_XInputGetCapabilities(joyxi_joysticks[index].index, 0, &joyxi_joysticks[index].capabilities);
      joyxi_joysticks[index].active = (res == ERROR_SUCCESS);
      if (joyxi_joysticks[index].active) {
         res = _imp_XInputGetState(joyxi_joysticks[index].index, &joyxi_joysticks[index].state);
         joyxi_joysticks[index].active = (res == ERROR_SUCCESS);
      }
   }
   al_unlock_mutex(joyxi_mutex);
   /** Signal the conditions so new events are sent immediately for the new joysticks. */
   al_signal_cond(joyxi_cond);
   /** Signal the disconnected thread in case another joystick got connected. */
   al_signal_cond(joyxi_disconnected_cond);
   return true;
}

static int joyxi_get_num_joysticks(void)
{
   int result = 0, index;
   for (index = 0; index < MAX_JOYSTICKS; index++) {
      if (joyxi_joysticks[index].active)
         result++;
   }
   return result;
}

static ALLEGRO_JOYSTICK *joyxi_get_joystick(int num)
{
   int al_number = 0, index;
   /* Use a linear scan, so the first active joystick ends up as the first
    * allegro joystick etc. */
   for (index = 0; index < MAX_JOYSTICKS; index++) {
      if (joyxi_joysticks[index].active) {
         if (num == al_number)
            return &joyxi_joysticks[index].parent;
         else
            al_number++;
      }
   }
   return NULL;
}


static void joyxi_release_joystick(ALLEGRO_JOYSTICK *joy)
{
   /* No need to do anything. */
   (void)joy;
}

static void joyxi_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ALLEGRO_JOYSTICK_XINPUT *xjoy = (ALLEGRO_JOYSTICK_XINPUT *)joy;
   ASSERT(xjoy);
   ASSERT(ret_state);
   /* Copy the data with the mutex
    * locked to prevent changes during copying. */
   al_lock_mutex(joyxi_mutex);
   (*ret_state) = xjoy->joystate;
   al_unlock_mutex(joyxi_mutex);
}


static const char *joyxi_get_name(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_JOYSTICK_XINPUT *xjoy = (ALLEGRO_JOYSTICK_XINPUT *)joy;
   ASSERT(xjoy);
   return xjoy->name;
}


static bool joyxi_get_active(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_JOYSTICK_XINPUT *xjoy = (ALLEGRO_JOYSTICK_XINPUT *)joy;
   ASSERT(xjoy);
   return xjoy->active;
}


#endif /* #ifdef ALLEGRO_CFG_XINPUT */
