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
 *      Linux haptic (force-feedback) device driver.
 *
 *      By Beoran.
 *
 *      See LICENSE.txt for copyright information.
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_haptic.h"
#include "allegro5/internal/aintern_ljoynu.h"
#include "allegro5/platform/aintunix.h"

#ifdef A5O_HAVE_LINUX_INPUT_H

#include <linux/input.h>

A5O_DEBUG_CHANNEL("lhaptic")


/* For compatibility with older kernels. */
#ifndef input_event_sec
#define input_event_sec time.tv_sec
#define input_event_usec time.tv_usec
#endif


/* Support at most 32 haptic devices. */
#define HAPTICS_MAX             32

/* Support at most 16 effects per device. */
#define HAPTICS_EFFECTS_MAX     16


typedef struct
{
   struct A5O_HAPTIC parent; /* must be first */
   bool active;
   int fd;
   int flags;
   int effects[HAPTICS_EFFECTS_MAX];
} A5O_HAPTIC_LINUX;


#define LONG_BITS    (sizeof(long) * 8)
#define NLONGS(x)    (((x) + LONG_BITS - 1) / LONG_BITS)
/* Tests if a bit in an array of longs is set. */
#define TEST_BIT(nr, addr) \
   ((1UL << ((nr) % LONG_BITS)) & (addr)[(nr) / LONG_BITS])


/* forward declarations */
static bool lhap_init_haptic(void);
static void lhap_exit_haptic(void);

static bool lhap_is_mouse_haptic(A5O_MOUSE *dev);
static bool lhap_is_joystick_haptic(A5O_JOYSTICK *);
static bool lhap_is_keyboard_haptic(A5O_KEYBOARD *dev);
static bool lhap_is_display_haptic(A5O_DISPLAY *dev);
static bool lhap_is_touch_input_haptic(A5O_TOUCH_INPUT *dev);

static A5O_HAPTIC *lhap_get_from_mouse(A5O_MOUSE *dev);
static A5O_HAPTIC *lhap_get_from_joystick(A5O_JOYSTICK *dev);
static A5O_HAPTIC *lhap_get_from_keyboard(A5O_KEYBOARD *dev);
static A5O_HAPTIC *lhap_get_from_display(A5O_DISPLAY *dev);
static A5O_HAPTIC *lhap_get_from_touch_input(A5O_TOUCH_INPUT *dev);

static bool lhap_release(A5O_HAPTIC *haptic);

static bool lhap_get_active(A5O_HAPTIC *hap);
static int lhap_get_capabilities(A5O_HAPTIC *dev);
static double lhap_get_gain(A5O_HAPTIC *dev);
static bool lhap_set_gain(A5O_HAPTIC *dev, double);
static int lhap_get_max_effects(A5O_HAPTIC *dev);

static bool lhap_is_effect_ok(A5O_HAPTIC *dev, A5O_HAPTIC_EFFECT *eff);
static bool lhap_upload_effect(A5O_HAPTIC *dev,
                               A5O_HAPTIC_EFFECT *eff,
                               A5O_HAPTIC_EFFECT_ID *id);
static bool lhap_play_effect(A5O_HAPTIC_EFFECT_ID *id, int loop);
static bool lhap_stop_effect(A5O_HAPTIC_EFFECT_ID *id);
static bool lhap_is_effect_playing(A5O_HAPTIC_EFFECT_ID *id);
static bool lhap_release_effect(A5O_HAPTIC_EFFECT_ID *id);

static double lhap_get_autocenter(A5O_HAPTIC *dev);
static bool lhap_set_autocenter(A5O_HAPTIC *dev, double);

static void lhap_timerclear(struct input_event *evt);

A5O_HAPTIC_DRIVER _al_hapdrv_linux =
{
   _A5O_HAPDRV_LINUX,
   "",
   "",
   "Linux haptic(s)",
   lhap_init_haptic,
   lhap_exit_haptic,

   lhap_is_mouse_haptic,
   lhap_is_joystick_haptic,
   lhap_is_keyboard_haptic,
   lhap_is_display_haptic,
   lhap_is_touch_input_haptic,

   lhap_get_from_mouse,
   lhap_get_from_joystick,
   lhap_get_from_keyboard,
   lhap_get_from_display,
   lhap_get_from_touch_input,

   lhap_get_active,
   lhap_get_capabilities,
   lhap_get_gain,
   lhap_set_gain,
   lhap_get_max_effects,

   lhap_is_effect_ok,
   lhap_upload_effect,
   lhap_play_effect,
   lhap_stop_effect,
   lhap_is_effect_playing,
   lhap_release_effect,

   lhap_release,

   lhap_get_autocenter,
   lhap_set_autocenter
};


