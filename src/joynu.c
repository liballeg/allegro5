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

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_system.h"



/* the active joystick driver */
static ALLEGRO_JOYSTICK_DRIVER *new_joystick_driver = NULL;

/* a list of joystick devices currently "opened" */
static _AL_VECTOR opened_joysticks = _AL_VECTOR_INITIALIZER(ALLEGRO_JOYSTICK *);



/* Function: al_install_joystick
 *
 *  Install a joystick driver, returning true if successful.  If a
 *  joystick driver was already installed, returns true immediately.
 */
bool al_install_joystick(void)
{
   ALLEGRO_SYSTEM *sysdrv;
   ALLEGRO_JOYSTICK_DRIVER *joydrv;

   if (new_joystick_driver)
      return true;

   ASSERT(_al_vector_is_empty(&opened_joysticks));

   sysdrv = al_system_driver();
   ASSERT(sysdrv);

   /* Currently every platform only has at most one joystick driver. */
   if (sysdrv->vt->get_joystick_driver) {
      joydrv = sysdrv->vt->get_joystick_driver();
      if (joydrv->init_joystick()) {
         new_joystick_driver = joydrv;
         _al_add_exit_func(al_uninstall_joystick, "al_uninstall_joystick");
         return true;
      }
   }

   return false;
}



/* Function: al_uninstall_joystick
 *
 *  Uninstalls the active joystick driver.  All outstanding
 *  ALLEGRO_JOYSTICKs are automatically released.  If no joystick driver
 *  was active, this function does nothing.
 *
 *  This function is automatically called when Allegro is shut down.
 */
void al_uninstall_joystick(void)
{
   if (new_joystick_driver) {
      /* automatically release all the outstanding joysticks */
      while (!_al_vector_is_empty(&opened_joysticks)) {
         ALLEGRO_JOYSTICK **slot = _al_vector_ref_back(&opened_joysticks);
         al_release_joystick(*slot);
      }
      _al_vector_free(&opened_joysticks);

      /* perform driver clean up */
      new_joystick_driver->exit_joystick();
      new_joystick_driver = NULL;
   }
}



/* Function: al_num_joysticks
 *
 *  Return the number of joysticks on the system (depending on the OS
 *  this may not be accurate).  Returns 0 if there is no joystick driver
 *  installed.
 */
int al_num_joysticks(void)
{
   if (new_joystick_driver)
      return new_joystick_driver->num_joysticks();

   return 0;
}



/* find_opened_joystick_by_num: [primary thread]
 *
 *  Return the joystick structure corresponding to device number NUM if
 *  the device was already opened.
 */
static ALLEGRO_JOYSTICK *find_opened_joystick_by_num(int num)
{
   ALLEGRO_JOYSTICK **slot;
   unsigned int i;

   for (i = 0; i < _al_vector_size(&opened_joysticks); i++) {
      slot = _al_vector_ref(&opened_joysticks, i);
      if ((*slot)->num == num)
         return *slot;
   }

   return NULL;
}



/* Function: al_get_joystick
 *
 *  Get a handle for joystick number NUM on the system.  If successful
 *  a pointer to a joystick object is returned.  Otherwise NULL is
 *  returned.
 *
 *  If the joystick was previously 'gotten' (and not yet released)
 *  then the returned pointer will be the same as in previous calls.
 */
ALLEGRO_JOYSTICK *al_get_joystick(int num)
{
   ASSERT(new_joystick_driver);
   ASSERT(num >= 0);
   {
      ALLEGRO_JOYSTICK *joy;
      ALLEGRO_JOYSTICK **slot;

      if (num >= new_joystick_driver->num_joysticks())
         return NULL;

      if ((joy = find_opened_joystick_by_num(num)))
         return joy;

      if ((joy = new_joystick_driver->get_joystick(num))) {
         /* Add the new structure to the list of opened joysticks. */
         slot = _al_vector_alloc_back(&opened_joysticks);
         *slot = joy;
      }

      return joy;
   }
}



/* Function: al_release_joystick
 *  Release a previously 'gotten' joystick object.
 */
void al_release_joystick(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);

   new_joystick_driver->release_joystick(joy);

   _al_vector_find_and_delete(&opened_joysticks, &joy);
}



/* Function: al_get_joystick_name
 *  Return the name of the given joystick.
 */
const char *al_get_joystick_name(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);
   (void)joy;

   return "Joystick"; /* TODO */
}



/* Function: al_get_num_joystick_sticks
 *  Return the number of "sticks" on the given joystick.
 */
int al_get_num_joystick_sticks(const ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_sticks;
}



/* Function: al_get_joystick_stick_flags
 *  Return the flags of the given "stick".  If the stick doesn't
 *  exist, NULL is returned.
 */
int al_get_joystick_stick_flags(const ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].flags;

   return 0;
}



/* Function: al_get_joystick_stick_name
 *  Return the name of the given "stick".  If the stick doesn't
 *  exist, NULL is returned.
 */
const char *al_get_joystick_stick_name(const ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].name;

   return NULL;
}



/* Function: al_get_num_joystick_axes
 *  Return the number of axes on the given "stick".  If the stick
 *  doesn't exist, 0 is returned.
 */
int al_get_num_joystick_axes(const ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].num_axes;

   return 0;
}



/* Function: al_get_joystick_axis_name
 *  Return the name of the given axis.  If the axis doesn't exist,
 *  NULL is returned.
 */
const char *al_get_joystick_axis_name(const ALLEGRO_JOYSTICK *joy, int stick, int axis)
{
   ASSERT(joy);
   ASSERT(stick >= 0);
   ASSERT(axis >= 0);

   if (stick < joy->info.num_sticks)
      if (axis < joy->info.stick[stick].num_axes)
         return joy->info.stick[stick].axis[axis].name;

   return NULL;
}



/* Function: al_get_num_joystick_buttons
 *  Return the number of buttons on the joystick.
 */
int al_get_num_joystick_buttons(const ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_buttons;
}



/* Function: al_get_joystick_button_name
 *  Return the name of the given button.  If the button doesn't exist,
 *  NULL is returned.
 */
const char *al_get_joystick_button_name(const ALLEGRO_JOYSTICK *joy, int button)
{
   ASSERT(joy);
   ASSERT(button >= 0);

   if (button < joy->info.num_buttons)
      return joy->info.button[button].name;

   return NULL;
}



/* Function: al_get_joystick_state
 *  Get the current joystick state.
 */
void al_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);
   ASSERT(ret_state);

   new_joystick_driver->get_joystick_state(joy, ret_state);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
