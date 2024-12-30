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
 *      Touch input API.
 *
 *      By Michał Cichoń.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_NO_COMPATIBILITY

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_touch_input.h"


/* the active driver */
static ALLEGRO_TOUCH_INPUT_DRIVER *touch_input_driver = NULL;


/* Function: al_is_touch_input_installed
 */
bool al_is_touch_input_installed(void)
{
   return (touch_input_driver ? true : false);
}


/* Function: al_install_touch_input
 */
bool al_install_touch_input(void)
{
   if (touch_input_driver)
      return true;

   if (al_get_system_driver()->vt->get_touch_input_driver) {

      touch_input_driver = al_get_system_driver()->vt->get_touch_input_driver();
      if (touch_input_driver) {
         if (!touch_input_driver->init_touch_input()) {
            touch_input_driver = NULL;
            return false;
         }

         _al_add_exit_func(al_uninstall_touch_input, "al_uninstall_touch_input");
         return true;
      }
   }

   return false;
}


/* Function: al_uninstall_touch_input
 */
void al_uninstall_touch_input(void)
{
   if (!touch_input_driver)
      return;

   touch_input_driver->exit_touch_input();
   touch_input_driver = NULL;
}


static ALLEGRO_TOUCH_INPUT *get_touch_input(void)
{
   ALLEGRO_TOUCH_INPUT *touch_input;

   ASSERT(touch_input_driver);

   touch_input = touch_input_driver->get_touch_input();
   ASSERT(touch_input);

   return touch_input;
}


/* Function: al_get_touch_input_state
 */
void al_get_touch_input_state(ALLEGRO_TOUCH_INPUT_STATE *ret_state)
{
   ASSERT(touch_input_driver);
   ASSERT(ret_state);

   touch_input_driver->get_touch_input_state(ret_state);
}


/* Function: al_set_mouse_emulation_mode
 */
void al_set_mouse_emulation_mode(int mode)
{
   ASSERT(touch_input_driver);

   if (touch_input_driver->set_mouse_emulation_mode)
      touch_input_driver->set_mouse_emulation_mode(mode);
   else
      get_touch_input()->mouse_emulation_mode = mode;
}


/* Function: al_get_mouse_emulation_mode
 */
int al_get_mouse_emulation_mode(void)
{
   ASSERT(touch_input_driver);

   if (touch_input_driver->get_mouse_emulation_mode)
      return touch_input_driver->get_mouse_emulation_mode();
   else
      return get_touch_input()->mouse_emulation_mode;
}


/* Function: al_get_touch_input_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_touch_input_event_source(void)
{
   ALLEGRO_TOUCH_INPUT *touch_input = get_touch_input();

   return (touch_input) ? &touch_input->es : NULL;
}


/* Function: al_get_touch_input_mouse_emulation_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_touch_input_mouse_emulation_event_source(void)
{
   ALLEGRO_TOUCH_INPUT *touch_input = get_touch_input();

   return (touch_input) ? &touch_input->mouse_emulation_es : NULL;
}
