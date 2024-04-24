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



#define ALLEGRO_NO_COMPATIBILITY

#define DIRECTINPUT_VERSION 0x0800

/* For waitable timers */
#define _WIN32_WINNT 0x0501

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_haptic.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_bitmap.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#ifdef ALLEGRO_MINGW32
#undef MAKEFOURCC
#endif

#include <initguid.h>
#include <stdio.h>
#include <mmsystem.h>
#include <process.h>
#include <math.h>
#include <dinput.h>
/* #include <sys/time.h> */

#include "allegro5/internal/aintern_wjoydxnu.h"

ALLEGRO_DEBUG_CHANNEL("whaptic")

/* Support at most 32 haptic devices. */
#define HAPTICS_MAX             32
/* Support at most 16 effects per device. */
#define HAPTICS_EFFECTS_MAX     16
/* Support at most 3 axes per device. */
#define HAPTICS_AXES_MAX        3

/** This union is needed to avoid
   dynamical memory allocation. */
typedef union
{
   DICONSTANTFORCE constant;
   DIRAMPFORCE ramp;
   DIPERIODIC periodic;
   DICONDITION condition;
   DICUSTOMFORCE custom;
} ALLEGRO_HAPTIC_PARAMETER_WINDOWS;


/*
 * Haptic effect system data.
 */
struct ALLEGRO_HAPTIC_EFFECT_WINDOWS
{
   int id;
   bool active;
   DIEFFECT effect;
   DIENVELOPE envelope;
   LPDIRECTINPUTEFFECT ref;
   DWORD axes[HAPTICS_AXES_MAX];
   LONG directions[HAPTICS_AXES_MAX];
   ALLEGRO_HAPTIC_PARAMETER_WINDOWS parameter;
   const GUID* guid;
};



struct ALLEGRO_HAPTIC_WINDOWS
{
   struct ALLEGRO_HAPTIC parent;        /* must be first */
   bool active;
   LPDIRECTINPUTDEVICE2 device;
   GUID guid;
   DIDEVICEINSTANCE instance;
   DIDEVCAPS capabilities;
   LPDIRECTINPUTDEVICE8 device8;
   ALLEGRO_DISPLAY_WIN* display;
   int flags;
   ALLEGRO_HAPTIC_EFFECT_WINDOWS effects[HAPTICS_EFFECTS_MAX];
   DWORD axes[HAPTICS_AXES_MAX];
   int naxes;
};


#define LONG_BITS    (sizeof(long) * 8)
#define NLONGS(x)    (((x) + LONG_BITS - 1) / LONG_BITS)
/* Tests if a bit in an array of longs is set. */
#define TEST_BIT(nr, addr) \
   ((1UL << ((nr) % LONG_BITS)) & (addr)[(nr) / LONG_BITS])


/* forward declarations */
static bool whap_init_haptic(void);
static void whap_exit_haptic(void);

static bool whap_is_mouse_haptic(ALLEGRO_MOUSE *dev);
static bool whap_is_joystick_haptic(ALLEGRO_JOYSTICK *);
static bool whap_is_keyboard_haptic(ALLEGRO_KEYBOARD *dev);
static bool whap_is_display_haptic(ALLEGRO_DISPLAY *dev);
static bool whap_is_touch_input_haptic(ALLEGRO_TOUCH_INPUT *dev);

static ALLEGRO_HAPTIC *whap_get_from_mouse(ALLEGRO_MOUSE *dev);
static ALLEGRO_HAPTIC *whap_get_from_joystick(ALLEGRO_JOYSTICK *dev);
static ALLEGRO_HAPTIC *whap_get_from_keyboard(ALLEGRO_KEYBOARD *dev);
static ALLEGRO_HAPTIC *whap_get_from_display(ALLEGRO_DISPLAY *dev);
static ALLEGRO_HAPTIC *whap_get_from_touch_input(ALLEGRO_TOUCH_INPUT *dev);

static bool whap_release(ALLEGRO_HAPTIC *haptic);

static bool whap_get_active(ALLEGRO_HAPTIC *hap);
static int whap_get_capabilities(ALLEGRO_HAPTIC *dev);
static double whap_get_gain(ALLEGRO_HAPTIC *dev);
static bool whap_set_gain(ALLEGRO_HAPTIC *dev, double);
static int whap_get_max_effects(ALLEGRO_HAPTIC *dev);

static bool whap_is_effect_ok(ALLEGRO_HAPTIC *dev,
                              ALLEGRO_HAPTIC_EFFECT *eff);
static bool whap_upload_effect(ALLEGRO_HAPTIC *dev,
                               ALLEGRO_HAPTIC_EFFECT *eff,
                               ALLEGRO_HAPTIC_EFFECT_ID *id);
static bool whap_play_effect(ALLEGRO_HAPTIC_EFFECT_ID *id, int loop);
static bool whap_stop_effect(ALLEGRO_HAPTIC_EFFECT_ID *id);
static bool whap_is_effect_playing(ALLEGRO_HAPTIC_EFFECT_ID *id);
static bool whap_release_effect(ALLEGRO_HAPTIC_EFFECT_ID *id);

static double whap_get_autocenter(ALLEGRO_HAPTIC *dev);
static bool whap_set_autocenter(ALLEGRO_HAPTIC *dev, double);

ALLEGRO_HAPTIC_DRIVER _al_hapdrv_directx = {
   AL_HAPTIC_TYPE_DIRECTX,
   "",
   "",
   "Windows haptic(s)",
   whap_init_haptic,
   whap_exit_haptic,

   whap_is_mouse_haptic,
   whap_is_joystick_haptic,
   whap_is_keyboard_haptic,
   whap_is_display_haptic,
   whap_is_touch_input_haptic,

   whap_get_from_mouse,
   whap_get_from_joystick,
   whap_get_from_keyboard,
   whap_get_from_display,
   whap_get_from_touch_input,

   whap_get_active,
   whap_get_capabilities,
   whap_get_gain,
   whap_set_gain,
   whap_get_max_effects,

   whap_is_effect_ok,
   whap_upload_effect,
   whap_play_effect,
   whap_stop_effect,
   whap_is_effect_playing,
   whap_release_effect,

   whap_release,

   whap_get_autocenter,
   whap_set_autocenter
};


