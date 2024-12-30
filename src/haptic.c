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
 *      Haptic API.
 *
 *      By Beoran.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/haptic.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_haptic.h"
#include "allegro5/internal/aintern_system.h"


/* the active haptic driver */
static ALLEGRO_HAPTIC_DRIVER *haptic_driver = NULL;


/* Function: al_install_haptic
 */
bool al_install_haptic(void)
{
   ALLEGRO_SYSTEM *sysdrv;
   ALLEGRO_HAPTIC_DRIVER *hapdrv;

   if (haptic_driver)
      return true;

   sysdrv = al_get_system_driver();
   ASSERT(sysdrv);

   /* Currently every platform only has at most one haptic driver. */
   if (sysdrv->vt->get_haptic_driver) {
      hapdrv = sysdrv->vt->get_haptic_driver();
      /* Avoid race condition in case the haptic driver generates an
       * event right after ->init_haptic.
       */
      if (hapdrv && hapdrv->init_haptic()) {
         haptic_driver = hapdrv;
         _al_add_exit_func(al_uninstall_haptic, "al_uninstall_haptic");
         return true;
      }
   }

   return false;
}


/* Function: al_uninstall_haptic
 */
void al_uninstall_haptic(void)
{
   if (haptic_driver) {
      /* perform driver clean up */
      haptic_driver->exit_haptic();
      haptic_driver = NULL;
   }
}


/* Function: al_is_haptic_installed
 */
bool al_is_haptic_installed(void)
{
   return (haptic_driver) ? true : false;
}


/* Function: al_is_joystick_haptic
 */
bool al_is_joystick_haptic(ALLEGRO_JOYSTICK *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->is_joystick_haptic(dev);
}


/* Function: al_is_mouse_haptic
 */
bool al_is_mouse_haptic(ALLEGRO_MOUSE *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->is_mouse_haptic(dev);
}


/* Function: al_is_keyboard_haptic
 */
bool al_is_keyboard_haptic(ALLEGRO_KEYBOARD *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->is_keyboard_haptic(dev);
}


/* Function: al_is_display_haptic
 */
bool al_is_display_haptic(ALLEGRO_DISPLAY *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->is_display_haptic(dev);
}

/* Function: al_is_touch_input_haptic
 */
bool al_is_touch_input_haptic(ALLEGRO_TOUCH_INPUT *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->is_touch_input_haptic(dev);
}

/* Function: al_get_haptic_from_joystick
 */
ALLEGRO_HAPTIC *al_get_haptic_from_joystick(ALLEGRO_JOYSTICK *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->get_from_joystick(dev);
}


/* Function: al_get_haptic_from_mouse
 */
ALLEGRO_HAPTIC *al_get_haptic_from_mouse(ALLEGRO_MOUSE *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->get_from_mouse(dev);
}


/* Function: al_get_haptic_from_keyboard
 */
ALLEGRO_HAPTIC *al_get_haptic_from_keyboard(ALLEGRO_KEYBOARD *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->get_from_keyboard(dev);
}


/* Function: al_get_haptic_from_display
 */
ALLEGRO_HAPTIC *al_get_haptic_from_display(ALLEGRO_DISPLAY *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->get_from_display(dev);
}


/* Function: al_get_haptic_from_touch_input
 */
ALLEGRO_HAPTIC *al_get_haptic_from_touch_input(ALLEGRO_TOUCH_INPUT *dev)
{
   ASSERT(dev);
   ASSERT(haptic_driver);

   return haptic_driver->get_from_touch_input(dev);
}


/* Function: al_is_haptic_active
 */
bool al_is_haptic_active(ALLEGRO_HAPTIC *hap)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->get_active(hap);
}


/* Function: al_get_haptic_capabilities
 */
int al_get_haptic_capabilities(ALLEGRO_HAPTIC *hap)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->get_capabilities(hap);
}

/* Function: al_is_haptic_capable
 */
bool al_is_haptic_capable(ALLEGRO_HAPTIC * hap, int query) {
  int capabilities = al_get_haptic_capabilities(hap);
  return (capabilities & query) == query;
}