static A5O_HAPTIC_LINUX haptics[HAPTICS_MAX];
static A5O_MUTEX *haptic_mutex = NULL;


struct CAP_MAP {
   int linux_bit;
   int allegro_bit;
};

static const struct CAP_MAP cap_map[] = {
   { FF_PERIODIC, A5O_HAPTIC_PERIODIC },
   { FF_RUMBLE,   A5O_HAPTIC_RUMBLE },
   { FF_CONSTANT, A5O_HAPTIC_CONSTANT },
   { FF_SPRING,   A5O_HAPTIC_SPRING },
   { FF_FRICTION, A5O_HAPTIC_FRICTION },
   { FF_DAMPER,   A5O_HAPTIC_DAMPER },
   { FF_INERTIA,  A5O_HAPTIC_INERTIA },
   { FF_RAMP,     A5O_HAPTIC_RAMP },
   { FF_SQUARE,   A5O_HAPTIC_SQUARE },
   { FF_TRIANGLE, A5O_HAPTIC_TRIANGLE },
   { FF_SINE,     A5O_HAPTIC_SINE },
   { FF_SAW_UP,   A5O_HAPTIC_SAW_UP },
   { FF_SAW_DOWN, A5O_HAPTIC_SAW_DOWN },
   { FF_CUSTOM,   A5O_HAPTIC_CUSTOM },
   { FF_GAIN,     A5O_HAPTIC_GAIN },
   { FF_AUTOCENTER, A5O_HAPTIC_AUTOCENTER },
   { -1,          -1 }
};


static bool lhap_init_haptic(void)
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


static A5O_HAPTIC_LINUX *lhap_get_available_haptic(void)
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


/* Converts a generic haptic device to a Linux-specific one. */
static A5O_HAPTIC_LINUX *lhap_from_al(A5O_HAPTIC *hap)
{
   return (A5O_HAPTIC_LINUX *)hap;
}


static void lhap_exit_haptic(void)
{
   ASSERT(haptic_mutex);
   al_destroy_mutex(haptic_mutex);
   haptic_mutex = NULL;
}


static bool lhap_type2lin(__u16 *res, int type)
{
   ASSERT(res);

   switch (type) {
      case A5O_HAPTIC_RUMBLE:
         (*res) = FF_RUMBLE;
         break;
      case A5O_HAPTIC_PERIODIC:
         (*res) = FF_PERIODIC;
         break;
      case A5O_HAPTIC_CONSTANT:
         (*res) = FF_CONSTANT;
         break;
      case A5O_HAPTIC_SPRING:
         (*res) = FF_SPRING;
         break;
      case A5O_HAPTIC_FRICTION:
         (*res) = FF_FRICTION;
         break;
      case A5O_HAPTIC_DAMPER:
         (*res) = FF_DAMPER;
         break;
      case A5O_HAPTIC_INERTIA:
         (*res) = FF_INERTIA;
         break;
      case A5O_HAPTIC_RAMP:
         (*res) = FF_RAMP;
         break;
      default:
         return false;
   }
   return true;
}


static bool lhap_wave2lin(__u16 *res, int type)
{
   ASSERT(res);

   switch (type) {
      case A5O_HAPTIC_SQUARE:
         (*res) = FF_SQUARE;
         break;
      case A5O_HAPTIC_TRIANGLE:
         (*res) = FF_TRIANGLE;
         break;
      case A5O_HAPTIC_SINE:
         (*res) = FF_SINE;
         break;
      case A5O_HAPTIC_SAW_UP:
         (*res) = FF_SAW_UP;
         break;
      case A5O_HAPTIC_SAW_DOWN:
         (*res) = FF_SAW_DOWN;
         break;
      case A5O_HAPTIC_CUSTOM:
         (*res) = FF_CUSTOM;
         break;
      default:
         return false;
   }
   return true;
}