static ALLEGRO_HAPTIC_WINDOWS haptics[HAPTICS_MAX];
static ALLEGRO_MUTEX *haptic_mutex = NULL;

/* Capability map between directinput effects and allegro effect types. */
struct CAP_MAP
{
   GUID guid;
   int allegro_bit;
};

/* GUID values are borrowed from Wine */
#define DEFINE_PRIVATE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
   static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

DEFINE_PRIVATE_GUID(_al_GUID_None, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


static const struct CAP_MAP cap_map[] = {
   { GUID_ConstantForce, ALLEGRO_HAPTIC_CONSTANT },
   { GUID_Spring, ALLEGRO_HAPTIC_SPRING },
   { GUID_Spring, ALLEGRO_HAPTIC_FRICTION },
   { GUID_Damper, ALLEGRO_HAPTIC_DAMPER },
   { GUID_Inertia, ALLEGRO_HAPTIC_INERTIA },
   { GUID_RampForce, ALLEGRO_HAPTIC_RAMP },
   { GUID_Square, ALLEGRO_HAPTIC_SQUARE },
   { GUID_Triangle, ALLEGRO_HAPTIC_TRIANGLE },
   { GUID_Sine, ALLEGRO_HAPTIC_SINE },
   { GUID_SawtoothUp, ALLEGRO_HAPTIC_SAW_UP },
   { GUID_SawtoothDown, ALLEGRO_HAPTIC_SAW_DOWN },
   { GUID_CustomForce, ALLEGRO_HAPTIC_CUSTOM },
   /*{ { _al_GUID_None    },      -1 } */
};


static bool whap_init_haptic(void)
{
   int i;

   ASSERT(haptic_mutex == NULL);
   haptic_mutex = al_create_mutex();
   if (!haptic_mutex)
      return false;

   for (i = 0; i < HAPTICS_MAX; i++) {
      haptics[i].active = false;
   }

   return true;
}


static ALLEGRO_HAPTIC_WINDOWS *whap_get_available_haptic(void)
{
   int i;

   for (i = 0; i < HAPTICS_MAX; i++) {
      if (!haptics[i].active) {
         haptics[i].active = true;
         return &haptics[i];
      }
   }

   return NULL;
}

/* Look for a free haptic effect slot for a device and return it,
 * or NULL if exhausted. Also initializes the effect
 * reference to NULL. */
static ALLEGRO_HAPTIC_EFFECT_WINDOWS
*whap_get_available_effect(ALLEGRO_HAPTIC_WINDOWS *whap)
{
   ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff;
   int i;
   for (i = 0; i < al_get_max_haptic_effects(&whap->parent); i++) {
      if (!whap->effects[i].active) {
         weff = whap->effects + i;
         weff->id = i;
         weff->active = true;
         weff->ref = NULL;
         return weff;
      }
   }
   return NULL;
}


/* Releases a windows haptics effect and unloads it from the device. */
static bool whap_release_effect_windows(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff)
{
   bool result = true;
   if (!weff)
      return false;             /* make it easy to handle all cases later on. */
   if (!weff->active)
      return false;             /* already not in use, bail out. */

   /* Unload the effect from the device. */
   if (weff->ref) {
      HRESULT ret;
      ret = IDirectInputEffect_Unload(weff->ref);
      if (FAILED(ret)) {
         ALLEGRO_WARN("Could not unload effect.");
         result = false;
      }
   }
   /* Custom force needs to clean up it's data. */
   if (weff->guid == &GUID_CustomForce) {
      al_free(weff->parameter.custom.rglForceData);
      weff->parameter.custom.rglForceData = NULL;
   }
   weff->active = false;        /* not in use */
   weff->ref = NULL;            /* No reference to effect anymore. */
   return result;
}


/* Converts a generic haptic device to a Windows-specific one. */
static ALLEGRO_HAPTIC_WINDOWS *whap_from_al(ALLEGRO_HAPTIC *hap)
{
   return (ALLEGRO_HAPTIC_WINDOWS *)hap;
}

static void whap_exit_haptic(void)
{
   ASSERT(haptic_mutex);
   al_destroy_mutex(haptic_mutex);
   haptic_mutex = NULL;
}

/* Convert the type of the periodic allegro effect to the windows effect*/
static bool whap_periodictype2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                                  ALLEGRO_HAPTIC_EFFECT *effect)
{
   switch (effect->data.periodic.waveform) {
      case ALLEGRO_HAPTIC_SINE:
         weff->guid = &GUID_Sine;
         return true;

      case ALLEGRO_HAPTIC_SQUARE:
         weff->guid = &GUID_Square;
         return true;

      case ALLEGRO_HAPTIC_TRIANGLE:
         weff->guid = &GUID_Triangle;
         return true;

      case ALLEGRO_HAPTIC_SAW_UP:
         weff->guid = &GUID_SawtoothUp;
         return true;

      case ALLEGRO_HAPTIC_SAW_DOWN:
         weff->guid = &GUID_SawtoothDown;
         return true;

      case ALLEGRO_HAPTIC_CUSTOM:
         weff->guid = &GUID_CustomForce;
         return true;
      default:
         return false;
   }
}



/* Convert the type of the allegro effect to the windows effect*/
static bool whap_type2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                          ALLEGRO_HAPTIC_EFFECT *effect)
{
   switch (effect->type) {
      case ALLEGRO_HAPTIC_RUMBLE:
         weff->guid = &GUID_Sine;
         return true;
      case ALLEGRO_HAPTIC_PERIODIC:
         return whap_periodictype2win(weff, effect);
      case ALLEGRO_HAPTIC_CONSTANT:
         weff->guid = &GUID_ConstantForce;
         return true;
      case ALLEGRO_HAPTIC_SPRING:
         weff->guid = &GUID_Spring;
         return true;
      case ALLEGRO_HAPTIC_FRICTION:
         weff->guid = &GUID_Friction;
         return true;
      case ALLEGRO_HAPTIC_DAMPER:
         weff->guid = &GUID_Damper;
         return true;
      case ALLEGRO_HAPTIC_INERTIA:
         weff->guid = &GUID_Inertia;
         return true;
      case ALLEGRO_HAPTIC_RAMP:
         weff->guid = &GUID_RampForce;
         return true;
      default:
         return NULL;
   }
   return true;
}

