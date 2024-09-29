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
 *      Windows haptic (force-feedback) device driver.
 *
 *      By Beoran.
 *
 *      See LICENSE.txt for copyright information.
 */


#define A5O_NO_COMPATIBILITY

#define DIRECTINPUT_VERSION 0x0800

/* For waitable timers */
#define _WIN32_WINNT 0x0501

#include "allegro5/allegro.h"
#include "allegro5/haptic.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_haptic.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_bitmap.h"

#ifdef A5O_CFG_XINPUT
/* Don't compile this lot if xinput isn't supported. */

#ifndef A5O_WINDOWS
#error something is wrong with the makefile
#endif

#ifndef A5O_XINPUT_POLL_DELAY
#define A5O_XINPUT_POLL_DELAY 0.1
#endif

#ifdef A5O_MINGW32
   #undef MAKEFOURCC
#endif

#include <initguid.h>
#include <stdio.h>
#include <mmsystem.h>
#include <process.h>
#include <math.h>

/* The official DirectX xinput.h uses SAL annotations.
 * Need the sal.h header to get rid of them. On some other platforms 
 * such as MinGW on Linux this header is lacking because it is not needed there. 
 * So, simply try to include sal.h IF we have it , and if not, hope 
 * for the best. 
 * This does no harm on msys2 either, they have a sal.h header.
 */ 
#ifdef A5O_HAVE_SAL_H 
   #include <sal.h>
#endif

#include <xinput.h>

#include "allegro5/internal/aintern_wjoyxi.h"



A5O_DEBUG_CHANNEL("haptic")

/* Support at most 4 haptic devices. */
#define HAPTICS_MAX             4

/* Support at most 1 rumble effect per device, because
 * XInput doesn't really support uploading the effects. */
#define HAPTIC_EFFECTS_MAX     1

/* Enum for the state of a haptic effect. The playback is managed by a small
 *  finite state machine.
 */

typedef enum A5O_HAPTIC_EFFECT_XINPUT_STATE
{
   A5O_HAPTIC_EFFECT_XINPUT_STATE_INACTIVE = 0,
   A5O_HAPTIC_EFFECT_XINPUT_STATE_READY = 1,
   A5O_HAPTIC_EFFECT_XINPUT_STATE_STARTING = 2,
   A5O_HAPTIC_EFFECT_XINPUT_STATE_PLAYING = 3,
   A5O_HAPTIC_EFFECT_XINPUT_STATE_DELAYED = 4,
   A5O_HAPTIC_EFFECT_XINPUT_STATE_STOPPING = 5,
} A5O_HAPTIC_EFFECT_XINPUT_STATE;

/* Allowed state transitions:
 * from inactive to ready,
 * from ready to starting
 * from starting to delayed or playing
 * from delayed to playing
 * from playing to delayed (for loops) or stopping
 * from stopping to ready
 * from ready to inactive
 */

typedef struct A5O_HAPTIC_EFFECT_XINPUT
{
   A5O_HAPTIC_EFFECT effect;
   XINPUT_VIBRATION vibration;
   int id;
   double start_time;
   double loop_start;
   double stop_time;
   int repeats;
   int delay_repeated;
   int play_repeated;
   A5O_HAPTIC_EFFECT_XINPUT_STATE state;
} A5O_HAPTIC_EFFECT_XINPUT;


typedef struct A5O_HAPTIC_XINPUT
{
   /* Important, parent must be first. */
   A5O_HAPTIC parent;
   A5O_JOYSTICK_XINPUT *xjoy;
   bool active;
   A5O_HAPTIC_EFFECT_XINPUT effect;
   int flags;
   /* Only one effect is supported since the XInput API allows only one pair of
      vibration speeds to be set to the device.  */
} A5O_HAPTIC_XINPUT;


/* forward declarations */
static bool hapxi_init_haptic(void);
static void hapxi_exit_haptic(void);

