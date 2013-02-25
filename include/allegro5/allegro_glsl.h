#ifndef __al_included_allegro5_allegro_glsl_h
#define __al_included_allegro5_allegro_glsl_h

#include "allegro5/allegro_opengl.h"

#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(GLuint, al_get_opengl_program_object, (ALLEGRO_SHADER *shader));
AL_FUNC(char const *, al_get_default_glsl_vertex_shader, (void));
AL_FUNC(char const *, al_get_default_glsl_pixel_shader, (void));

#ifdef __cplusplus
}
#endif

#endif