/* Convert the direction of the allegro effect to the windows effect*/
static bool whap_direction2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                               ALLEGRO_HAPTIC_EFFECT *effect,
                               ALLEGRO_HAPTIC_WINDOWS *whap)
{
   unsigned int index;
   double calc_x, calc_y;

   /* It seems that (at least for sine motions), 1 or 2 axes work, but 0 or 3 don't
      for my test joystick. Also, while polar or spherical coordinates are accepted,
      they don't cause any vibration for a periodic effect. All in
      all it seems that cartesian is the only well-supported axis system,
      at least for periodic effects. Need more tests with other joysticks to see
      their behavior.
      Annoyingly there seems to be no way to discover
      what kind of axis system is supported without trying to upload the
      effect... Hence, use a 1 or 2 axis cartesian system and hope for the
      best for non-periodic effects.
    */


   /* Use CARTESIAN coordinates since those seem to be the only well suppported
      ones. */
   weff->effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
   /* Prepare axes. If whap naxes is > 2, use 2 because more than 2
      axes isn't always supported. */
   weff->effect.cAxes = whap->naxes;
   if (weff->effect.cAxes > 2) {
      weff->effect.cAxes = 2;
   }
   memset((void *)weff->axes, 0, sizeof(weff->axes));
   for (index = 0; index < weff->effect.cAxes; index++) {
      weff->axes[index] = whap->axes[index];
   }
   weff->effect.rgdwAxes = weff->axes;
   /* Set up directions as well.. */
   memset((void *)weff->directions, 0, sizeof(weff->directions));
   /* Calculate the X and Y coordinates of the effect based on the angle.
      That is map angular coordinates to cartesian ones. */
   calc_x =
      sin(effect->direction.angle) * effect->direction.radius *
      DI_FFNOMINALMAX;
   calc_y =
      cos(effect->direction.angle) * effect->direction.radius *
      DI_FFNOMINALMAX;

   /* Set X if there is 1 axis and also y if there are more .
    */
   if (weff->effect.cAxes > 1) {
      weff->directions[0] = (long)calc_x;
   }
   if (whap->naxes > 2) {
      weff->directions[1] = (long)calc_y;
   }
   weff->effect.rglDirection = weff->directions;
   return true;
}

/* Converts the time in seconds to a Windows-compatible time.
 * Return false if out of bounds.
 */
static bool whap_time2win(DWORD *res, double sec)
{
   ASSERT(res);

   if (sec < 0.0 || sec >= 4294.967296)
      return false;
   (*res) = (DWORD)floor(sec * DI_SECONDS);
   return true;
}

/* Converts the level in range 0.0 to 1.0 to a Windows-compatible level.
 * Returns false if out of bounds.
 */
static bool whap_level2win(DWORD *res, double level)
{
   ASSERT(res);

   if (level < 0.0 || level > 1.0)
      return false;
   *res = (DWORD)floor(level * DI_FFNOMINALMAX);
   return true;
}


/* Converts the level in range -1.0 to 1.0 to a Windows-compatible level.
 * Returns false if out of bounds.
 */
static bool whap_slevel2win(LONG *res, double level)
{
   ASSERT(res);

   if (level < -1.0 || level > 1.0)
      return false;
   *res = (LONG)(level * DI_FFNOMINALMAX);
   return true;
}

/* Converts a phase in range 0.0 to 1.0 to a Windows-compatible level.
 * Returns false if out of bounds.
 */
static bool whap_phase2win(DWORD *res, double phase)
{
   ASSERT(res);

   if (phase < 0.0 || phase > 1.0)
      return false;
   *res = (DWORD)(phase * 35999);
   return true;
}


/* Converts replay data to Widows-compatible data. */
static bool whap_replay2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                            ALLEGRO_HAPTIC_EFFECT *effect)
{
   return whap_time2win(&weff->effect.dwStartDelay, effect->replay.delay)
          && whap_time2win(&weff->effect.dwDuration, effect->replay.length);
}


/* Converts an Allegro haptic effect envelope to DirectInput API. */
static bool whap_envelope2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                              ALLEGRO_HAPTIC_ENVELOPE *aenv)
{
   /* Prepare envelope. */
   DIENVELOPE *wenv = &weff->envelope;

   /* Do not set any envelope if all values are 0.0  */
   if ((aenv->attack_length == 0.0) &&
       (aenv->fade_length == 0.0) &&
       (aenv->attack_level == 0.0) && (aenv->fade_level == 0.0)) {
      return true;
   }

   /* Prepare the envelope. */
   memset((void *)wenv, 0, sizeof(DIENVELOPE));
   weff->envelope.dwSize = sizeof(DIENVELOPE);
   weff->effect.lpEnvelope = wenv;

   /* Set the values. */
   return whap_time2win(&wenv->dwAttackTime, aenv->attack_length)
          && whap_time2win(&wenv->dwFadeTime, aenv->fade_length)
          && whap_level2win(&wenv->dwAttackLevel, aenv->attack_level)
          && whap_level2win(&wenv->dwFadeLevel, aenv->fade_level);
}

/* Converts a constant effect to directinput API. */
static bool whap_constant2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                              ALLEGRO_HAPTIC_EFFECT *effect)
{
   weff->effect.cbTypeSpecificParams = sizeof(weff->parameter.constant);
   weff->effect.lpvTypeSpecificParams = &weff->parameter.constant;
   return whap_envelope2win(weff, &effect->data.constant.envelope)
          && whap_slevel2win(&weff->parameter.constant.lMagnitude,
                             effect->data.constant.level);
}