static bool hapxi_is_mouse_haptic(A5O_MOUSE *dev);
static bool hapxi_is_joystick_haptic(A5O_JOYSTICK *);
static bool hapxi_is_keyboard_haptic(A5O_KEYBOARD *dev);
static bool hapxi_is_display_haptic(A5O_DISPLAY *dev);
static bool hapxi_is_touch_input_haptic(A5O_TOUCH_INPUT *dev);

static A5O_HAPTIC *hapxi_get_from_mouse(A5O_MOUSE *dev);
static A5O_HAPTIC *hapxi_get_from_joystick(A5O_JOYSTICK *dev);
static A5O_HAPTIC *hapxi_get_from_keyboard(A5O_KEYBOARD *dev);
static A5O_HAPTIC *hapxi_get_from_display(A5O_DISPLAY *dev);
static A5O_HAPTIC *hapxi_get_from_touch_input(A5O_TOUCH_INPUT *dev);

static bool hapxi_release(A5O_HAPTIC *haptic);

static bool hapxi_get_active(A5O_HAPTIC *hap);
static int hapxi_get_capabilities(A5O_HAPTIC *dev);
static double hapxi_get_gain(A5O_HAPTIC *dev);
static bool hapxi_set_gain(A5O_HAPTIC *dev, double);
static int hapxi_get_max_effects(A5O_HAPTIC *dev);

static bool hapxi_is_effect_ok(A5O_HAPTIC *dev, A5O_HAPTIC_EFFECT *eff);
static bool hapxi_upload_effect(A5O_HAPTIC *dev,
                                A5O_HAPTIC_EFFECT *eff,
                                A5O_HAPTIC_EFFECT_ID *id);
static bool hapxi_play_effect(A5O_HAPTIC_EFFECT_ID *id, int loop);
static bool hapxi_stop_effect(A5O_HAPTIC_EFFECT_ID *id);
static bool hapxi_is_effect_playing(A5O_HAPTIC_EFFECT_ID *id);
static bool hapxi_release_effect(A5O_HAPTIC_EFFECT_ID *id);

static double hapxi_get_autocenter(A5O_HAPTIC *dev);
static bool hapxi_set_autocenter(A5O_HAPTIC *dev, double);

static void *hapxi_poll_thread(A5O_THREAD *thread, void *arg);

A5O_HAPTIC_DRIVER _al_hapdrv_xinput =
{
   AL_HAPTIC_TYPE_XINPUT,
   "",
   "",
   "Windows XInput haptic(s)",
   hapxi_init_haptic,
   hapxi_exit_haptic,

   hapxi_is_mouse_haptic,
   hapxi_is_joystick_haptic,
   hapxi_is_keyboard_haptic,
   hapxi_is_display_haptic,
   hapxi_is_touch_input_haptic,

   hapxi_get_from_mouse,
   hapxi_get_from_joystick,
   hapxi_get_from_keyboard,
   hapxi_get_from_display,
   hapxi_get_from_touch_input,

   hapxi_get_active,
   hapxi_get_capabilities,
   hapxi_get_gain,
   hapxi_set_gain,
   hapxi_get_max_effects,

   hapxi_is_effect_ok,
   hapxi_upload_effect,
   hapxi_play_effect,
   hapxi_stop_effect,
   hapxi_is_effect_playing,
   hapxi_release_effect,

   hapxi_release,

   hapxi_get_autocenter,
   hapxi_set_autocenter
};


static A5O_HAPTIC_XINPUT haptics[HAPTICS_MAX];
/* For the background thread */
static A5O_THREAD *hapxi_thread = NULL;
static A5O_MUTEX  *hapxi_mutex = NULL;
/* Use a condition variable to put the thread to sleep and prevent too
   frequent polling. */
static A5O_COND   *hapxi_cond = NULL;

typedef DWORD(WINAPI *XInputSetStatePROC)(DWORD, XINPUT_VIBRATION*);
extern XInputSetStatePROC _al_imp_XInputSetState;

/* Forces vibration to stop immediately. */
static bool hapxi_force_stop(A5O_HAPTIC_XINPUT *hapxi,
                             A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   XINPUT_VIBRATION no_vibration = { 0, 0 };
   A5O_DEBUG("XInput haptic effect stopped.\n");
   _al_imp_XInputSetState(hapxi->xjoy->index, &no_vibration);
   effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_READY;
   return true;
}

