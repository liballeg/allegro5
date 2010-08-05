#ifndef __al_included_allegro5_allegro_image_h
#define __al_included_allegro5_allegro_image_h

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_IIO_SRC
         #define _ALLEGRO_IIO_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_IIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_IIO_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_IIO_FUNC(type, name, args)      _ALLEGRO_IIO_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_IIO_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_IIO_FUNC(type, name, args)      extern _ALLEGRO_IIO_DLL type name args
#else
   #define ALLEGRO_IIO_FUNC      AL_FUNC
#endif


#ifdef __cplusplus
extern "C" {
#endif


ALLEGRO_IIO_FUNC(bool, al_init_image_addon, (void));
ALLEGRO_IIO_FUNC(void, al_shutdown_image_addon, (void));
ALLEGRO_IIO_FUNC(uint32_t, al_get_allegro_image_version, (void));


#ifdef __cplusplus
}
#endif

#endif