/* Converts a ramp effect to directinput input API. */
static bool whap_ramp2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                          ALLEGRO_HAPTIC_EFFECT *effect)
{
   weff->effect.cbTypeSpecificParams = sizeof(weff->parameter.ramp);
   weff->effect.lpvTypeSpecificParams = &weff->parameter.ramp;

   return whap_envelope2win(weff, &effect->data.ramp.envelope)
          && whap_slevel2win(&weff->parameter.ramp.lStart,
                             effect->data.ramp.start_level)
          && whap_slevel2win(&weff->parameter.ramp.lEnd,
                             effect->data.ramp.end_level);
}

/* Converts a condition effect to directinput input API. */
static bool whap_condition2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                               ALLEGRO_HAPTIC_EFFECT *effect)
{
   weff->effect.cbTypeSpecificParams = sizeof(weff->parameter.condition);
   weff->effect.lpvTypeSpecificParams = &weff->parameter.condition;
   /* XXX: no envelope here ???  */

   return whap_level2win(&weff->parameter.condition.dwNegativeSaturation,
                         effect->data.condition.left_saturation)
          && whap_level2win(&weff->parameter.condition.dwPositiveSaturation,
                            effect->data.condition.right_saturation)
          && whap_slevel2win(&weff->parameter.condition.lNegativeCoefficient,
                             effect->data.condition.left_coeff)
          && whap_slevel2win(&weff->parameter.condition.lPositiveCoefficient,
                             effect->data.condition.right_coeff)
          && whap_slevel2win(&weff->parameter.condition.lDeadBand,
                             effect->data.condition.deadband)
          && whap_slevel2win(&weff->parameter.condition.lOffset,
                             effect->data.condition.center);
}

/* Converts a custom effect to directinput input API. */
static bool whap_custom2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                            ALLEGRO_HAPTIC_EFFECT *effect)
{
   int index;
   weff->effect.cbTypeSpecificParams = sizeof(weff->parameter.custom);
   weff->effect.lpvTypeSpecificParams = &weff->parameter.custom;
   weff->parameter.custom.cChannels = 1;
   weff->parameter.custom.cSamples = effect->data.periodic.custom_len;
   /* Use al malloc only in this case since the custom_data can be arbitrarily long. */
   weff->parameter.custom.rglForceData =
      (LONG *)al_malloc(sizeof(LONG) * effect->data.periodic.custom_len);
   if (!weff->parameter.custom.rglForceData)
      return false;
   /* Gotta copy this to long values, and scale them too... */
   for (index = 0; index < effect->data.periodic.custom_len; index++) {
      weff->parameter.custom.rglForceData[index] =
         (LONG)(effect->data.periodic.custom_data[index] *
                ((double)(1 << 31)));
   }
   return true;
}


/* Converts a periodic effect to directinput input API. */
static bool whap_periodic2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                              ALLEGRO_HAPTIC_EFFECT *effect)
{
   if (effect->data.periodic.waveform == ALLEGRO_HAPTIC_CUSTOM) {
      return whap_custom2win(weff, effect);
   }

   weff->effect.cbTypeSpecificParams = sizeof(weff->parameter.periodic);
   weff->effect.lpvTypeSpecificParams = &weff->parameter.periodic;

   return whap_envelope2win(weff, &effect->data.periodic.envelope)
          && whap_level2win(&weff->parameter.periodic.dwMagnitude,
                            effect->data.periodic.magnitude)
          && whap_phase2win(&weff->parameter.periodic.dwPhase,
                            effect->data.periodic.phase)
          && whap_time2win(&weff->parameter.periodic.dwPeriod,
                           effect->data.periodic.period)
          && whap_slevel2win(&weff->parameter.periodic.lOffset,
                             effect->data.periodic.offset);
}


/* Converts a periodic effect to directinput input API. */
static bool whap_rumble2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                            ALLEGRO_HAPTIC_EFFECT *effect)
{
   weff->effect.cbTypeSpecificParams = sizeof(weff->parameter.periodic);
   weff->effect.lpvTypeSpecificParams = &weff->parameter.periodic;

   return whap_level2win(&weff->parameter.periodic.dwMagnitude,
                         effect->data.rumble.strong_magnitude)
          && whap_phase2win(&weff->parameter.periodic.dwPhase, 0)
          && whap_time2win(&weff->parameter.periodic.dwPeriod, 0.01)
          && whap_slevel2win(&weff->parameter.periodic.lOffset, 0);
}

/* Converts Allegro haptic effect to dinput API. */
static bool whap_effect2win(ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff,
                            ALLEGRO_HAPTIC_EFFECT *effect,
                            ALLEGRO_HAPTIC_WINDOWS *whap)
{
   /* Generic setup */
   memset((void *)weff, 0, sizeof(*weff));
   /* Set global stuff. */
   weff->effect.dwSize = sizeof(DIEFFECT);
   weff->effect.dwGain = DI_FFNOMINALMAX;
   weff->effect.dwSamplePeriod = 0;
   weff->effect.dwFlags = DIEFF_OBJECTOFFSETS;
   weff->effect.lpEnvelope = NULL;
   /* Gain of the effect must be set to max, otherwise it won't be felt
      (enough) as the per effect gain multiplies with the per-device gain. */
   weff->effect.dwGain = DI_FFNOMINALMAX;
   /* This effect is not mapped to a trigger, and must be played explicitly. */
   weff->effect.dwTriggerButton = DIEB_NOTRIGGER;

   if (!whap_type2win(weff, effect)) {
      return false;
   }

   if (!whap_direction2win(weff, effect, whap)) {
      return false;
   }

   if (!whap_replay2win(weff, effect)) {
      return false;
   }


   switch (effect->type) {
      case ALLEGRO_HAPTIC_RUMBLE:
         return whap_rumble2win(weff, effect);
      case ALLEGRO_HAPTIC_PERIODIC:
         return whap_periodic2win(weff, effect);
      case ALLEGRO_HAPTIC_CONSTANT:
         return whap_constant2win(weff, effect);
      case ALLEGRO_HAPTIC_RAMP:
         return whap_ramp2win(weff, effect);
      case ALLEGRO_HAPTIC_SPRING:
      case ALLEGRO_HAPTIC_FRICTION:
      case ALLEGRO_HAPTIC_DAMPER:
      case ALLEGRO_HAPTIC_INERTIA:
         return whap_condition2win(weff, effect);
      default:
         return false;
   }
}