/* starts vibration immediately. If successfully also sets playing   */
static bool hapxi_force_play(A5O_HAPTIC_XINPUT *hapxi,
                             A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   DWORD res;
   res = _al_imp_XInputSetState(hapxi->xjoy->index, &effxi->vibration);
   A5O_DEBUG("Starting to play back haptic effect: %d.\n", (int)(res));
   if (res == ERROR_SUCCESS) {
      effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_PLAYING;
      return true;
   }
   else {
      effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_READY;
      return false;
   }
}



static bool hapxi_poll_haptic_effect_ready(A5O_HAPTIC_XINPUT *hapxi,
                                           A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   (void)hapxi; (void)effxi;
   /* when ready do nothing */
   return true;
}

static bool hapxi_poll_haptic_effect_starting(A5O_HAPTIC_XINPUT *hapxi,
                                              A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   /* when starting switch to delayed mode or play mode */
   double now = al_get_time();
   if ((now - effxi->start_time) < effxi->effect.replay.delay) {
      effxi->loop_start = al_get_time();
      effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_DELAYED;
   }
   else {
      hapxi_force_play(hapxi, effxi);
   }
   A5O_DEBUG("Polling XInput haptic effect. Really Starting: %d!\n", effxi->state);
   return true;
}

static bool hapxi_poll_haptic_effect_playing(A5O_HAPTIC_XINPUT *hapxi,
                                             A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   double now = al_get_time();
   double stop = effxi->loop_start + effxi->effect.replay.delay +
                 effxi->effect.replay.length;
   (void)hapxi;
   if (now > stop) {
      /* may need to repeat play because of "loop" in playback. */
      effxi->play_repeated++;
      if (effxi->play_repeated < effxi->repeats) {
         /* need to play another loop. Stop playing now. */
         hapxi_force_stop(hapxi, effxi);
         effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_DELAYED;
         effxi->loop_start = al_get_time();
      }
      else {
         effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_STOPPING;
      }
      return true;
   }
   return false;
}

static bool hapxi_poll_haptic_effect_delayed(A5O_HAPTIC_XINPUT *hapxi,
                                             A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   double now = al_get_time();
   if (now > (effxi->loop_start + effxi->effect.replay.delay)) {
      return hapxi_force_play(hapxi, effxi);
   }
   return false;
}

static bool hapxi_poll_haptic_effect_stopping(A5O_HAPTIC_XINPUT *hapxi,
                                              A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   /* when stopping, force stop and go to ready state (hapxi_force_stop does this)*/
   return hapxi_force_stop(hapxi, effxi);
}


/* Polls the xinput API for a single haptic device and effect. */
static bool hapxi_poll_haptic_effect(A5O_HAPTIC_XINPUT *hapxi,
                                     A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   /* Check the state of the effect. */
   switch (effxi->state) {
      case A5O_HAPTIC_EFFECT_XINPUT_STATE_INACTIVE:
         return false;
      case A5O_HAPTIC_EFFECT_XINPUT_STATE_READY:
         return hapxi_poll_haptic_effect_ready(hapxi, effxi);
      case A5O_HAPTIC_EFFECT_XINPUT_STATE_STARTING:
         return hapxi_poll_haptic_effect_starting(hapxi, effxi);
      case A5O_HAPTIC_EFFECT_XINPUT_STATE_PLAYING:
         return hapxi_poll_haptic_effect_playing(hapxi, effxi);
      case A5O_HAPTIC_EFFECT_XINPUT_STATE_DELAYED:
         return hapxi_poll_haptic_effect_delayed(hapxi, effxi);
      case A5O_HAPTIC_EFFECT_XINPUT_STATE_STOPPING:
         return hapxi_poll_haptic_effect_stopping(hapxi, effxi);
      default:
         A5O_DEBUG("XInput haptic effect state not valid :%d.\n", effxi->state);
         return false;
   }
}

