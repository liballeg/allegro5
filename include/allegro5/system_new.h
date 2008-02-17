#ifndef ALLEGRO_SYSTEM_NEW_H
#define ALLEGRO_SYSTEM_NEW_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_SYSTEM ALLEGRO_SYSTEM;

AL_FUNC(bool, _al_init, (void));

#ifdef ALLEGRO_MINGW32
static inline bool al_init(void)
{
   if (!system_driver) {
      if (allegro_init())
         return false;
   }
   return _al_init();
}
#else
AL_INLINE(bool, al_init, (void),
{
   if (!system_driver) {
      if (allegro_init())
         return false;
   }
   return _al_init();
})
#endif

ALLEGRO_SYSTEM *al_system_driver(void);

#ifdef __cplusplus
   }
#endif

#endif