/* Function: al_get_haptic_gain
 */
double al_get_haptic_gain(ALLEGRO_HAPTIC *hap)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->get_gain(hap);
}


/* Function: al_set_haptic_gain
 */
bool al_set_haptic_gain(ALLEGRO_HAPTIC *hap, double gain)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->set_gain(hap, gain);
}

/* Function: al_get_haptic_autocenter
 */
double al_get_haptic_autocenter(ALLEGRO_HAPTIC *hap)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->get_autocenter(hap);
}


/* Function: al_set_haptic_autocenter
 */
bool al_set_haptic_autocenter(ALLEGRO_HAPTIC *hap, double intensity)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->set_autocenter(hap, intensity);
}



/* Function: al_get_max_haptic_effects
 */
int al_get_max_haptic_effects(ALLEGRO_HAPTIC *hap)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->get_max_effects(hap);
}


/* Function: al_is_haptic_effect_ok
 */
bool al_is_haptic_effect_ok(ALLEGRO_HAPTIC *hap, ALLEGRO_HAPTIC_EFFECT *effect)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->is_effect_ok(hap, effect);
}


/* Function: al_upload_haptic_effect
 */
bool al_upload_haptic_effect(ALLEGRO_HAPTIC *hap,
   ALLEGRO_HAPTIC_EFFECT *effect, ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ASSERT(hap);
   ASSERT(haptic_driver);

   return haptic_driver->upload_effect(hap, effect, id);
}


/* Function: al_play_haptic_effect
 */
bool al_play_haptic_effect(ALLEGRO_HAPTIC_EFFECT_ID *id, int loop)
{
   ASSERT(haptic_driver);
   ASSERT(id);

   return haptic_driver->play_effect(id, loop);
}


/* Function: al_upload_and_play_haptic_effect
 */
bool al_upload_and_play_haptic_effect(ALLEGRO_HAPTIC *hap,
   ALLEGRO_HAPTIC_EFFECT *effect, ALLEGRO_HAPTIC_EFFECT_ID *id, int loop)
{
   ASSERT(hap);
   ASSERT(effect);
   ASSERT(id);

   if (!al_upload_haptic_effect(hap, effect, id))
      return false;
   /* If playing the effect failed, unload the haptic effect automatically
    */
   if (!al_play_haptic_effect(id, loop)) {
     al_release_haptic_effect(id);
     return false;
   }
   return true;
}


/* Function: al_stop_haptic_effect
 */
bool al_stop_haptic_effect(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ASSERT(id);

   return haptic_driver->stop_effect(id);
}


/* Function: al_is_haptic_effect_playing
 */
bool al_is_haptic_effect_playing(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ASSERT(id);

   return haptic_driver->is_effect_playing(id);
}

/* Function: al_get_haptic_effect_duration
 */
double al_get_haptic_effect_duration(ALLEGRO_HAPTIC_EFFECT * effect)
{
  return effect->replay.delay + effect->replay.length;
}

/* Function: al_rumble_haptic
 */
bool al_rumble_haptic(ALLEGRO_HAPTIC *hap,
   double intensity, double duration, ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ALLEGRO_HAPTIC_EFFECT effect;
   ASSERT(hap);
   ASSERT(id);

   effect.type = ALLEGRO_HAPTIC_RUMBLE;
   effect.data.rumble.strong_magnitude = intensity;
   effect.data.rumble.weak_magnitude = intensity;
   effect.replay.delay = 0.0;
   effect.replay.length = duration;
   return al_upload_and_play_haptic_effect(hap, &effect, id, 1);
}


/* Function: al_release_haptic_effect
 */
bool al_release_haptic_effect(ALLEGRO_HAPTIC_EFFECT_ID *id)
{
   ASSERT(haptic_driver);
   ASSERT(id);

   return haptic_driver->release_effect(id);
}


/* Function: al_release_haptic
 */
bool al_release_haptic(ALLEGRO_HAPTIC *haptic)
{
   ASSERT(haptic_driver);
   ASSERT(haptic);

   return haptic_driver->release(haptic);
}


/* vim: set sts=3 sw=3 et: */
