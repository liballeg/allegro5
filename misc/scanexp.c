/*
 * SCANEXP - serves as a support to scan the header files.
 *
 *  By Michael Rickmann.
 */

/* don't include non-standard header files */
#define SCAN_DEPEND


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
#define ALLEGRO_WINDOWS
/* Make OpenGL mandatory. A library without OpenGL lacks many
 * exports specific to OpenGL, which makes it ABI incompatible. */
#define ALLEGRO_CFG_OPENGL
/* Make D3D mandatory. */
#define ALLEGRO_CFG_D3D


#if defined ALLEGRO_API

   #include "allegro5/allegro5.h"
   #include "allegro5/a5_opengl.h"
   #include "allegro5/a5_direct3d.h"

#elif defined ALLEGRO_WINAPI

   #define ALLEGRO_H
   #include "allegro5/platform/alwin.h"

#elif defined ALLEGRO_INTERNALS

   #define ALLEGRO_H
   #include "allegro5/internal/aintern.h"
   #include "allegro5/internal/aintern_dtor.h"
   #include "allegro5/internal/aintern_events.h"
   #include "allegro5/internal/aintern_memory.h"
   #include "allegro5/internal/aintern_vector.h"

#endif

