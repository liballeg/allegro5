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
 *      New keyboard API.
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



/* the active keyboard driver */
static AL_KEYBOARD_DRIVER *new_keyboard_driver = NULL;

/* mode flags */
/* TODO: use the config system for these */
bool _al_three_finger_flag = true;
bool _al_key_led_flag = true;



/* al_install_keyboard: [primary thread]
 *  Install a keyboard driver. Returns true if successful. If a driver
 *  was already installed, nothing happens and true is returned.
 */
bool al_install_keyboard(void)
{
   _DRIVER_INFO *driver_list;
   int i;

   if (new_keyboard_driver)
      return true;

   if (system_driver->keyboard_drivers)
      driver_list = system_driver->keyboard_drivers();
   else
      driver_list = _al_keyboard_driver_list;

   for (i=0; driver_list[i].driver; i++) {
      new_keyboard_driver = driver_list[i].driver;
      new_keyboard_driver->name = new_keyboard_driver->desc = get_config_text(new_keyboard_driver->ascii_name);
      if (new_keyboard_driver->init())
	 break;
   }

   if (!driver_list[i].driver) {
      new_keyboard_driver = NULL;
      return false;
   }

   //set_leds(-1);

   _add_exit_func(al_uninstall_keyboard);

   return true;
}



/* al_uninstall_keyboard: [primary thread]
 *  Uninstalls the active keyboard driver, if any.  This will
 *  automatically unregister the keyboard event source with any event
 *  queues.
 *
 *  This function is automatically called when Allegro is shut down.
 */
void al_uninstall_keyboard(void)
{
   if (!new_keyboard_driver)
      return;

   //set_leds(-1);

   new_keyboard_driver->exit();
   new_keyboard_driver = NULL;

   _remove_exit_func(al_uninstall_keyboard);
}



/* al_get_keyboard:
 *  Return a pointer to an object representing the keyboard, that can
 *  be used as an event source.
 */
AL_KEYBOARD *al_get_keyboard(void)
{
   ASSERT(new_keyboard_driver);
   {
      AL_KEYBOARD *kbd = new_keyboard_driver->get_keyboard();
      ASSERT(kbd);

      return kbd;
   }
}



/* al_set_keyboard_leds:
 *  Overrides the state of the keyboard LED indicators.
 *  Set to -1 to return to default behavior.
 *  False is returned if the current keyboard driver cannot set LED indicators.
 */
bool al_set_keyboard_leds(int leds)
{
   ASSERT(new_keyboard_driver);

   if (new_keyboard_driver->set_leds)
      return new_keyboard_driver->set_leds(leds);

   return false;
}



/* al_get_keyboard_state: [primary thread]
 *  Save the state of the keyboard specified at the time the function
 *  is called into the structure pointed to by RET_STATE.
 */
void al_get_keyboard_state(AL_KBDSTATE *ret_state)
{
   ASSERT(new_keyboard_driver);
   ASSERT(ret_state);

   new_keyboard_driver->get_state(ret_state);
}



/* al_key_down:
 *  Return true if the key specified was held down in the state
 *  specified.
 */
bool al_key_down(AL_KBDSTATE *state, int keycode)
{
   return _AL_KBDSTATE_KEY_DOWN(*state, keycode);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
