/*
 * SCANEXP - serves as a support to scan the header files.
 *
 *  By Michael Rickmann.
 */


#define AL_VAR(type, name)                   allexpvar name##_sym
#define AL_FUNCPTR(type, name, args)         allexpfpt name##_sym
#define AL_ARRAY(type, name)                 allexparr name##_sym
#define AL_FUNC(type, name, args)            allexpfun name##_sym
#define AL_INLINE(type, name, args, code)    allexpinl name##_sym


#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_COLOR8
#define ALLEGRO_COLOR16
#define ALLEGRO_COLOR24
#define ALLEGRO_COLOR32
#define ALLEGRO_NO_COLORCOPY


#if defined ALLEGRO_API

   #include "allegro.h"

#elif defined ALLEGRO_WINAPI

   #define ALLEGRO_H
   #define ALLEGRO_WINDOWS
   #include "winalleg.h"
   #include "allegro/platform/alwin.h"

#elif defined ALLEGRO_INTERNALS

   #define ALLEGRO_H
   #include "allegro/internal/aintern.h"

#endif

