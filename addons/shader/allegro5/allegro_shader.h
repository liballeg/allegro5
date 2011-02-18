#ifndef __al_included_allegro5_allegro_shader_h
#define __al_included_allegro5_allegro_shader_h

/* Type: ALLEGRO_SHADER
 */
typedef struct ALLEGRO_SHADER ALLEGRO_SHADER;

enum ALLEGRO_SHADER_TYPE {
   ALLEGRO_VERTEX_SHADER = 1,
   ALLEGRO_PIXEL_SHADER = 2
};

/* Enum: ALLEGRO_SHADER_TYPE
 */
typedef enum ALLEGRO_SHADER_TYPE ALLEGRO_SHADER_TYPE;

enum ALLEGRO_SHADER_PLATFORM {
   ALLEGRO_SHADER_AUTO = 0,
   ALLEGRO_SHADER_GLSL = 1,
   ALLEGRO_SHADER_HLSL = 2,
   ALLEGRO_SHADER_CG   = 4
};

/* Enum: ALLEGRO_SHADER_PLATFORM
 */
typedef enum ALLEGRO_SHADER_PLATFORM ALLEGRO_SHADER_PLATFORM;

#include "allegro5/allegro.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_SHADER_SRC
         #define _ALLEGRO_SHADER_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_SHADER_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_SHADER_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_SHADER_FUNC(type, name, args)      _ALLEGRO_SHADER_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_SHADER_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_SHADER_FUNC(type, name, args)      extern _ALLEGRO_SHADER_DLL type name args
#else
   #define ALLEGRO_SHADER_FUNC      AL_FUNC
#endif


ALLEGRO_SHADER_FUNC(ALLEGRO_SHADER *, al_create_shader, (ALLEGRO_SHADER_PLATFORM platform));
ALLEGRO_SHADER_FUNC(bool, al_attach_shader_source, (ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *source));
ALLEGRO_SHADER_FUNC(bool, al_attach_shader_source_file, (ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *filename));
ALLEGRO_SHADER_FUNC(bool, al_link_shader, (ALLEGRO_SHADER *shader));
ALLEGRO_SHADER_FUNC(const char *, al_get_shader_log, (ALLEGRO_SHADER *shader));
ALLEGRO_SHADER_FUNC(void, al_set_shader, (ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader));
ALLEGRO_SHADER_FUNC(void, al_use_shader, (ALLEGRO_SHADER *shader, bool use));
ALLEGRO_SHADER_FUNC(void, al_destroy_shader, (ALLEGRO_SHADER *shader));

ALLEGRO_SHADER_FUNC(bool, al_set_shader_sampler, (ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_matrix, (ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_int, (ALLEGRO_SHADER *shader, const char *name, int i));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_float, (ALLEGRO_SHADER *shader, const char *name, float f));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_int_vector, (ALLEGRO_SHADER *shader, const char *name,
   int elem_size, int *i, int num_elems));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_float_vector, (ALLEGRO_SHADER *shader, const char *name,
   int elem_size, float *f, int num_elems));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_bool, (ALLEGRO_SHADER *shader, const char *name, bool b));

ALLEGRO_SHADER_FUNC(bool, al_set_shader_vertex_array, (ALLEGRO_SHADER *shader, float *v, int stride));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_color_array, (ALLEGRO_SHADER *shader, unsigned char *c, int stride));
ALLEGRO_SHADER_FUNC(bool, al_set_shader_texcoord_array, (ALLEGRO_SHADER *shader, float *u, int stride));

#ifdef __cplusplus
}
#endif

#endif
