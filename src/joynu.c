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


#define ALLEGRO_NO_COMPATIBILITY

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern2.h"



/* the active joystick driver */
static AL_JOYSTICK_DRIVER *new_joystick_driver = NULL;

/* a list of joystick devices currently "opened" */
static _AL_VECTOR opened_joysticks = _AL_VECTOR_INITIALIZER(AL_JOYSTICK *);



/* al_install_joystick: [primary thread]
 *
 *  Install a joystick driver, returning true if successful.  If a
 *  joystick driver was already installed, returns true immediately.
 */
bool al_install_joystick(void)
{
   _DRIVER_INFO *driver_list;
   AL_JOYSTICK_DRIVER *driver;
   int c;

   if (new_joystick_driver)
      return true;

   ASSERT(_al_vector_is_empty(&opened_joysticks));

   if (system_driver->joystick_drivers)
      driver_list = system_driver->joystick_drivers();
   else
      driver_list = _al_joystick_driver_list;

#if 0
   /* search table for a specific driver */
   /* DISABLED: There is no need so far as every OS only has one
    * joystick driver.
    */
   for (c=0; driver_list[c].driver; c++) { 
      if (driver_list[c].id == type) {
	 joystick_driver = driver_list[c].driver;
	 joystick_driver->name = joystick_driver->desc = get_config_text(joystick_driver->ascii_name);
	 _joy_type = type;
	 if (joystick_driver->init() != 0) {
	    if (!ugetc(allegro_error))
	       uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s not found"), joystick_driver->name);
	    joystick_driver = NULL; 
	    _joy_type = JOY_TYPE_NONE;
	    return -1;
	 }
	 break;
      }
   }
#endif /* #if 0 */

   /* autodetect driver */
   for (c=0; driver_list[c].driver; c++) {
      if (driver_list[c].autodetect) {
         driver = driver_list[c].driver;
         driver->name = driver->desc = get_config_text(driver->ascii_name);
         if (driver->init()) {
            new_joystick_driver = driver;
            break;
         }
      }
   }

   if (new_joystick_driver) {
      _add_exit_func(al_uninstall_joystick);
      return true;
   }

   return false;
}



/* al_uninstall_joystick: [primary thread]
 *
 *  Uninstalls the active joystick driver.  All outstanding
 *  AL_JOYSTICKs are automatically released.  If no joystick driver
 *  was active, this function does nothing.
 *
 *  This function is automatically called when Allegro is shut down.
 */
void al_uninstall_joystick(void)
{
   if (new_joystick_driver) {
      /* automatically release all the outstanding joysticks */
      while (!_al_vector_is_empty(&opened_joysticks)) {
         AL_JOYSTICK **slot = _al_vector_ref_back(&opened_joysticks);
         al_release_joystick(*slot);
      }
      _al_vector_free(&opened_joysticks);

      /* perform driver clean up */
      new_joystick_driver->exit();
      new_joystick_driver = NULL;
   }

   /* this is an atexit'd function */
   _remove_exit_func(al_uninstall_joystick);
}



/* al_num_joysticks: [primary thread]
 *
 *  Return the number of joysticks on the system (depending on the OS
 *  this may not be accurate).  The joystick driver must already be
 *  installed.
 */
int al_num_joysticks(void)
{
   ASSERT(new_joystick_driver);

   if (new_joystick_driver)
      return new_joystick_driver->num_joysticks();

   return 0;
}



/* find_opened_joystick_by_num: [primary thread]
 *
 *  Return the joystick structure corresponding to device number NUM if
 *  the device was already opened.
 */
static AL_JOYSTICK *find_opened_joystick_by_num(int num)
{
   AL_JOYSTICK **slot;
   int i;

   for (i = 0; i < _al_vector_size(&opened_joysticks); i++) {
      slot = _al_vector_ref(&opened_joysticks, i);
      if ((*slot)->num == num)
         return *slot;
   }

   return NULL;
}



/* al_get_joystick: [primary thread]
 *
 *  Get a handle for joystick number NUM on the system.  If successful
 *  a pointer to a joystick object is returned.  Otherwise NULL is
 *  returned.
 *
 *  If the joystick was previously 'gotten' (and not yet released)
 *  then the returned pointer will be the same as in previous calls.
 */
AL_JOYSTICK *al_get_joystick(int num)
{
   ASSERT(new_joystick_driver);
   ASSERT(num >= 0);
   {
      AL_JOYSTICK *joy;
      AL_JOYSTICK **slot;

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



/* al_release_joystick: [primary thread]
 *  Release a previously 'gotten' joystick object.
 */
void al_release_joystick(AL_JOYSTICK *joy)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);

   new_joystick_driver->release_joystick(joy);

   _al_vector_find_and_delete(&opened_joysticks, &joy);
}



/* al_joystick_name: [primary thread]
 *  Return the name of the given joystick.
 */
AL_CONST char *al_joystick_name(AL_JOYSTICK *joy)
{
   ASSERT(joy);

   return "Joystick"; /* TODO */
}



/* al_joystick_num_sticks: [primary thread]
 *  Return the number of "sticks" on the given joystick.
 */
int al_joystick_num_sticks(AL_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_sticks;
}



/* al_joystick_stick_flags: [primary thread]
 *  Return the flags of the given "stick".  If the stick doesn't
 *  exist, NULL is returned.
 */
int al_joystick_stick_flags(AL_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].flags;

   return 0;
}



/* al_joystick_stick_name: [primary thread]
 *  Return the name of the given "stick".  If the stick doesn't
 *  exist, NULL is returned.
 */
AL_CONST char *al_joystick_stick_name(AL_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].name;

   return NULL;
}



/* al_joystick_num_axes: [primary thread]
 *  Return the number of axes on the given "stick".  If the stick
 *  doesn't exist, 0 is returned.
 */
int al_joystick_num_axes(AL_JOYSTICK *joy, int stick)
{
   ASSERT(joy);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].num_axes;

   return 0;
}



/* al_joystick_axis_name: [primary thread]
 *  Return the name of the given axis.  If the axis doesn't exist,
 *  NULL is returned.
 */
AL_CONST char *al_joystick_axis_name(AL_JOYSTICK *joy, int stick, int axis)
{
   ASSERT(joy);
   ASSERT(stick >= 0);
   ASSERT(axis >= 0);

   if (stick < joy->info.num_sticks)
      if (axis < joy->info.stick[stick].num_axes)
         return joy->info.stick[stick].axis[axis].name;

   return NULL;
}



/* al_joystick_num_buttons: [primary thread]
 *  Return the number of buttons on the joystick.
 */
int al_joystick_num_buttons(AL_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_buttons;
}



/* al_joystick_button_name: [primary thread]
 *  Return the name of the given button.  If the button doesn't exist,
 *  NULL is returned.
 */
AL_CONST char *al_joystick_button_name(AL_JOYSTICK *joy, int button)
{
   ASSERT(joy);
   ASSERT(button >= 0);

   if (button < joy->info.num_buttons)
      return joy->info.button[button].name;

   return NULL;
}



/* al_get_joystick_state: [primary thread]
 *  Get the current joystick state.
 */
void al_get_joystick_state(AL_JOYSTICK *joy, AL_JOYSTATE *ret_state)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);
   ASSERT(ret_state);

   new_joystick_driver->get_state(joy, ret_state);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
