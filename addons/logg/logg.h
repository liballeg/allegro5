#ifndef LOGG_H
#define LOGG_H

#include <allegro.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (defined LOGG_DYNAMIC) && (defined ALLEGRO_WINDOWS)
    #ifdef LOGG_SRC_BUILD
        #define _AOGG_DLL __declspec(dllexport)
    #else
        #define _AOGG_DLL __declspec(dllimport)
    #endif /* ALLEGRO_GL_SRC_BUILD */
#else
    #define _AOGG_DLL
#endif /* (defined LOGG_DYNAMIC) && (defined ALLEGRO_WINDOWS) */

#define AOGG_VAR(type, name) extern _AOGG_DLL type name

#if (defined LOGG_DYNAMIC) && (defined ALLEGRO_WINDOWS)
    #define AOGG_FUNC(type, name, args) extern _AOGG_DLL type __cdecl name args
#else
    #define AOGG_FUNC(type, name, args) extern type name args
#endif /* (defined LOGG_DYNAMIC) && (defined ALLEGRO_WINDOWS) */


#define LOGG_VERSION     1
#define LOGG_SUBVERSION  0
#define LOGG_VERSIONSTR  "1.0"

typedef struct LOGG_Stream LOGG_Stream;

AOGG_FUNC(SAMPLE*, logg_load,(const char* filename));
AOGG_FUNC(int, logg_get_buffer_size,(void));
AOGG_FUNC(void, logg_set_buffer_size,(int size));
AOGG_FUNC(LOGG_Stream*, logg_get_stream,(const char* filename,
		int volume, int pan, int loop));
AOGG_FUNC(int, logg_update_stream,(LOGG_Stream* s));
AOGG_FUNC(void, logg_destroy_stream,(LOGG_Stream* s));
AOGG_FUNC(void, logg_stop_stream,(LOGG_Stream* s));
AOGG_FUNC(int, logg_restart_stream,(LOGG_Stream* s));

#ifdef __cplusplus
}
#endif

#endif

