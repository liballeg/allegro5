#ifndef _ALLEGRO_SHADER_INTERN_GLSL
#define _ALLEGRO_SHADER_INTERN_GLSL

#include "../allegro_shader.h"
#include "aintern_shader.h"

#include "allegro5/internal/aintern_vector.h"

struct ALLEGRO_SHADER_GLSL_S
{
	ALLEGRO_SHADER shader;
	GLuint vertex_shader;
	GLuint pixel_shader;
	GLuint program_object;
	_AL_VECTOR *deferred_sets;
};

typedef struct ALLEGRO_SHADER_GLSL_S ALLEGRO_SHADER_GLSL_S;

#endif