static bool whap_get_active(ALLEGRO_HAPTIC *haptic)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(haptic);
   return whap->active;
}


static bool whap_is_dinput_device_haptic(LPDIRECTINPUTDEVICE2 device)
{
   HRESULT ret;
   DIDEVCAPS dicaps;
   /* Get capabilities. */
   ALLEGRO_DEBUG("IDirectInputDevice_GetCapabilities on %p\n", device);
   dicaps.dwSize = sizeof(dicaps);
   ret = IDirectInputDevice_GetCapabilities(device, &dicaps);
   if (FAILED(ret)) {
      ALLEGRO_ERROR("IDirectInputDevice_GetCapabilities failed on %p\n",
                    device);
      return false;
   }
   /** Is it a haptic device? */
   bool ishaptic = (dicaps.dwFlags & DIDC_FORCEFEEDBACK);
   ALLEGRO_DEBUG("dicaps.dwFlags: %lu, %d, %d\n", dicaps.dwFlags,
                 DIDC_FORCEFEEDBACK, ishaptic);
   return(ishaptic);
}



static bool whap_is_mouse_haptic(ALLEGRO_MOUSE *mouse)
{
   (void)mouse;
   return false;
}


static bool whap_is_joystick_haptic(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_JOYSTICK_DIRECTX *joydx = (ALLEGRO_JOYSTICK_DIRECTX *)joy;
   (void)joydx;
   if (!al_is_joystick_installed())
      return false;
   if (!al_get_joystick_active(joy))
      return false;
   ALLEGRO_DEBUG("Checking capabilities of joystick %s\n", joydx->name);
   return whap_is_dinput_device_haptic(joydx->device);
}


static bool whap_is_display_haptic(ALLEGRO_DISPLAY *dev)
{
   (void)dev;
   return false;
}


static bool whap_is_keyboard_haptic(ALLEGRO_KEYBOARD *dev)
{
   (void)dev;
   return false;
}


static bool whap_is_touch_input_haptic(ALLEGRO_TOUCH_INPUT *dev)
{
   (void)dev;
   return false;
}


static ALLEGRO_HAPTIC *whap_get_from_mouse(ALLEGRO_MOUSE *mouse)
{
   (void)mouse;
   return NULL;
}


/* Sets the force feedback gain on a directinput device.
   Returns true on success and false on failure.  */
static bool whap_set_dinput_device_gain(LPDIRECTINPUTDEVICE2 device,
                                        double gain)
{
   HRESULT ret;
   DIPROPDWORD dipdw;
   if (gain < 0.0)
      return false;
   if (gain > 1.0)
      return false;

   dipdw.diph.dwSize = sizeof(DIPROPDWORD);
   dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
   dipdw.diph.dwObj = 0;
   dipdw.diph.dwHow = DIPH_DEVICE;
   dipdw.dwData = gain * DI_FFNOMINALMAX;

   ret = IDirectInputDevice_SetProperty(device, DIPROP_FFGAIN, &dipdw.diph);
   return(!FAILED(ret));
}

/* Sets the force feedback autocentering intensity on a directinput device.
   Returns true on success and false on failure. */
static bool whap_set_dinput_device_autocenter(LPDIRECTINPUTDEVICE2 device,
                                              double intensity)
{
   HRESULT ret;
   DIPROPDWORD dipdw;
   if (intensity < 0.0)
      return false;
   if (intensity > 1.0)
      return false;

   dipdw.diph.dwSize = sizeof(DIPROPDWORD);
   dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
   dipdw.diph.dwObj = 0;
   dipdw.diph.dwHow = DIPH_DEVICE;
   if (intensity < 0.5) {
      dipdw.dwData = DIPROPAUTOCENTER_OFF;
   }
   else {
      dipdw.dwData = DIPROPAUTOCENTER_ON;
   }
   /* Try to set the autocenter. */
   ret = IDirectInputDevice_SetProperty(device, DIPROP_AUTOCENTER, &dipdw.diph);
   return(!FAILED(ret));
}


/* Callback to check which effect types are supported. */
static BOOL CALLBACK
whap_check_effect_callback(LPCDIEFFECTINFO info, LPVOID data)
{
   ALLEGRO_HAPTIC *haptic = (ALLEGRO_HAPTIC *)data;
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(haptic);

   const CAP_MAP *map;
   for (map = cap_map; map->allegro_bit != -1; map++) {
      if (GUID_EQUAL(info->guid, map->guid)) {
         whap->flags |= map->allegro_bit;
      }
   }
   /* Check for more supported effect types. */
   return DIENUM_CONTINUE;
}


/* Callback to check which axes are supported. */
static BOOL CALLBACK
whap_check_axes_callback(LPCDIDEVICEOBJECTINSTANCE dev, LPVOID data)
{
   ALLEGRO_HAPTIC *haptic = (ALLEGRO_HAPTIC *)data;
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(haptic);

   if ((dev->dwType & DIDFT_AXIS) && (dev->dwFlags & DIDOI_FFACTUATOR)) {
      whap->axes[whap->naxes] = dev->dwOfs;
      whap->naxes++;

      /* Stop if the axes limit is reached */
      if (whap->naxes >= HAPTICS_AXES_MAX) {
         return DIENUM_STOP;
      }
   }
   return DIENUM_CONTINUE;
}

