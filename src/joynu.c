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
 *      New joystick API.
 * 
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Joystick routines
 */


#define ALLEGRO_NO_COMPATIBILITY

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_system.h"



/* the active joystick driver */
static ALLEGRO_JOYSTICK_DRIVER *new_joystick_driver = NULL;
static ALLEGRO_EVENT_SOURCE es;


/* Function: al_install_joystick
 */
bool al_install_joystick(void)
{
   ALLEGRO_SYSTEM *sysdrv;
   ALLEGRO_JOYSTICK_DRIVER *joydrv;

   if (new_joystick_driver)
      return true;

   sysdrv = al_get_system_driver();
   ASSERT(sysdrv);

   /* Currently every platform only has at most one joystick driver. */
   if (sysdrv->vt->get_joystick_driver) {
      joydrv = sysdrv->vt->get_joystick_driver();
      /* Avoid race condition in case the joystick driver generates an
       * event right after ->init_joystick.
       */
      _al_event_source_init(&es);
      if (joydrv && joydrv->init_joystick()) {
         new_joystick_driver = joydrv;
         _al_add_exit_func(al_uninstall_joystick, "al_uninstall_joystick");
         return true;
      }
      _al_event_source_free(&es);
   }

   return false;
}



/* Function: al_uninstall_joystick
 */
void al_uninstall_joystick(void)
{
   if (new_joystick_driver) {
      /* perform driver clean up */
      new_joystick_driver->exit_joystick();
      _al_event_source_free(&es);
      new_joystick_driver = NULL;
   }
}



/* Function: al_is_joystick_installed
 */
bool al_is_joystick_installed(void)
{
   return (new_joystick_driver) ? true : false;
}



/* Function: al_reconfigure_joysticks
 */
bool al_reconfigure_joysticks(void)
{
   if (!new_joystick_driver)
      return false;

   /* XXX only until Windows and Mac joystick drivers are updated */
   if (!new_joystick_driver->reconfigure_joysticks) {
      new_joystick_driver->num_joysticks();
      return true;
   }

   return new_joystick_driver->reconfigure_joysticks();
}



/* Function: al_get_joystick_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_joystick_event_source(void)
{
   if (!new_joystick_driver)
      return NULL;
   return &es;
}



void _al_generate_joystick_event(ALLEGRO_EVENT *event)
{
   ASSERT(new_joystick_driver);

   _al_event_source_lock(&es);
   if (_al_event_source_needs_to_generate_event(&es)) {
      _al_event_source_emit_event(&es, event);
   }
   _al_event_source_unlock(&es);
}



/* Function: al_get_num_joysticks
 */
int al_get_num_joysticks(void)
{
   if (new_joystick_driver)
      return new_joystick_driver->num_joysticks();

   return 0;
}



/* Function: al_get_joystick
 */
ALLEGRO_JOYSTICK * al_get_joystick(int num)
{
   ASSERT(new_joystick_driver);
   ASSERT(num >= 0);

   return new_joystick_driver->get_joystick(num);
}



/* Function: al_release_joystick
 */
void al_release_joystick(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);

   new_joystick_driver->release_joystick(joy);
}



/* Function: al_get_joystick_active
 */
bool al_get_joystick_active(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return new_joystick_driver->get_active(joy);
}



/* Function: al_get_joystick_name
 */
const char *al_get_joystick_name(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return new_joystick_driver->get_name(joy);
}



/* Function: al_get_joystick_num_sticks
 */
int al_get_joystick_num_sticks(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_sticks;
}



/* Function: al_get_joystick_stick_flags
 */
int al_get_joystick_stick_flags(ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].flags;

   return 0;
}



/* Function: al_get_joystick_stick_name
 */
const char *al_get_joystick_stick_name(ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].name;

   return NULL;
}



/* Function: al_get_joystick_num_axes
 */
int al_get_joystick_num_axes(ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].num_axes;

   return 0;
}



/* Function: al_get_joystick_axis_name
 */
const char *al_get_joystick_axis_name(ALLEGRO_JOYSTICK *joy, int stick, int axis)
{
   ASSERT(joy);
   ASSERT(stick >= 0);
   ASSERT(axis >= 0);

   if (stick < joy->info.num_sticks)
      if (axis < joy->info.stick[stick].num_axes)
         return joy->info.stick[stick].axis[axis].name;

   return NULL;
}



/* Function: al_get_joystick_num_buttons
 */
int al_get_joystick_num_buttons(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_buttons;
}



/* Function: al_get_joystick_button_name
 */
const char *al_get_joystick_button_name(ALLEGRO_JOYSTICK *joy, int button)
{
   ASSERT(joy);
   ASSERT(button >= 0);

   if (button < joy->info.num_buttons)
      return joy->info.button[button].name;

   return NULL;
}



/* Function: al_get_joystick_state
 */
void al_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);
   ASSERT(ret_state);

   new_joystick_driver->get_joystick_state(joy, ret_state);
}



uint32_t _al_get_joystick_compat_version(void)
{
   ALLEGRO_CONFIG *system_config = al_get_system_config();
   const char* compat_version = al_get_config_value(system_config, "compatibility", "joystick_version");
   if (!compat_version || strlen(compat_version) == 0)
      return al_get_allegro_version();
   int version = 0;
   int sub_version = 0;
   int wip_version = 0;
   /* Ignore the release number, we don't expect that to make a difference */
   sscanf(compat_version, "%2d.%2d.%2d", &version, &sub_version, &wip_version);
   return AL_ID(version, sub_version, wip_version, 0);
}

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
