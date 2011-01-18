#ifndef ALLEGRO_SHADER_GLSL_H
#define ALLEGRO_SHADER_GLSL_H

#include "allegro5/allegro_opengl.h"

#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(GLuint, al_get_opengl_program_object, (ALLEGRO_SHADER *shader));

#ifdef __cplusplus
}
#endif

#endif