/* Polls the xinput API for a single haptic device. */
static void hapxi_poll_haptic(A5O_HAPTIC_XINPUT *hapxi)
{
   hapxi_poll_haptic_effect(hapxi, &hapxi->effect);
}

/* Polls the xinput API for hapxi effect and starts
 * or stops playback when needed.
 */
static void hapxi_poll_haptics(void)
{
   int i;

   for (i = 0; i < HAPTICS_MAX; i++) {
      if (haptics[i].active) {
         hapxi_poll_haptic(haptics + i);
      }
   }
}


/* Function for the haptics polling thread. */
static void *hapxi_poll_thread(A5O_THREAD *thread, void *arg)
{
   A5O_TIMEOUT timeout;
   al_lock_mutex(hapxi_mutex);
   while (!al_get_thread_should_stop(thread)) {
      /* Poll once every 10 milliseconds. XXX: Should this be configurable? */
      al_init_timeout(&timeout, A5O_XINPUT_POLL_DELAY);
      /* Wait for the condition to allow the
         polling thread to be awoken when needed. */
      al_wait_cond_until(hapxi_cond, hapxi_mutex, &timeout);
      /* If we get here poll joystick for new input or connection
       * and dispatch events. */
      hapxi_poll_haptics();
   }
   al_unlock_mutex(hapxi_mutex);
   return arg;
}



/* Initializes the XInput haptic system. */
static bool hapxi_init_haptic(void)
{
   int i;

   ASSERT(hapxi_mutex == NULL);
   ASSERT(hapxi_thread == NULL);
   ASSERT(hapxi_cond == NULL);


   /* Create the mutex and a condition vaiable. */
   hapxi_mutex = al_create_mutex_recursive();
   if (!hapxi_mutex)
      return false;
   hapxi_cond = al_create_cond();
   if (!hapxi_cond)
      return false;

   al_lock_mutex(hapxi_mutex);

   for (i = 0; i < HAPTICS_MAX; i++) {
      haptics[i].active = false;
   }

   /* Now start a polling background thread, since XInput is a polled API,
      and also to make it possible for effects to stop running when their
      duration has passed. */
   hapxi_thread = al_create_thread(hapxi_poll_thread, NULL);
   al_unlock_mutex(hapxi_mutex);
   if (hapxi_thread) al_start_thread(hapxi_thread);
   return(hapxi_thread != NULL);
}


/* Converts a generic haptic device to a Windows-specific one. */
static A5O_HAPTIC_XINPUT *hapxi_from_al(A5O_HAPTIC *hap)
{
   return (A5O_HAPTIC_XINPUT *)hap;
}

static void hapxi_exit_haptic(void)
{
   void *ret_value;
   ASSERT(hapxi_thread);
   ASSERT(hapxi_mutex);
   ASSERT(hapxi_cond);

   /* Request the event thread to shut down, signal the condition, then join the thread. */
   al_set_thread_should_stop(hapxi_thread);
   al_signal_cond(hapxi_cond);
   al_join_thread(hapxi_thread, &ret_value);

   /* clean it all up. */
   al_destroy_thread(hapxi_thread);
   al_destroy_cond(hapxi_cond);

   al_destroy_mutex(hapxi_mutex);
   hapxi_mutex = NULL;
}

/* Converts a float to a unsigned WORD range */
static bool hapxi_magnitude2win(WORD *word, double value)
{
   if (!word) return false;
   (*word) = (WORD)(65535 * value);
   return true;
}

/* Converts Allegro haptic effect to xinput API. */
static bool hapxi_effect2win(
   A5O_HAPTIC_EFFECT_XINPUT *effxi,
   A5O_HAPTIC_EFFECT *effect,
   A5O_HAPTIC_XINPUT *hapxi)
{
   (void)hapxi;
   /* Generic setup */
   if (effect->type != A5O_HAPTIC_RUMBLE)
      return false;

   /* Right motor is "weaker" than left one, That is, right produces a less
      violent vibration than left. */
   return
      hapxi_magnitude2win(&effxi->vibration.wRightMotorSpeed,
                          effect->data.rumble.weak_magnitude) &&
      hapxi_magnitude2win(&effxi->vibration.wLeftMotorSpeed,
                          effect->data.rumble.strong_magnitude);
}