/* Converts the time in seconds to a Linux-compatible time.
 * Return false if out of bounds.
 */
static bool lhap_time2lin(__u16 *res, double sec)
{
   ASSERT(res);

   if (sec < 0.0 || sec > 32.767)
      return false;
   (*res) = (__u16) round(sec * 1000.0);
   return true;
}


/* Converts the time in seconds to a Linux-compatible time.
 * Return false if out of bounds. This one allows negative times.
 */
static bool lhap_stime2lin(__s16 *res, double sec)
{
   ASSERT(res);

   if (sec < -32.767 || sec > 32.767)
      return false;
   (*res) = (__s16) round(sec * 1000.0);
   return true;
}


/* Converts replay data to Linux-compatible data. */
static bool lhap_replay2lin(struct ff_replay *lin,
   struct A5O_HAPTIC_REPLAY *al)
{
   return lhap_time2lin(&lin->delay, al->delay)
      && lhap_time2lin(&lin->length, al->length);
}


/* Converts the level in range 0.0 to 1.0 to a Linux-compatible level.
 * Returns false if out of bounds.
 */
static bool lhap_level2lin(__u16 *res, double level)
{
   ASSERT(res);

   if (level < 0.0 || level > 1.0)
      return false;
   *res = (__u16) round(level * (double)0x7fff);
   return true;
}


/* Converts the level in range -1.0 to 1.0 to a Linux-compatible level.
 * Returns false if out of bounds.
 */
static bool lhap_slevel2lin(__s16 *res, double level)
{
   ASSERT(res);

   if (level < -1.0 || level > 1.0)
      return false;
   *res = (__s16) round(level * (double)0x7ffe);
   return true;
}


/* Converts an Allegro haptic effect envelope to Linux input API. */
static bool lhap_envelope2lin(struct ff_envelope *lin,
   struct A5O_HAPTIC_ENVELOPE *al)
{
   return lhap_time2lin(&lin->attack_length, al->attack_length)
      && lhap_time2lin(&lin->fade_length, al->fade_length)
      && lhap_level2lin(&lin->attack_level, al->attack_level)
      && lhap_level2lin(&lin->fade_level, al->fade_level);
}


/* Converts a rumble effect to Linux input API. */
static bool lhap_rumble2lin(struct ff_rumble_effect *lin,
   struct A5O_HAPTIC_RUMBLE_EFFECT *al)
{
   return lhap_level2lin(&lin->strong_magnitude, al->strong_magnitude)
      && lhap_level2lin(&lin->weak_magnitude, al->weak_magnitude);
}


/* Converts a constant effect to Linux input API. */
static bool lhap_constant2lin(struct ff_constant_effect *lin,
   struct A5O_HAPTIC_CONSTANT_EFFECT *al)
{
   return lhap_envelope2lin(&lin->envelope, &al->envelope)
      && lhap_slevel2lin(&lin->level, al->level);
}


/* Converts a ramp effect to Linux input API. */
static bool lhap_ramp2lin(struct ff_ramp_effect *lin,
   struct A5O_HAPTIC_RAMP_EFFECT *al)
{
   return lhap_envelope2lin(&lin->envelope, &al->envelope)
      && lhap_slevel2lin(&lin->start_level, al->start_level)
      && lhap_slevel2lin(&lin->end_level, al->end_level);
}


/* Converts a ramp effect to Linux input API. */
static bool lhap_condition2lin(struct ff_condition_effect *lin,
   struct A5O_HAPTIC_CONDITION_EFFECT *al)
{
   return lhap_slevel2lin(&lin->center, al->center)
      && lhap_level2lin(&lin->deadband, al->deadband)
      && lhap_slevel2lin(&lin->right_coeff, al->right_coeff)
      && lhap_level2lin(&lin->right_saturation, al->right_saturation)
      && lhap_slevel2lin(&lin->left_coeff, al->left_coeff)
      && lhap_level2lin(&lin->left_saturation, al->left_saturation);
}


/* Converts a periodic effect to linux input API. */
static bool lhap_periodic2lin(struct ff_periodic_effect *lin,
   struct A5O_HAPTIC_PERIODIC_EFFECT *al)
{
   /* Custom data is not supported yet, because currently no Linux
    * haptic driver supports it.
    */
   if (al->custom_data)
      return false;

