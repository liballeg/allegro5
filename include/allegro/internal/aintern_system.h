#ifndef ALLEGRO_INTERNAL_SYSTEM_NEW_H
#define ALLEGRO_INTERNAL_SYSTEM_NEW_H

#include "allegro/system_new.h"
#include "aintern_display.h"
#include "aintern_events.h"
#include "aintern_keyboard.h"
#include "allegro/internal/aintern_vector.h"

typedef struct AL_SYSTEM_INTERFACE AL_SYSTEM_INTERFACE;

struct AL_SYSTEM_INTERFACE
{
   int id;
   AL_SYSTEM *(*initialize)(int flags);
   AL_DISPLAY_INTERFACE *(*get_display_driver)(void);
   AL_KEYBOARD_DRIVER *(*get_keyboard_driver)(void);
   int (*get_num_display_modes)(void);
   AL_DISPLAY_MODE *(*get_display_mode)(int index, AL_DISPLAY_MODE *mode);
};

struct AL_SYSTEM
{
   AL_SYSTEM_INTERFACE *vt;
   _AL_VECTOR displays; /* Keep a list of all displays attached to us. */
};

// FIXME: later, there will be a general interface to find the best driver, and
// some platform specific initialization code will fill in the available system
// drivers.. for now, my dummy driver is the only available system driver,
// always
//AL_SYSTEM_INTERFACE *_al_system_xdummy_driver(void);
//AL_SYSTEM_INTERFACE *_al_system_d3d_driver(void);

AL_FUNC(void, _al_register_system_interfaces, (void));
AL_VAR(_AL_VECTOR, _al_system_interfaces);

#endif