static bool hapxi_get_active(A5O_HAPTIC *haptic)
{
   A5O_HAPTIC_XINPUT *hapxi = hapxi_from_al(haptic);
   return hapxi->active;
}



static bool hapxi_is_mouse_haptic(A5O_MOUSE *mouse)
{
   (void)mouse;
   return false;
}


static bool hapxi_is_joystick_haptic(A5O_JOYSTICK *joy)
{
   A5O_JOYSTICK_XINPUT *joyxi = (A5O_JOYSTICK_XINPUT *)joy;
   if (!al_is_joystick_installed())
      return false;
   if (!al_get_joystick_active(joy))
      return false;

   /* IF this flag is not supported, then it means we're compiling against
      an older XInput library. In this case, the Flags are inoperable,
      and force feedback must be assumed to be always available.  */
#ifndef XINPUT_CAPS_FFB_SUPPORTED
   (void)joyxi;
   A5O_DEBUG("Compiled against older XInput library, assuming force feedback support is available.\n");
   return true;
#else
   A5O_DEBUG("joyxi->capabilities.Flags: %d <-> %d\n", joyxi->capabilities.Flags, XINPUT_CAPS_FFB_SUPPORTED);
   return(joyxi->capabilities.Flags & XINPUT_CAPS_FFB_SUPPORTED);
#endif
}


static bool hapxi_is_display_haptic(A5O_DISPLAY *dev)
{
   (void)dev;
   return false;
}


static bool hapxi_is_keyboard_haptic(A5O_KEYBOARD *dev)
{
   (void)dev;
   return false;
}


static bool hapxi_is_touch_input_haptic(A5O_TOUCH_INPUT *dev)
{
   (void)dev;
   return false;
}


static A5O_HAPTIC *hapxi_get_from_mouse(A5O_MOUSE *mouse)
{
   (void)mouse;
   return NULL;
}


static A5O_HAPTIC *hapxi_get_from_joystick(A5O_JOYSTICK *joy)
{
   A5O_JOYSTICK_XINPUT *joyxi = (A5O_JOYSTICK_XINPUT *)joy;
   A5O_HAPTIC_XINPUT   *hapxi;

   if (!al_is_joystick_haptic(joy))
      return NULL;

   al_lock_mutex(hapxi_mutex);

   hapxi = haptics + joyxi->index;
   hapxi->parent.driver = &_al_hapdrv_xinput;
   hapxi->parent.device = joyxi;
   hapxi->parent.from = _AL_HAPTIC_FROM_JOYSTICK;
   hapxi->active = true;
   hapxi->effect.state = A5O_HAPTIC_EFFECT_XINPUT_STATE_INACTIVE;
   /* not in use */
   hapxi->parent.gain = 1.0;
   hapxi->parent.autocenter = 0.0;
   hapxi->flags = A5O_HAPTIC_RUMBLE;
   hapxi->xjoy = joyxi;
   al_unlock_mutex(hapxi_mutex);

   return &hapxi->parent;
}


static A5O_HAPTIC *hapxi_get_from_display(A5O_DISPLAY *dev)
{
   (void)dev;
   return NULL;
}


static A5O_HAPTIC *hapxi_get_from_keyboard(A5O_KEYBOARD *dev)
{
   (void)dev;
   return NULL;
}


static A5O_HAPTIC *hapxi_get_from_touch_input(A5O_TOUCH_INPUT *dev)
{
   (void)dev;
   return NULL;
}


static int hapxi_get_capabilities(A5O_HAPTIC *dev)
{
   A5O_HAPTIC_XINPUT *hapxi = hapxi_from_al(dev);
   return hapxi->flags;
}


static double hapxi_get_gain(A5O_HAPTIC *dev)
{
   (void)dev;
   /* Just return the 1.0, gain isn't supported  */
   return 1.0;
}