   return lhap_slevel2lin(&lin->magnitude, al->magnitude)
      && lhap_stime2lin(&lin->offset, al->offset)
      && lhap_time2lin(&lin->period, al->period)
      && lhap_time2lin(&lin->phase, al->phase)
      && lhap_wave2lin(&lin->waveform, al->waveform)
      && lhap_envelope2lin(&lin->envelope, &al->envelope);
}


/* Converts Allegro haptic effect to Linux input API. */
static bool lhap_effect2lin(struct ff_effect *lin, A5O_HAPTIC_EFFECT *al)
{
   memset(lin, 0, sizeof(*lin));

   if (!lhap_type2lin(&lin->type, al->type))
      return false;
   /* lin_effect->replay = effect->re; */
   lin->direction = (__u16)
      round(((double)0xC000 * al->direction.angle) / (2 * M_PI));
   lin->id = -1;
   if (!lhap_replay2lin(&lin->replay, &al->replay))
      return false;
   switch (lin->type) {
      case FF_RUMBLE:
         return lhap_rumble2lin(&lin->u.rumble, &al->data.rumble);
      case FF_PERIODIC:
         return lhap_periodic2lin(&lin->u.periodic, &al->data.periodic);
      case FF_CONSTANT:
         return lhap_constant2lin(&lin->u.constant, &al->data.constant);
      case FF_RAMP:
         return lhap_ramp2lin(&lin->u.ramp, &al->data.ramp);
      case FF_SPRING:   /* fall through */
      case FF_FRICTION: /* fall through */
      case FF_DAMPER:   /* fall through */
      case FF_INERTIA:
         return lhap_condition2lin(&lin->u.condition[0], &al->data.condition);
      default:
         return false;
   }
}


static bool lhap_get_active(A5O_HAPTIC *haptic)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(haptic);
   return lhap->active;
}


static bool lhap_is_mouse_haptic(A5O_MOUSE *mouse)
{
   (void)mouse;
   return false;
}


static bool lhap_fd_can_ff(int fd)
{
   long bitmask[NLONGS(EV_CNT)] = { 0 };

   if (ioctl(fd, EVIOCGBIT(0, sizeof(bitmask)), bitmask) < 0) {
      return false;
   }
   if (TEST_BIT(EV_FF, bitmask)) {
      return true;
   }
   return false;
}


static bool lhap_is_joystick_haptic(A5O_JOYSTICK *joy)
{
   A5O_JOYSTICK_LINUX *ljoy = (A5O_JOYSTICK_LINUX *) joy;
   if (!al_is_joystick_installed())
      return false;
   if (!al_get_joystick_active(joy))
      return false;
   if (ljoy->fd <= 0)
      return false;
   return lhap_fd_can_ff(ljoy->fd);
}


static bool lhap_is_display_haptic(A5O_DISPLAY *dev)
{
   (void)dev;
   return false;
}


static bool lhap_is_keyboard_haptic(A5O_KEYBOARD *dev)
{
   (void)dev;
   return false;
}


static bool lhap_is_touch_input_haptic(A5O_TOUCH_INPUT *dev)
{
   (void)dev;
   return false;
}


static A5O_HAPTIC *lhap_get_from_mouse(A5O_MOUSE *mouse)
{
   (void)mouse;
   return NULL;
}


static bool get_haptic_capabilities(int fd, int *capabilities)
{
   unsigned long bitmask[NLONGS(FF_CNT)] = { 0 };
   int caps;
   int i;

   if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof(bitmask)), bitmask) < 0) {
      A5O_ERROR("EVIOCGBIT failed for fd %d", fd);
      return false;
   }

   caps = 0;
   for (i = 0; cap_map[i].allegro_bit >= 0; i++) {
      if (TEST_BIT(cap_map[i].linux_bit, bitmask)) {
         caps |= cap_map[i].allegro_bit;
      }
   }
   (*capabilities) = caps;
   A5O_INFO("Capabilities: 0x%x\n", caps);
   return true;
}


