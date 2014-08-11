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
 *      Windows wrapper haptic (force-feedback) device driver.
 *      This wraps around the XInput and DirectInput drivers to support both.
 *      By Beoran.
 *
 *      See LICENSE.txt for copyright information.
 */


#define ALLEGRO_NO_COMPATIBILITY

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

#ifdef ALLEGRO_CFG_XINPUT
/* Don't compile this lot if xinput and directinput isn't supported. */

#include "allegro5/internal/aintern_wjoyall.h"

ALLEGRO_DEBUG_CHANNEL("haptic")

/* forward declarations */
static bool hapall_init_haptic(void);
static void hapall_exit_haptic(void);

static bool hapall_is_mouse_haptic(ALLEGRO_MOUSE *dev);
static bool hapall_is_joystick_haptic(ALLEGRO_JOYSTICK *);
static bool hapall_is_keyboard_haptic(ALLEGRO_KEYBOARD *dev);
static bool hapall_is_display_haptic(ALLEGRO_DISPLAY *dev);
static bool hapall_is_touch_input_haptic(ALLEGRO_TOUCH_INPUT *dev);

static ALLEGRO_HAPTIC *hapall_get_from_mouse(ALLEGRO_MOUSE *dev);
static ALLEGRO_HAPTIC *hapall_get_from_joystick(ALLEGRO_JOYSTICK *dev);
static ALLEGRO_HAPTIC *hapall_get_from_keyboard(ALLEGRO_KEYBOARD *dev);
static ALLEGRO_HAPTIC *hapall_get_from_display(ALLEGRO_DISPLAY *dev);
static ALLEGRO_HAPTIC *hapall_get_from_touch_input(ALLEGRO_TOUCH_INPUT *dev);

static bool hapall_release(ALLEGRO_HAPTIC *haptic);

static bool hapall_get_active(ALLEGRO_HAPTIC *hap);
static int hapall_get_capabilities(ALLEGRO_HAPTIC *dev);
static double hapall_get_gain(ALLEGRO_HAPTIC *dev);
static bool hapall_set_gain(ALLEGRO_HAPTIC *dev, double);
static int hapall_get_num_effects(ALLEGRO_HAPTIC *dev);

static bool hapall_is_effect_ok(ALLEGRO_HAPTIC *dev, ALLEGRO_HAPTIC_EFFECT *eff);
static bool hapall_upload_effect(ALLEGRO_HAPTIC *dev,
                                 ALLEGRO_HAPTIC_EFFECT *eff,
                                 ALLEGRO_HAPTIC_EFFECT_ID *id);
static bool hapall_play_effect(ALLEGRO_HAPTIC_EFFECT_ID *id, int loop);
static bool hapall_stop_effect(ALLEGRO_HAPTIC_EFFECT_ID *id);
static bool hapall_is_effect_playing(ALLEGRO_HAPTIC_EFFECT_ID *id);
static bool hapall_release_effect(ALLEGRO_HAPTIC_EFFECT_ID *id);

static double hapall_get_autocenter(ALLEGRO_HAPTIC *dev);
static bool hapall_set_autocenter(ALLEGRO_HAPTIC *dev, double);


ALLEGRO_HAPTIC_DRIVER _al_hapdrv_windows_all =
{
   AL_HAPTIC_TYPE_WINDOWS_ALL,
   "",
   "",
   "Windows haptic(s)",
   hapall_init_haptic,
   hapall_exit_haptic,

   hapall_is_mouse_haptic,
   hapall_is_joystick_haptic,
   hapall_is_keyboard_haptic,
   hapall_is_display_haptic,
   hapall_is_touch_input_haptic,

   hapall_get_from_mouse,
   hapall_get_from_joystick,
   hapall_get_from_keyboard,
   hapall_get_from_display,
   hapall_get_from_touch_input,

   hapall_get_active,
   hapall_get_capabilities,
   hapall_get_gain,
   hapall_set_gain,
   hapall_get_num_effects,

   hapall_is_effect_ok,
   hapall_upload_effect,
   hapall_play_effect,
   hapall_stop_effect,
   hapall_is_effect_playing,
   hapall_release_effect,

   hapall_release,

   hapall_get_autocenter,
   hapall_set_autocenter
};


/* Mutex for thread protection. */
static ALLEGRO_MUTEX  *hapall_mutex = NULL;


/* Initializes the combined haptic system. */
static bool hapall_init_haptic(void)
{
   bool xi_ok, di_ok;
   ASSERT(hapall_mutex == NULL);

   /* Create the mutex. */
   hapall_mutex = al_create_mutex_recursive();
   if (!hapall_mutex)
      return false;

   al_lock_mutex(hapall_mutex);

   xi_ok = _al_hapdrv_xinput.init_haptic();
   di_ok = _al_hapdrv_directx.init_haptic();
   al_unlock_mutex(hapall_mutex);
   return xi_ok || di_ok;
}

static void hapall_exit_haptic(void)
{
   ASSERT(hapall_mutex);
   al_destroy_mutex(hapall_mutex);
   hapall_mutex = NULL;
}

