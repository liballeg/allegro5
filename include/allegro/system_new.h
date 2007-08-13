#ifndef ALLEGRO_SYSTEM_NEW_H
#define ALLEGRO_SYSTEM_NEW_H

typedef struct ALLEGRO_SYSTEM ALLEGRO_SYSTEM;

bool _al_init(void);

#define al_init() { \
   if (!system_driver) \
      allegro_init(); \
   _al_init(); \
}

ALLEGRO_SYSTEM *al_system_driver(void);

#endif
