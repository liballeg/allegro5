#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern_system.h"

static AL_SYSTEM *active;

/* Initialize the Allegro system. */
bool _al_init(void)
{
   AL_SYSTEM_INTERFACE *driver;

   // FIXME: detect the system driver to use
   driver = _al_system_d3d_driver();

   active = driver->initialize(0);

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