static bool hapxi_set_gain(A5O_HAPTIC *dev, double gain)
{
   (void)dev; (void)gain;
   /* Gain not supported*/
   return false;
}


double hapxi_get_autocenter(A5O_HAPTIC *dev)
{
   (void)dev;
   /* Autocenter not supported so return 0.0. */
   return 0.0;
}


static bool hapxi_set_autocenter(A5O_HAPTIC *dev, double intensity)
{
   (void)dev; (void)intensity;
   /* Autocenter not supported*/
   return false;
}

static int hapxi_get_max_effects(A5O_HAPTIC *dev)
{
   (void)dev;
   /* Support only one effect */
   return 1;
}


static bool hapxi_is_effect_ok(A5O_HAPTIC *haptic,
                               A5O_HAPTIC_EFFECT *effect)
{
   int caps;

   caps = al_get_haptic_capabilities(haptic);
   if (caps & effect->type) {
      return true;
   }
   return false;
}

/* Gets an available haptic effect slot from the device or NULL if not
 * available.
 */
static A5O_HAPTIC_EFFECT_XINPUT *
hapxi_get_available_effect(A5O_HAPTIC_XINPUT *hapxi)
{
   if (hapxi->effect.state == A5O_HAPTIC_EFFECT_XINPUT_STATE_INACTIVE) {
      /* Set up ID here. */
      hapxi->effect.id = 0;
      return &hapxi->effect;
   }
   return NULL;
}

static bool hapxi_release_effect_windows(A5O_HAPTIC_EFFECT_XINPUT *effxi)
{
   effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_INACTIVE;
   return true;
}

static bool hapxi_upload_effect(A5O_HAPTIC *dev,
                                A5O_HAPTIC_EFFECT *effect, A5O_HAPTIC_EFFECT_ID *id)
{
   A5O_HAPTIC_XINPUT *hapxi = hapxi_from_al(dev);
   A5O_HAPTIC_EFFECT_XINPUT *effxi = NULL;

   ASSERT(dev);
   ASSERT(id);
   ASSERT(effect);

   /* Set id's values to indicate failure beforehand. */
   id->_haptic = NULL;
   id->_id = -1;
   id->_pointer = NULL;
   id->_playing = false;
   id->_effect_duration = 0.0;
   id->_start_time = 0.0;
   id->_end_time = 0.0;

   if (!al_is_haptic_effect_ok(dev, effect))
      return false;

   al_lock_mutex(hapxi_mutex);

   /* Is a haptic effect slot available? */
   effxi = hapxi_get_available_effect(hapxi);
   /* No more space for an effect. */
   if (!effxi) {
      A5O_WARN("No free effect slot.");
      al_unlock_mutex(hapxi_mutex);
      return false;
   }

   if (!hapxi_effect2win(effxi, effect, hapxi)) {
      A5O_WARN("Cannot convert haptic effect to XINPUT effect.\n");
      al_unlock_mutex(hapxi_mutex);
      return false;
   }

   effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_READY;
   effxi->effect = (*effect);
   /* set ID handle to signify success */
   id->_haptic = dev;
   id->_pointer = effxi;
   id->_id = effxi->id;
   id->_effect_duration = al_get_haptic_effect_duration(effect);

   al_unlock_mutex(hapxi_mutex);
   return true;
}


static A5O_HAPTIC_XINPUT *
hapxi_device_for_id(A5O_HAPTIC_EFFECT_ID *id)
{
   return (A5O_HAPTIC_XINPUT *)id->_haptic;
}


static A5O_HAPTIC_EFFECT_XINPUT *
hapxi_effect_for_id(A5O_HAPTIC_EFFECT_ID *id)
{
   return (A5O_HAPTIC_EFFECT_XINPUT *)id->_pointer;
}

