#ifndef _SHADER_GLSL_H
#define _SHADER_GLSL_H

ALLEGRO_SHADER *_al_create_shader_glsl(ALLEGRO_SHADER_PLATFORM platform);
bool _al_attach_shader_source_glsl(ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *source);
bool _al_link_shader_glsl(ALLEGRO_SHADER *shader);
void _al_use_shader_glsl(ALLEGRO_SHADER *shader, bool use);
void _al_destroy_shader_glsl(ALLEGRO_SHADER *shader);

bool _al_set_shader_sampler_glsl(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit);
bool _al_set_shader_matrix_glsl(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix);
bool _al_set_shader_int_glsl(ALLEGRO_SHADER *shader, const char *name, int i);
bool _al_set_shader_float_glsl(ALLEGRO_SHADER *shader, const char *name, float f);
bool _al_set_shader_int_vector_glsl(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, int *i, int num_elems);
bool _al_set_shader_float_vector_glsl(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, float *f, int num_elems);
bool _al_set_shader_bool_glsl(ALLEGRO_SHADER *shader, const char *name, bool b);

bool _al_set_shader_vertex_array_glsl(ALLEGRO_SHADER *shader, float *v, int stride);
bool _al_set_shader_color_array_glsl(ALLEGRO_SHADER *shader, unsigned char *c, int stride);
bool _al_set_shader_texcoord_array_glsl(ALLEGRO_SHADER *shader, float *u, int stride);

void _al_set_shader_glsl(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader);

#endif