/* Acquires an exclusive lock on the device. */
static bool whap_acquire_lock(ALLEGRO_HAPTIC_WINDOWS *whap)
{
   HRESULT ret;

   /* Release previous acquire lock on device if any */
   ret = IDirectInputDevice_Unacquire(whap->device);
   if (FAILED(ret)) {
      ALLEGRO_WARN
         ("IDirectInputDevice_Unacquire failed for haptic device.\n");
      return false;
   }

   /* Need a display to lock the cooperative level of the haptic device on */
   whap->display = (ALLEGRO_DISPLAY_WIN *)al_get_current_display();
   if (!whap->display) {
      ALLEGRO_WARN("No active window available to lock the haptic device on.");
      return false;
   }

   /* Must set the cooperative level to exclusive now to enable force feedback.
    */
   ret =
      IDirectInputDevice_SetCooperativeLevel(whap->device,
                                             whap->display->window,
                                             DISCL_BACKGROUND |
                                             DISCL_EXCLUSIVE);
   if (FAILED(ret)) {
      ALLEGRO_WARN
         ("IDirectInputDevice_SetCooperativeLevel failed for haptic device.\n");
      return false;
   }

   /* Get acquire lock on device */
   ret = IDirectInputDevice_Acquire(whap->device);
   if (FAILED(ret)) {
      ALLEGRO_WARN("IDirectInputDevice_Acquire failed for haptic device.\n");
      return false;
   }
   return true;
}


/* Initializes the haptic device for use with DirectInput */
static bool whap_initialize_dinput(ALLEGRO_HAPTIC_WINDOWS *whap)
{
   HRESULT ret;
   ALLEGRO_HAPTIC *haptic = &whap->parent;
   /* Set number of axes to zero, and then ... */
   whap->naxes = 0;
   /* ... get number of axes. */
   ret = IDirectInputDevice_EnumObjects(whap->device,
                                        whap_check_axes_callback,
                                        haptic, DIDFT_AXIS);
   if (FAILED(ret)) {
      ALLEGRO_WARN("Could not get haptic device axes \n");
      return false;
   }

   /* Support angle and radius if we have at least 2 axes.
    * Axis support on DirectInput is a big mess, so Azimuth is unlikely to be
    * supported.
    */
   if (whap->naxes >= 1) {
      whap->flags |= ALLEGRO_HAPTIC_ANGLE;
      whap->flags |= ALLEGRO_HAPTIC_RADIUS;
   }

   if (!whap_acquire_lock(whap)) {
      ALLEGRO_WARN("Could not lock haptic device \n");
      return false;
   }
   /* Reset all actuators in case some where active */
   ret = IDirectInputDevice8_SendForceFeedbackCommand(whap->device,
                                                      DISFFC_RESET);
   if (FAILED(ret)) {
      ALLEGRO_WARN("Could not reset haptic device \n");
   }

   /* Enable all actuators. */
   ret = IDirectInputDevice8_SendForceFeedbackCommand(whap->device,
                                                      DISFFC_SETACTUATORSON);
   if (FAILED(ret)) {
      ALLEGRO_WARN("Could not enable haptic device actuators\n");
      return false;
   }

   /* Get known supported effects. */
   ret = IDirectInputDevice8_EnumEffects(whap->device,
                                         whap_check_effect_callback, haptic,
                                         DIEFT_ALL);
   if (FAILED(ret)) {
      ALLEGRO_WARN("Could not get haptic device supported effects\n");
      return false;
   }

   /* Check if any periodic effects are supported. */
   bool periodic_ok = al_is_haptic_capable(haptic, ALLEGRO_HAPTIC_SINE);
   periodic_ok |= al_is_haptic_capable(haptic, ALLEGRO_HAPTIC_SQUARE);
   periodic_ok |= al_is_haptic_capable(haptic, ALLEGRO_HAPTIC_TRIANGLE);
   periodic_ok |= al_is_haptic_capable(haptic, ALLEGRO_HAPTIC_SAW_DOWN);
   periodic_ok |= al_is_haptic_capable(haptic, ALLEGRO_HAPTIC_SAW_UP);

   if (periodic_ok) {
      /* If we have any of the effects above, we can use
         periodic and rumble effects. */
      whap->flags |= (ALLEGRO_HAPTIC_PERIODIC | ALLEGRO_HAPTIC_RUMBLE);
   }

   if (whap_set_dinput_device_gain(whap->device, 1.0)) {
      whap->flags |= ALLEGRO_HAPTIC_GAIN;
   }

   /* Check autocenter and turn it off in one go. */
   if (whap_set_dinput_device_autocenter(whap->device, 0.0)) {
      whap->flags |= ALLEGRO_HAPTIC_AUTOCENTER;
   }

   return true;
}




static ALLEGRO_HAPTIC *whap_get_from_joystick(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_JOYSTICK_DIRECTX *joydx = (ALLEGRO_JOYSTICK_DIRECTX *)joy;
   ALLEGRO_HAPTIC_WINDOWS *whap;

   int i;

   if (!al_is_joystick_haptic(joy))
      return NULL;

   al_lock_mutex(haptic_mutex);

   whap = whap_get_available_haptic();

   if (!whap) {
      al_unlock_mutex(haptic_mutex);
      return NULL;
   }

   whap->parent.driver = &_al_hapdrv_directx;
   whap->parent.device = joy;
   whap->parent.from = _AL_HAPTIC_FROM_JOYSTICK;

   whap->guid = joydx->guid;
   whap->device = joydx->device;
   whap->active = true;
   for (i = 0; i < HAPTICS_EFFECTS_MAX; i++) {
      whap->effects[i].active = false;  /* not in use */
   }
   whap->parent.gain = 1.0;
   whap->parent.autocenter = 0.0;

   /* result is ok if init functions returns true. */
   if (!whap_initialize_dinput(whap)) {
      al_release_haptic(&whap->parent);
      al_unlock_mutex(haptic_mutex);
      return NULL;
   }

   al_unlock_mutex(haptic_mutex);

   return &whap->parent;
}


static ALLEGRO_HAPTIC *whap_get_from_display(ALLEGRO_DISPLAY *dev)
{
   (void)dev;
   return NULL;
}


static ALLEGRO_HAPTIC *whap_get_from_keyboard(ALLEGRO_KEYBOARD *dev)
{
   (void)dev;
   return NULL;
}


