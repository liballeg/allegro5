#ifndef ALLEGRO_SYSTEM_NEW_H
#define ALLEGRO_SYSTEM_NEW_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_SYSTEM ALLEGRO_SYSTEM;

AL_FUNC(bool, al_init, (void));
AL_FUNC(void, allegro_exit, (void));
AL_FUNC(ALLEGRO_SYSTEM *, al_system_driver, (void));

#ifdef __cplusplus
   }
#endif

#endif
