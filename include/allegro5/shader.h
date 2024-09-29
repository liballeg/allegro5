#ifndef __al_included_allegro5_shader_h
#define __al_included_allegro5_shader_h

#include "allegro5/base.h"
#include "allegro5/transformations.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_SHADER
 */
typedef struct A5O_SHADER A5O_SHADER;

enum A5O_SHADER_TYPE {
   A5O_VERTEX_SHADER = 1,
   A5O_PIXEL_SHADER = 2
};

/* Enum: A5O_SHADER_TYPE
 */
typedef enum A5O_SHADER_TYPE A5O_SHADER_TYPE;

enum A5O_SHADER_PLATFORM {
   A5O_SHADER_AUTO = 0,
   A5O_SHADER_GLSL = 1,
   A5O_SHADER_HLSL = 2,
   A5O_SHADER_AUTO_MINIMAL = 3,
   A5O_SHADER_GLSL_MINIMAL = 4,
   A5O_SHADER_HLSL_MINIMAL = 5,
   A5O_SHADER_HLSL_SM_3_0 = 6,
};

/* Enum: A5O_SHADER_PLATFORM
 */
typedef enum A5O_SHADER_PLATFORM A5O_SHADER_PLATFORM;

/* Shader variable names */
#define A5O_SHADER_VAR_COLOR             "al_color"
#define A5O_SHADER_VAR_POS               "al_pos"
#define A5O_SHADER_VAR_PROJVIEW_MATRIX   "al_projview_matrix"
#define A5O_SHADER_VAR_TEX               "al_tex"
#define A5O_SHADER_VAR_TEXCOORD          "al_texcoord"
#define A5O_SHADER_VAR_TEX_MATRIX        "al_tex_matrix"
#define A5O_SHADER_VAR_USER_ATTR         "al_user_attr_"
#define A5O_SHADER_VAR_USE_TEX           "al_use_tex"
#define A5O_SHADER_VAR_USE_TEX_MATRIX    "al_use_tex_matrix"
#define A5O_SHADER_VAR_ALPHA_TEST        "al_alpha_test"
#define A5O_SHADER_VAR_ALPHA_FUNCTION    "al_alpha_func"
#define A5O_SHADER_VAR_ALPHA_TEST_VALUE  "al_alpha_test_val"

AL_FUNC(A5O_SHADER *, al_create_shader, (A5O_SHADER_PLATFORM platform));
AL_FUNC(bool, al_attach_shader_source, (A5O_SHADER *shader,
   A5O_SHADER_TYPE type, const char *source));
AL_FUNC(bool, al_attach_shader_source_file, (A5O_SHADER *shader,
   A5O_SHADER_TYPE type, const char *filename));
AL_FUNC(bool, al_build_shader, (A5O_SHADER *shader));
AL_FUNC(const char *, al_get_shader_log, (A5O_SHADER *shader));
AL_FUNC(A5O_SHADER_PLATFORM, al_get_shader_platform, (A5O_SHADER *shader));
AL_FUNC(bool, al_use_shader, (A5O_SHADER *shader));
AL_FUNC(A5O_SHADER *, al_get_current_shader, (void));
AL_FUNC(void, al_destroy_shader, (A5O_SHADER *shader));

AL_FUNC(bool, al_set_shader_sampler, (const char *name, A5O_BITMAP *bitmap,
   int unit));
AL_FUNC(bool, al_set_shader_matrix, (const char *name,
   const A5O_TRANSFORM *matrix));
AL_FUNC(bool, al_set_shader_int, (const char *name, int i));
AL_FUNC(bool, al_set_shader_float, (const char *name, float f));
AL_FUNC(bool, al_set_shader_int_vector, (const char *name, int num_components,
   const int *i, int num_elems));
AL_FUNC(bool, al_set_shader_float_vector, (const char *name, int num_components,
   const float *f, int num_elems));
AL_FUNC(bool, al_set_shader_bool, (const char *name, bool b));

AL_FUNC(char const *, al_get_default_shader_source, (A5O_SHADER_PLATFORM platform,
   A5O_SHADER_TYPE type));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