static bool hapxi_play_effect(A5O_HAPTIC_EFFECT_ID *id, int loops)
{
   A5O_HAPTIC_XINPUT *hapxi = hapxi_device_for_id(id);
   A5O_HAPTIC_EFFECT_XINPUT *effxi = hapxi_effect_for_id(id);

   if ((!hapxi) || (id->_id < 0) || (!effxi) || (loops < 1))
      return false;
   al_lock_mutex(hapxi_mutex);
   /* Simply set some flags. The polling thread will see this and start playing.
      after the effect's delay has passed. */
   effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_STARTING;
   effxi->start_time = al_get_time();
   effxi->stop_time = effxi->start_time + al_get_haptic_effect_duration(&effxi->effect) * loops;
   effxi->repeats = loops;
   effxi->play_repeated = 0;
   effxi->loop_start = effxi->start_time;

   id->_playing = true;
   id->_start_time = al_get_time();
   id->_start_time = effxi->start_time;
   id->_end_time = effxi->stop_time;
   al_unlock_mutex(hapxi_mutex);
   al_signal_cond(hapxi_cond);
   return true;
}


static bool hapxi_stop_effect(A5O_HAPTIC_EFFECT_ID *id)
{
   A5O_HAPTIC_XINPUT *hapxi = hapxi_device_for_id(id);
   A5O_HAPTIC_EFFECT_XINPUT *effxi = hapxi_effect_for_id(id);

   if ((!hapxi) || (id->_id < 0))
      return false;
   /* Simply set some flags. The polling thread will see this and stop playing.*/
   effxi = (A5O_HAPTIC_EFFECT_XINPUT *)id->_pointer;
   if (effxi->state <= A5O_HAPTIC_EFFECT_XINPUT_STATE_READY) return false;
   al_lock_mutex(hapxi_mutex);
   effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_STOPPING;
   id->_playing = false;
   al_unlock_mutex(hapxi_mutex);
   al_signal_cond(hapxi_cond);
   return true;
}


static bool hapxi_is_effect_playing(A5O_HAPTIC_EFFECT_ID *id)
{
   A5O_HAPTIC_XINPUT *hapxi;
   A5O_HAPTIC_EFFECT_XINPUT *effxi;
   bool result;
   ASSERT(id);

   hapxi = hapxi_device_for_id(id);
   effxi = hapxi_effect_for_id(id);

   if ((!hapxi) || (id->_id < 0) || (!id->_playing))
      return false;
   al_lock_mutex(hapxi_mutex);
   A5O_DEBUG("Playing effect state: %d %p %lf %lf\n", effxi->state, effxi, al_get_time(), id->_end_time);

   result = (effxi->state > A5O_HAPTIC_EFFECT_XINPUT_STATE_READY);
   al_unlock_mutex(hapxi_mutex);
   al_signal_cond(hapxi_cond);

   return result;
}


static bool hapxi_release_effect(A5O_HAPTIC_EFFECT_ID *id)
{
   A5O_HAPTIC_XINPUT *hapxi = hapxi_device_for_id(id);
   A5O_HAPTIC_EFFECT_XINPUT *effxi = hapxi_effect_for_id(id);
   bool result;
   if ((!hapxi) || (!effxi))
      return false;

   al_lock_mutex(hapxi_mutex);
   /* Forcefully stop since a normal stop may not be instant. */
   hapxi_force_stop(hapxi, effxi);
   effxi->state = A5O_HAPTIC_EFFECT_XINPUT_STATE_INACTIVE;
   result = hapxi_release_effect_windows(effxi);
   al_unlock_mutex(hapxi_mutex);
   return result;
}


static bool hapxi_release(A5O_HAPTIC *haptic)
{
   A5O_HAPTIC_XINPUT *hapxi = hapxi_from_al(haptic);
   ASSERT(haptic);

   if (!hapxi->active)
      return false;
   al_lock_mutex(hapxi_mutex);

   /* Release the effect for this device. */
   /* Forcefully stop since a normal stop may not be instant. */
   hapxi_force_stop(hapxi, &hapxi->effect);
   hapxi_release_effect_windows(&hapxi->effect);

   hapxi->active = false;
   hapxi->parent.device = NULL;
   al_unlock_mutex(hapxi_mutex);
   return true;
}

#endif

/* vim: set sts=3 sw=3 et: */
