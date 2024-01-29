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
 *      Windows wrap-all joystick driver.
 *      By Beoran.
 *      See readme.txt for copyright information.
 *
 * This driver exists because on Windows, DirectInput and XInput are two
 * different joystick input APIs which bith need to be supported simultaneously.
 *
 * Although DirectInput is deprecated, it is a far more capable API in terms
 * of joystick layout and force feedback effects. XInput is a much simpler
 * API but it only supports joysticks that have a layout similar to that of an
 * XBOX controller. XInput also has very limited force feedback effects,
 * it only supportd rubmble style vibrations.
 *
 * Older joystics or input devices such as steering wheels that do not map
 * cleanly to an XBOX controller tend to have DirectInput drivers available,
 * while more recent joypads tend to have a XInput driver available. In theory
 * XInput devices also support DirectInput, and some devices even have a switch
 * that lets the user select the API. But XInput devices only partially support
 * DirectInput. In particular, XInput devices do not support force feedback when
 * using DirectInput to access them.
 *
 * For all these reasons, both directinput and xinput drivers should be
 * supported on Windows to allow greates comfort to end users.
 * The wjoyall.c and whapall.c drivers are wrapper drivers that combine
 * the Allegro XInput and DirectInput drivers into a single driver.
 * For this reason, the same joystick ormay show up twice
 * in the joystick list. However, XInput devices will appear before DirectInput
 * devices in the list, so when in doubt, use the joystick with the lowest ID.
 *
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

#include <stdio.h>

ALLEGRO_DEBUG_CHANNEL("wjoyall")

#include "allegro5/joystick.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_wjoyall.h"

/* forward declarations */
static bool joyall_init_joystick(void);
static void joyall_exit_joystick(void);
static bool joyall_reconfigure_joysticks(void);
static int joyall_get_num_joysticks(void);
static ALLEGRO_JOYSTICK *joyall_get_joystick(int num);
static void joyall_release_joystick(ALLEGRO_JOYSTICK *joy);
static void joyall_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state);
static const char *joyall_get_name(ALLEGRO_JOYSTICK *joy);
static bool joyall_get_active(ALLEGRO_JOYSTICK *joy);
static int joyall_get_device_id(ALLEGRO_JOYSTICK *joy);


/* the driver vtable */
ALLEGRO_JOYSTICK_DRIVER _al_joydrv_windows_all =
{
   AL_JOY_TYPE_WINDOWS_ALL,
   "",
   "",
   "Windows Joystick",
   joyall_init_joystick,
   joyall_exit_joystick,
   joyall_reconfigure_joysticks,
   joyall_get_num_joysticks,
   joyall_get_joystick,
   joyall_release_joystick,
   joyall_get_joystick_state,
   joyall_get_name,
   joyall_get_active,
   joyall_get_device_id
};

/* Mutex to protect state access. XXX is this needed? */
static ALLEGRO_MUTEX  *joyall_mutex = NULL;

static bool ok_xi = false;
static bool ok_di = false;

/* Sets up all joysticks from the two wrapped apis. */
static void joyall_setup_joysticks(void)
{
   int index;
   int num_xinput = 0;
   int num_dinput = 0;
   if (ok_di)
      num_dinput = _al_joydrv_directx.num_joysticks();
   if (ok_xi)
      num_xinput = _al_joydrv_xinput.num_joysticks();

   for (index = 0; index < num_xinput; index++) {
      ALLEGRO_JOYSTICK *joystick = _al_joydrv_xinput.get_joystick(index);
      joystick->driver = &_al_joydrv_xinput;
   }

   for (index = 0; index < num_dinput; index++) {
      ALLEGRO_JOYSTICK *joystick = _al_joydrv_directx.get_joystick(index);
      joystick->driver = &_al_joydrv_directx;
   }
}


/* Initialization API function. */
static bool joyall_init_joystick(void)
{
   /* Create the mutex and a condition vaiable. */
   joyall_mutex = al_create_mutex_recursive();
   if (!joyall_mutex)
      return false;

   al_lock_mutex(joyall_mutex);

   ok_xi = _al_joydrv_xinput.init_joystick();
   ok_di = _al_joydrv_directx.init_joystick();
   joyall_setup_joysticks();
   al_unlock_mutex(joyall_mutex);
   return ok_xi || ok_di;
}


static void joyall_exit_joystick(void)
{
   al_lock_mutex(joyall_mutex);
   _al_joydrv_xinput.exit_joystick();
   _al_joydrv_directx.exit_joystick();
   al_unlock_mutex(joyall_mutex);
   al_destroy_mutex(joyall_mutex);
}

static bool joyall_reconfigure_joysticks(void)
{
   al_lock_mutex(joyall_mutex);
   if (ok_xi)
      _al_joydrv_xinput.reconfigure_joysticks();
   if (ok_di)
      _al_joydrv_directx.reconfigure_joysticks();
   joyall_setup_joysticks();
   al_unlock_mutex(joyall_mutex);
   return true;
}

static int joyall_get_num_joysticks(void)
{
   int ret = 0;
   if (ok_xi)
      ret += _al_joydrv_xinput.num_joysticks();
   if (ok_di)
      ret += _al_joydrv_directx.num_joysticks();
   return ret;
}

static ALLEGRO_JOYSTICK *joyall_get_joystick(int num)
{
   int num_xinput = 0;
   int num_dinput = 0;
   if (ok_di)
      num_dinput = _al_joydrv_directx.num_joysticks();
   if (ok_xi)
      num_xinput = _al_joydrv_xinput.num_joysticks();
   if (num < 0) return NULL;
   /* Shift the joystick number to fit the range of each of the subdrivers */
   if (num < num_xinput) {
      return _al_joydrv_xinput.get_joystick(num);
   }
   else if (num < (num_xinput + num_dinput)) {
      return _al_joydrv_directx.get_joystick(num - num_xinput);
   }
   return NULL;
}

static void joyall_release_joystick(ALLEGRO_JOYSTICK *joy)
{
   /* Forward to the driver's function. Here it's OK to use joy
    * since the get_joystick function returns a
    * pointer to the real underlying driver-specific joystick data.
    */
   if (joy) {
      joy->driver->release_joystick(joy);
   }
}

static void joyall_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   joy->driver->get_joystick_state(joy, ret_state);
}

static const char *joyall_get_name(ALLEGRO_JOYSTICK *joy)
{
   return joy->driver->get_name(joy);
}

static bool joyall_get_active(ALLEGRO_JOYSTICK *joy)
{
   return joy->driver->get_active(joy);
}

static int joyall_get_device_id(ALLEGRO_JOYSTICK *joy)
{
   (void)joy;
   // TODO: Add implementation here
   ALLEGRO_INFO("joyall_get_device_id: not implemented");
   return 0;
}

#endif /* #ifdef ALLEGRO_CFG_XINPUT */