static bool hapall_get_active(ALLEGRO_HAPTIC *haptic)
{
   return haptic->driver->get_active(haptic);
}


static bool hapall_is_mouse_haptic(ALLEGRO_MOUSE *mouse)
{
   (void)mouse;
   return false;
}


static bool hapall_is_joystick_haptic(ALLEGRO_JOYSTICK *joy)
{
   bool dx_ok = false, xi_ok = false;
   ASSERT(joy->driver);

   if (joy->driver == &_al_joydrv_xinput) {
      dx_ok = _al_hapdrv_xinput.is_joystick_haptic(joy);
   }
   else if (joy->driver == &_al_joydrv_directx) {
      xi_ok = _al_hapdrv_directx.is_joystick_haptic(joy);
   }
   return dx_ok || xi_ok;
}


static bool hapall_is_display_haptic(ALLEGRO_DISPLAY *dev)
{
   (void)dev;
   return false;
}


static bool hapall_is_keyboard_haptic(ALLEGRO_KEYBOARD *dev)
{
   (void)dev;
   return false;
}


static bool hapall_is_touch_input_haptic(ALLEGRO_TOUCH_INPUT *dev)
{
   (void)dev;
   return false;
}


static ALLEGRO_HAPTIC *hapall_get_from_mouse(ALLEGRO_MOUSE *mouse)
{
   (void)mouse;
   return NULL;
}


static ALLEGRO_HAPTIC *hapall_get_from_joystick(ALLEGRO_JOYSTICK *joy)
{
   ALLEGRO_HAPTIC *haptic = NULL;
   if (!al_is_joystick_haptic(joy))
      return NULL;

   al_lock_mutex(hapall_mutex);

   if (joy->driver == &_al_joydrv_xinput) {
      haptic = _al_hapdrv_xinput.get_from_joystick(joy);
      if (haptic) {
         haptic->driver = &_al_hapdrv_xinput;
      }
   }
   else if (joy->driver == &_al_joydrv_directx) {
      haptic = _al_hapdrv_directx.get_from_joystick(joy);
      if (haptic) {
         haptic->driver = &_al_hapdrv_directx;
      }
   }

   al_unlock_mutex(hapall_mutex);
   return haptic;
}


static ALLEGRO_HAPTIC *hapall_get_from_display(ALLEGRO_DISPLAY *dev)
{
   (void)dev;
   return NULL;
}


static ALLEGRO_HAPTIC *hapall_get_from_keyboard(ALLEGRO_KEYBOARD *dev)
{
   (void)dev;
   return NULL;
}


static ALLEGRO_HAPTIC *hapall_get_from_touch_input(ALLEGRO_TOUCH_INPUT *dev)
{
   (void)dev;
   return NULL;
}


static int hapall_get_capabilities(ALLEGRO_HAPTIC *dev)
{
   return dev->driver->get_capabilities(dev);
}


static double hapall_get_gain(ALLEGRO_HAPTIC *dev)
{
   return dev->driver->get_gain(dev);
}


static bool hapall_set_gain(ALLEGRO_HAPTIC *dev, double gain)
{
   return dev->driver->set_gain(dev, gain);
}


double hapall_get_autocenter(ALLEGRO_HAPTIC *dev)
{
   return dev->driver->get_autocenter(dev);
}

static bool hapall_set_autocenter(ALLEGRO_HAPTIC *dev, double intensity)
{
   return dev->driver->set_autocenter(dev, intensity);
}

static int hapall_get_num_effects(ALLEGRO_HAPTIC *dev)
{
   return dev->driver->get_num_effects(dev);
}


static bool hapall_is_effect_ok(ALLEGRO_HAPTIC *dev,
                                ALLEGRO_HAPTIC_EFFECT *effect)
{
   return dev->driver->is_effect_ok(dev, effect);
}


static bool hapall_upload_effect(ALLEGRO_HAPTIC *dev,
                                 ALLEGRO_HAPTIC_EFFECT *effect, ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   /* store the driver we'll need it later. */
   id->driver = dev->driver;
   return dev->driver->upload_effect(dev, effect, id);
}

static bool hapall_play_effect(ALLEGRO_HAPTIC_EFFECT_ID *id, int loops)
{
   ALLEGRO_HAPTIC_DRIVER      *driver = id->driver;
   /* Use the stored driver to perform the operation. */
   return driver->play_effect(id, loops);
}


static bool hapall_stop_effect(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ALLEGRO_HAPTIC_DRIVER *driver = id->driver;
   /* Use the stored driver to perform the operation. */
   return driver->stop_effect(id);
}


static bool hapall_is_effect_playing(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ALLEGRO_HAPTIC_DRIVER  *driver = id->driver;
   /* Use the stored driver to perform the operation. */
   return driver->is_effect_playing(id);
}

static bool hapall_release_effect(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ALLEGRO_HAPTIC_DRIVER *driver = id->driver;
   /* Use the stored driver to perform the operation. */
   return driver->release_effect(id);
}


static bool hapall_release(ALLEGRO_HAPTIC *haptic)
{
   if (!haptic)
      return false;

   return haptic->driver->release(haptic);
}

#endif

/* vim: set sts=3 sw=3 et: */
