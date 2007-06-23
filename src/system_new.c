#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern_system.h"
#include "allegro/internal/aintern_vector.h"

static AL_SYSTEM *active;

_AL_VECTOR _al_system_interfaces = _AL_VECTOR_INITIALIZER(AL_SYSTEM_INTERFACE *);
static _AL_VECTOR _user_system_interfaces = _AL_VECTOR_INITIALIZER(AL_SYSTEM_INTERFACE *);

bool al_register_system_driver(AL_SYSTEM_INTERFACE *sys_interface)
{
	AL_SYSTEM_INTERFACE **add = _al_vector_alloc_back(&_user_system_interfaces);
	if (!add)
		return false;
	*add = sys_interface;
	return true;
}

void al_unregister_system_driver(AL_SYSTEM_INTERFACE *sys_interface)
{
	_al_vector_find_and_delete(&_user_system_interfaces, sys_interface);
}

static AL_SYSTEM *find_system(_AL_VECTOR *vector)
{
   AL_SYSTEM_INTERFACE **sptr;
   AL_SYSTEM_INTERFACE *sys_interface;
   AL_SYSTEM *system;
   unsigned int i;

   for (i = 0; i < vector->_size; i++) {
   	sptr = _al_vector_ref(vector, i);
	sys_interface = *sptr;
	if ((system = sys_interface->initialize(0)) != NULL)
	   return system;
   }

   return NULL;
}

/* Initialize the Allegro system. */
bool _al_init(void)
{
   AL_SYSTEM_INTERFACE *driver = NULL;

   /* Register builtin system drivers */
   _al_register_system_interfaces();

   /* Check for a user-defined system driver first */
   active = find_system(&_user_system_interfaces);

   /* If a user-defined driver is not found, look for a builtin one */
   if (active == NULL) {
      active = find_system(&_al_system_interfaces);
   }

   if (active == NULL) {
      return false;
   }

#ifdef _al_tls_init
   bool _al_tls_init();
   if (!_al_tls_init())
      return false;
#endif

   return true;
}

/* Returns the currently active system driver. */
AL_SYSTEM *al_system_driver(void)
{
    return active;
}

