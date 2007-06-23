#ifndef ALLEGRO_SYSTEM_NEW_H
#define ALLEGRO_SYSTEM_NEW_H

typedef struct AL_SYSTEM AL_SYSTEM;

bool _al_init(void);

#define al_init() { \
   if (!system_driver) \
      allegro_init(); \
   _al_init(); \
}

AL_SYSTEM *al_system_driver(void);

#endif
