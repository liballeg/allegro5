#ifndef _ALLEGRO_SHADER_H
#define _ALLEGRO_SHADER_H

#include "allegro5/internal/aintern_shader_cfg.h"

struct ALLEGRO_SHADER;

typedef struct ALLEGRO_SHADER ALLEGRO_SHADER;

enum ALLEGRO_SHADER_TYPE {
   ALLEGRO_VERTEX_SHADER = 1,
   ALLEGRO_PIXEL_SHADER = 2
};

typedef enum ALLEGRO_SHADER_TYPE ALLEGRO_SHADER_TYPE;

enum ALLEGRO_SHADER_PLATFORM {
   ALLEGRO_SHADER_AUTO = 0,
   ALLEGRO_SHADER_GLSL = 1,
   ALLEGRO_SHADER_HLSL = 2,
   ALLEGRO_SHADER_CG   = 4
};

typedef enum ALLEGRO_SHADER_PLATFORM ALLEGRO_SHADER_PLATFORM;

#ifdef __cplusplus
extern "C" {
#endif

ALLEGRO_SHADER *al_create_shader(ALLEGRO_SHADER_PLATFORM platform);
bool al_attach_shader_source(ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *source);
bool al_link_shader(ALLEGRO_SHADER *shader);
void al_use_shader(ALLEGRO_SHADER *shader, bool use);
void al_destroy_shader(ALLEGRO_SHADER *shader);

bool al_set_shader_sampler(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit);
bool al_set_shader_matrix(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix);
bool al_set_shader_int(ALLEGRO_SHADER *shader, const char *name, int i);
bool al_set_shader_float(ALLEGRO_SHADER *shader, const char *name, float f);
bool al_set_shader_int_vector(ALLEGRO_SHADER *shader, const char *name,
   int size, int *i);
bool al_set_shader_float_vector(ALLEGRO_SHADER *shader, const char *name,
   int size, float *f);

bool al_set_shader_vertex_array(ALLEGRO_SHADER *shader, float *v, int stride);
bool al_set_shader_color_array(ALLEGRO_SHADER *shader, unsigned char *c, int stride);
bool al_set_shader_texcoord_array(ALLEGRO_SHADER *shader, float *u, int stride);

#ifdef ALLEGRO_CFG_OPENGL
#include <allegro5/allegro_opengl.h>
GLuint al_get_opengl_program_object(ALLEGRO_SHADER *shader);
#endif

#ifdef __cplusplus
}
#endif

#endif
