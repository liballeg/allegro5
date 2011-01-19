#ifndef __al_included_allegro5_allegro_shader_glsl_h
#define __al_included_allegro5_allegro_shader_glsl_h

#include "allegro5/allegro_shader.h"
#include "allegro5/allegro_opengl.h"

#ifdef __cplusplus
extern "C" {
#endif

ALLEGRO_SHADER_FUNC(GLuint, al_get_opengl_program_object, (ALLEGRO_SHADER *shader));

#ifdef __cplusplus
}
#endif

#endif
