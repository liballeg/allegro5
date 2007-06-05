#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern_system.h"

static AL_SYSTEM *active;

/* Initialize the Allegro system. */
void al_init(void)
{
    // FIXME: detect the system driver to use
    AL_SYSTEM_INTERFACE *driver = _al_system_d3d_driver();

    active = driver->initialize(0);
}

/* Returns the currently active system driver. */
AL_SYSTEM *al_system_driver(void)
{
    return active;
}

