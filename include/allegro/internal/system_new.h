#ifndef ALLEGRO_INTERNAL_SYSTEM_NEW_H
#define ALLEGRO_INTERNAL_SYSTEM_NEW_H

#include "../system_new.h"
#include "display_new.h"

typedef struct AL_SYSTEM_INTERFACE AL_SYSTEM_INTERFACE;

struct AL_SYSTEM_INTERFACE
{
   AL_SYSTEM *(*initialize)(int flags);
   AL_DISPLAY_INTERFACE *(*get_display_driver)(void);
};

struct AL_SYSTEM
{
   AL_SYSTEM_INTERFACE *vt;
};

AL_SYSTEM_INTERFACE *_al_system_xdummy_driver(void);

#endif
