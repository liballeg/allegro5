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
 *      New system driver.
 *
 *      By Elias Pschernig.
 *
 *      Modified by Trent Gamblin.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_vector.h"



static ALLEGRO_SYSTEM *active_sysdrv;

_AL_VECTOR _al_system_interfaces = _AL_VECTOR_INITIALIZER(ALLEGRO_SYSTEM_INTERFACE *);
static _AL_VECTOR _user_system_interfaces = _AL_VECTOR_INITIALIZER(ALLEGRO_SYSTEM_INTERFACE *);



bool al_register_system_driver(ALLEGRO_SYSTEM_INTERFACE *sys_interface)
{
   ALLEGRO_SYSTEM_INTERFACE **add = _al_vector_alloc_back(&_user_system_interfaces);
   if (!add)
      return false;
   *add = sys_interface;
   return true;
}



void al_unregister_system_driver(ALLEGRO_SYSTEM_INTERFACE *sys_interface)
{
   _al_vector_find_and_delete(&_user_system_interfaces, &sys_interface);
}



static ALLEGRO_SYSTEM *find_system(_AL_VECTOR *vector)
{
   ALLEGRO_SYSTEM_INTERFACE **sptr;
   ALLEGRO_SYSTEM_INTERFACE *sys_interface;
   ALLEGRO_SYSTEM *system;
   unsigned int i;

   for (i = 0; i < vector->_size; i++) {
      sptr = _al_vector_ref(vector, i);
      sys_interface = *sptr;
      if ((system = sys_interface->initialize(0)) != NULL)
         return system;
   }

   return NULL;
}



void _al_exit(void)
{
   if (screen && _al_current_display != NULL) {
      al_destroy_display(_al_current_display);
      al_set_current_display(NULL);
   }
   if (active_sysdrv) {
      if (active_sysdrv->vt && active_sysdrv->vt->shutdown_system)
         active_sysdrv->vt->shutdown_system();
      active_sysdrv = NULL;
   }
}



/* _al_init:
 *  Initialize the Allegro system.
 */
bool _al_init(void)
{
   if (active_sysdrv) {
      return true;
   }

   /* Register builtin system drivers */
   _al_register_system_interfaces();

   /* Check for a user-defined system driver first */
   active_sysdrv = find_system(&_user_system_interfaces);

   /* If a user-defined driver is not found, look for a builtin one */
   if (active_sysdrv == NULL) {
      active_sysdrv = find_system(&_al_system_interfaces);
   }

   if (active_sysdrv == NULL) {
      return false;
   }

   _al_generate_integer_unmap_table();

   _add_exit_func(_al_exit, "Old-API exit function for new API"); 

   return true;
}



/* al_system_driver:
 *  Returns the currently active_sysdrv system driver.
 */
ALLEGRO_SYSTEM *al_system_driver(void)
{
   return active_sysdrv;
}