static A5O_HAPTIC *lhap_get_from_joystick(A5O_JOYSTICK *joy)
{
   A5O_JOYSTICK_LINUX *ljoy = (A5O_JOYSTICK_LINUX *) joy;
   A5O_HAPTIC_LINUX *lhap;
   int i;

   if (!al_is_joystick_haptic(joy))
      return NULL;

   al_lock_mutex(haptic_mutex);

   lhap = lhap_get_available_haptic();

   if (!lhap) {
      al_unlock_mutex(haptic_mutex);
      return NULL;
   }

   lhap->parent.device = joy;
   lhap->parent.from = _AL_HAPTIC_FROM_JOYSTICK;

   lhap->fd = ljoy->fd;
   lhap->active = true;
   for (i = 0; i < HAPTICS_EFFECTS_MAX; i++) {
      lhap->effects[i] = -1; /* not in use */
   }
   lhap->parent.gain = 1.0;
   get_haptic_capabilities(lhap->fd, &lhap->flags);

   al_unlock_mutex(haptic_mutex);

   return &lhap->parent;
}


static A5O_HAPTIC *lhap_get_from_display(A5O_DISPLAY *dev)
{
   (void)dev;
   return NULL;
}


static A5O_HAPTIC *lhap_get_from_keyboard(A5O_KEYBOARD *dev)
{
   (void)dev;
   return NULL;
}


static A5O_HAPTIC *lhap_get_from_touch_input(A5O_TOUCH_INPUT *dev)
{
   (void)dev;
   return NULL;
}


static int lhap_get_capabilities(A5O_HAPTIC *dev)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(dev);
   return lhap->flags;
}


static double lhap_get_gain(A5O_HAPTIC *dev)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(dev);
   (void)dev;

   if(!al_is_haptic_capable(dev, A5O_HAPTIC_GAIN)) {
     return 0.0;
   }

   /* Unfortunately there seems to be no API to GET gain, only to set?!
    * So, return the stored gain.
    */
   return lhap->parent.gain;
}


static bool lhap_set_gain(A5O_HAPTIC *dev, double gain)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(dev);
   struct input_event ie;

   lhap->parent.gain = gain;
   lhap_timerclear(&ie);
   ie.type = EV_FF;
   ie.code = FF_GAIN;
   ie.value = (__s32) ((double)0xFFFF * gain);
   if (write(lhap->fd, &ie, sizeof(ie)) < 0) {
      return false;
   }
   return true;
}


static bool lhap_set_autocenter(A5O_HAPTIC *dev, double autocenter)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(dev);
   struct input_event ie;

   lhap->parent.autocenter = autocenter;
   lhap_timerclear(&ie);
   ie.type = EV_FF;
   ie.code = FF_AUTOCENTER;
   ie.value = (__s32) ((double)0xFFFF * autocenter);
   if (write(lhap->fd, &ie, sizeof(ie)) < 0) {
      return false;
   }
   return true;
}

static double lhap_get_autocenter(A5O_HAPTIC *dev)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(dev);
   (void)dev;

   if(!al_is_haptic_capable(dev, A5O_HAPTIC_AUTOCENTER)) {
     return 0.0;
   }

   /* Unfortunately there seems to be no API to GET gain, only to set?!
    * So, return the stored autocenter.
    */
   return lhap->parent.autocenter;
}

int lhap_get_max_effects(A5O_HAPTIC *dev)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(dev);
   int n_effects;

   if (ioctl(lhap->fd, EVIOCGEFFECTS, &n_effects) < 0) {
      A5O_WARN("EVIOCGEFFECTS failed on fd %d\n", lhap->fd);
      n_effects = HAPTICS_EFFECTS_MAX;
   }

   if (n_effects < HAPTICS_EFFECTS_MAX)
      return n_effects;
   else
      return HAPTICS_EFFECTS_MAX;
}


static bool lhap_is_effect_ok(A5O_HAPTIC *haptic,
                              A5O_HAPTIC_EFFECT *effect)
{
   int caps;
   struct ff_effect leff;

   caps = al_get_haptic_capabilities(haptic);
   if (caps & effect->type) {
      return lhap_effect2lin(&leff, effect);
   }
   return false;
}


