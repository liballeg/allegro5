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
   void (*shutdown_system)(void);
};

struct AL_SYSTEM
{
   AL_SYSTEM_INTERFACE *vt;
   _AL_VECTOR displays; /* Keep a list of all displays attached to us. */
};


AL_FUNC(void, _al_register_system_interfaces, (void));
AL_VAR(_AL_VECTOR, _al_system_interfaces);

AL_FUNC(void, _al_generate_integer_unmap_table, (void));

#endif