static ALLEGRO_HAPTIC *whap_get_from_touch_input(ALLEGRO_TOUCH_INPUT *dev)
{
   (void)dev;
   return NULL;
}


static int whap_get_capabilities(ALLEGRO_HAPTIC *dev)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(dev);
   return whap->flags;
}


static double whap_get_gain(ALLEGRO_HAPTIC *dev)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(dev);
   /* Just return the stored gain, it's easier than querying. */
   return whap->parent.gain;
}


static bool whap_set_gain(ALLEGRO_HAPTIC *dev, double gain)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(dev);
   bool ok = whap_set_dinput_device_gain(whap->device, gain);
   if (ok) {
      whap->parent.gain = gain;
   }
   else {
      whap->parent.gain = 1.0;
   }
   return ok;
}


double whap_get_autocenter(ALLEGRO_HAPTIC *dev)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(dev);
   /* Return the stored autocenter value. It's easiest like that. */
   return whap->parent.autocenter;
}


static bool whap_set_autocenter(ALLEGRO_HAPTIC *dev, double intensity)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(dev);
   bool ok = whap_set_dinput_device_autocenter(whap->device, intensity);
   if (ok) {
      whap->parent.autocenter = intensity;
   }
   else {
      whap->parent.autocenter = 0.0;
   }
   return ok;
}

static int whap_get_max_effects(ALLEGRO_HAPTIC *dev)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(dev);
   int n_effects;
   (void)n_effects, (void)whap;

   return HAPTICS_EFFECTS_MAX;
}


static bool whap_is_effect_ok(ALLEGRO_HAPTIC *haptic,
                              ALLEGRO_HAPTIC_EFFECT *effect)
{
   int caps;

   caps = al_get_haptic_capabilities(haptic);
   if (caps & effect->type) {
      return true;
   }
   /* XXX: should do more checking here? */
   return false;
}


struct dinput_error_pair
{
   HRESULT error;
   const char *text;
};

#define DIMKEP(ERROR) { ((HRESULT)ERROR), # ERROR }

struct dinput_error_pair dinput_errors[] = {
   DIMKEP(DI_BUFFEROVERFLOW),
   DIMKEP(DI_DOWNLOADSKIPPED),
   DIMKEP(DI_EFFECTRESTARTED),
   DIMKEP(DI_NOEFFECT),
   DIMKEP(DI_NOTATTACHED),
   DIMKEP(DI_OK),
   DIMKEP(DI_POLLEDDEVICE),
   DIMKEP(DI_PROPNOEFFECT),
   DIMKEP(DI_SETTINGSNOTSAVED),
   DIMKEP(DI_TRUNCATED),
   DIMKEP(DI_TRUNCATEDANDRESTARTED),
   DIMKEP(DI_WRITEPROTECT),
   DIMKEP(DIERR_ACQUIRED),
   DIMKEP(DIERR_ALREADYINITIALIZED),
   DIMKEP(DIERR_BADDRIVERVER),
   DIMKEP(DIERR_BETADIRECTINPUTVERSION),
   DIMKEP(DIERR_DEVICEFULL),
   DIMKEP(DIERR_DEVICENOTREG),
   DIMKEP(DIERR_EFFECTPLAYING),
   DIMKEP(DIERR_GENERIC),
   DIMKEP(DIERR_HANDLEEXISTS),
   DIMKEP(DIERR_HASEFFECTS),
   DIMKEP(DIERR_INCOMPLETEEFFECT),
   DIMKEP(DIERR_INPUTLOST),
   DIMKEP(DIERR_INVALIDPARAM),
   DIMKEP(DIERR_MAPFILEFAIL),
   DIMKEP(DIERR_MOREDATA),
   DIMKEP(DIERR_NOAGGREGATION),
   DIMKEP(DIERR_NOINTERFACE),
   DIMKEP(DIERR_NOTACQUIRED),
   DIMKEP(DIERR_NOTBUFFERED),
   DIMKEP(DIERR_NOTDOWNLOADED),
   DIMKEP(DIERR_NOTEXCLUSIVEACQUIRED),
   DIMKEP(DIERR_NOTFOUND),
   DIMKEP(DIERR_NOTINITIALIZED),
   DIMKEP(DIERR_OBJECTNOTFOUND),
   DIMKEP(DIERR_OLDDIRECTINPUTVERSION),
   DIMKEP(DIERR_OTHERAPPHASPRIO),
   DIMKEP(DIERR_OUTOFMEMORY),
   DIMKEP(DIERR_READONLY),
   DIMKEP(DIERR_REPORTFULL),
   DIMKEP(DIERR_UNPLUGGED),
   DIMKEP(DIERR_UNSUPPORTED),
   DIMKEP(E_HANDLE),
   DIMKEP(E_PENDING),
   DIMKEP(E_POINTER),
   { 0, NULL }
};

static void warn_on_error(HRESULT hr)
{
   struct dinput_error_pair *pair = dinput_errors;
   while (pair->text) {
      if (hr == pair->error) {
         ALLEGRO_WARN("HRESULT error: %s\n", pair->text);
      }
      pair++;
   }
   ALLEGRO_WARN("Unknown HRESULT error: %u\n", (unsigned int)hr);
}



static bool whap_upload_effect_helper
   (ALLEGRO_HAPTIC_WINDOWS *whap,
   ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff, ALLEGRO_HAPTIC_EFFECT *effect)
{
   HRESULT ret;

   if (!whap_effect2win(weff, effect, whap)) {
      ALLEGRO_WARN("Could not convert haptic effect.\n");
      return false;
   }

   /* Create the effect. */
   ret = IDirectInputDevice8_CreateEffect(whap->device, (*weff->guid),
                                          &weff->effect,
                                          &weff->ref, NULL);

   /* XXX Need to re-lock since the joystick driver steals my thunder
    * by calling Unacquire on the device.
    * The better way would be to fix this in that driver somehow.
    */
   if (!whap_acquire_lock(whap)) {
      ALLEGRO_WARN("Could not lock haptic device.\n");
      return false;
   }


   /* Create the effect. */
   ret = IDirectInputDevice8_CreateEffect(whap->device, (*weff->guid),
                                          &weff->effect, &weff->ref, NULL);

   if (FAILED(ret)) {
      ALLEGRO_WARN("Could not create haptic effect.\n");
      warn_on_error(ret);
      return false;
   }

   /* Upload the effect to the device. */
   ret = IDirectInputEffect_Download(weff->ref);
   if (FAILED(ret)) {
      ALLEGRO_WARN("Could not upload haptic effect.\n");
      warn_on_error(ret);
      return false;
   }

   return true;
}