static bool lhap_upload_effect(A5O_HAPTIC *dev,
   A5O_HAPTIC_EFFECT *effect, A5O_HAPTIC_EFFECT_ID *id)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(dev);
   struct ff_effect leff;
   int found;
   int i;

   ASSERT(dev);
   ASSERT(id);
   ASSERT(effect);

   /* Set id's values to indicate failure. */
   id->_haptic = NULL;
   id->_id = -1;
   id->_handle = -1;

   if (!lhap_effect2lin(&leff, effect)) {
      A5O_WARN("lhap_effect2lin failed");
      return false;
   }

   /* Find empty spot for effect . */
   found = -1;
   for (i = 0; i < al_get_max_haptic_effects(dev); i++) {
      if (lhap->effects[i] < 0) {
         found = i;
         break;
      }
   }

   /* No more space for an effect. */
   if (found < 0) {
      A5O_WARN("No free effect slot.");
      return false;
   }

   /* Upload effect. */
   leff.id = -1;
   if (ioctl(lhap->fd, EVIOCSFF, &leff) < 0) {
      A5O_ERROR("EVIOCSFF failed for fd %d\n", lhap->fd);
      return false;
   }

   id->_haptic = dev;
   id->_id = found;
   id->_handle = leff.id;
   id->_effect_duration = al_get_haptic_effect_duration(effect);
   id->_playing = false;

   /* XXX should be bool or something? */
   lhap->effects[i] = found;

   return true;
}


static bool lhap_play_effect(A5O_HAPTIC_EFFECT_ID *id, int loops)
{
   A5O_HAPTIC_LINUX *lhap = (A5O_HAPTIC_LINUX *) id->_haptic;
   struct input_event play;
   int fd;
   double now;
   double duration;

   if (!lhap)
      return false;

   fd = lhap->fd;

   lhap_timerclear(&play);
   play.type = EV_FF;
   play.code = id->_handle;
   loops = (loops < 0) ? 1 : loops;
   play.value = loops; /* play: 1, stop: 0 */

   if (write(fd, (const void *)&play, sizeof(play)) < 0) {
      A5O_ERROR("Effect play failed.\n");
      return false;
   }

   now = al_get_time();
   duration = loops * id->_effect_duration;

   id->_playing = true;
   id->_start_time = now;
   id->_end_time = now + duration;

   return true;
}


static bool lhap_stop_effect(A5O_HAPTIC_EFFECT_ID *id)
{
   A5O_HAPTIC_LINUX *lhap = (A5O_HAPTIC_LINUX *) id->_haptic;
   struct input_event play;

   if (!lhap)
      return false;

   memset(&play, 0, sizeof(play));

   play.type = EV_FF;
   play.code = id->_handle;
   play.value = 0;
   if (write(lhap->fd, (const void *)&play, sizeof(play)) < 0) {
      A5O_ERROR("Stop effect failed.\n");
      return false;
   }
   id->_playing = false;
   return true;
}


static bool lhap_is_effect_playing(A5O_HAPTIC_EFFECT_ID *id)
{
   ASSERT(id);

   /* Since AFAICS there is no Linux API to test this, use a timer to check
    * if the effect has been playing long enough to be finished or not.
    */
   return (id->_playing && al_get_time() < id->_end_time);
}


static bool lhap_release_effect(A5O_HAPTIC_EFFECT_ID *id)
{
   A5O_HAPTIC_LINUX *lhap = (A5O_HAPTIC_LINUX *)id->_haptic;

   lhap_stop_effect(id);

   if (ioctl(lhap->fd, EVIOCRMFF, id->_handle) < 0) {
      A5O_ERROR("EVIOCRMFF failed.\n");
      return false;
   }
   lhap->effects[id->_id] = -1; /* not in use */
   return true;
}


static bool lhap_release(A5O_HAPTIC *haptic)
{
   A5O_HAPTIC_LINUX *lhap = lhap_from_al(haptic);
   ASSERT(haptic);

   if (!lhap->active)
      return false;

   lhap->active = false;
   lhap->fd = -1;
   return true;
}

void lhap_timerclear(struct input_event* evt)
{
   evt->input_event_sec = 0;
   evt->input_event_usec = 0;
}

#endif /* A5O_HAVE_LINUX_INPUT_H */


/* vim: set sts=3 sw=3 et: */