static bool whap_upload_effect(ALLEGRO_HAPTIC *dev,
                               ALLEGRO_HAPTIC_EFFECT *effect,
                               ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   bool ok = FALSE;
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(dev);
   ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff = NULL;

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

   al_lock_mutex(haptic_mutex);

   /* Look for a free haptic effect slot. */
   weff = whap_get_available_effect(whap);
   /* Returns NULL if there is no more space for an effect. */
   if (weff) {
      if (whap_upload_effect_helper(whap, weff, effect)) {
         /* set ID handle to signify success */
         id->_haptic = dev;
         id->_pointer = weff;
         id->_id = weff->id;
         id->_effect_duration = al_get_haptic_effect_duration(effect);
         ok = true;
      }
      else {
         ALLEGRO_WARN("Could not upload effect.");
      }
   }
   else {
      ALLEGRO_WARN("No free effect slot.");
   }

   al_unlock_mutex(haptic_mutex);
   return ok;
}


static bool whap_play_effect(ALLEGRO_HAPTIC_EFFECT_ID *id, int loops)
{
   HRESULT res;
   ALLEGRO_HAPTIC_WINDOWS *whap = (ALLEGRO_HAPTIC_WINDOWS *)id->_haptic;
   ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff;
   if ((!whap) || (id->_id < 0))
      return false;
   weff = whap->effects + id->_id;

   /* Need to re-lock since the joystick driver steals the haptics' thunder
    * by calling Unacquire on the device.
    * The better way would be to fix this in that driver somehow.
    */
   if (!whap_acquire_lock(whap)) {
      ALLEGRO_WARN("Could not lock haptic device \n");
      return false;
   }

   res = IDirectInputEffect_Start(weff->ref, loops, 0);
   if (FAILED(res)) {
      ALLEGRO_WARN("Failed to play an effect.");
      warn_on_error(res);
      return false;
   }
   id->_playing = true;
   id->_start_time = al_get_time();
   id->_end_time = id->_start_time;
   id->_end_time += id->_effect_duration * (double)loops;
   return true;
}


static bool whap_stop_effect(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   HRESULT res;
   ALLEGRO_HAPTIC_WINDOWS *whap = (ALLEGRO_HAPTIC_WINDOWS *)id->_haptic;
   ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff;

   if ((!whap) || (id->_id < 0))
      return false;

   weff = whap->effects + id->_id;

   res = IDirectInputEffect_Stop(weff->ref);
   if (FAILED(res)) {
      ALLEGRO_WARN("Failed to play an effect.");
      return false;
   }
   id->_playing = false;


   return true;
}


static bool whap_is_effect_playing(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ASSERT(id);
   HRESULT res;
   DWORD flags = 0;
   ALLEGRO_HAPTIC_WINDOWS *whap = (ALLEGRO_HAPTIC_WINDOWS *)id->_haptic;
   ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff;

   if ((!whap) || (id->_id < 0) || (!id->_playing))
      return false;

   weff = whap->effects + id->_id;

   res = IDirectInputEffect_GetEffectStatus(weff->ref, &flags);
   if (FAILED(res)) {
      ALLEGRO_WARN("Failed to get the status of effect.");
      /* If we get here, then use the play time in stead to
       * see if the effect should still be playing.
       * Do this because in case GeteffectStatus fails, we can't
       * assume the sample isn't playing. In fact, if the play command
       * was successful, it should still be playing as long as the play
       * time has not passed.
       */
      return(al_get_time() < id->_end_time);
   }
   if (flags & DIEGES_PLAYING)
      return true;
   /* WINE is bugged here, it doesn't set flags, but it also
    * just returns DI_OK. Thats why here, don't believe the API
    * when it the playing flag isn't set if the effect's duration
    * has not passed. On real Windows it should probably always be the
    * case that the effect will have played completely when
    * the play time has ended.
    */
   return(al_get_time() < id->_end_time);
}



static bool whap_release_effect(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = (ALLEGRO_HAPTIC_WINDOWS *)id->_haptic;
   ALLEGRO_HAPTIC_EFFECT_WINDOWS *weff;
   if ((!whap) || (id->_id < 0))
      return false;

   whap_stop_effect(id);

   weff = whap->effects + id->_id;
   return whap_release_effect_windows(weff);
}


static bool whap_release(ALLEGRO_HAPTIC *haptic)
{
   ALLEGRO_HAPTIC_WINDOWS *whap = whap_from_al(haptic);
   int index;
   HRESULT res;

   ASSERT(haptic);

   if (!whap->active)
      return false;

   /* Release all effects for this device. */
   for (index = 0; index < HAPTICS_EFFECTS_MAX; index++) {
      whap_release_effect_windows(whap->effects + index);
   }

   /* Release the acquire lock on the device */
   IDirectInputDevice_Unacquire(whap->device);

   /* Reset the cooperative level to nonexclusive.
    */
   res =
      IDirectInputDevice_SetCooperativeLevel(whap->device,
                                             whap->display->window,
                                             DISCL_FOREGROUND |
                                             DISCL_NONEXCLUSIVE);
   if (FAILED(res)) {
      ALLEGRO_WARN
         ("IDirectInputDevice8_SetCooperativeLevel NONEXCLUSIVE failed for haptic device.\n");
   }

   whap->display = NULL;
   whap->active = false;
   whap->device = NULL;
   return true;
}



/* vim: set sts=3 sw=3 et: */
