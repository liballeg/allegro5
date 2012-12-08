#ifndef _AL_SHADER_H
#define _AL_SHADER_H

#include "allegro5/internal/aintern_shader.h"

#ifdef __cplusplus
extern "C" {
#endif


struct ALLEGRO_SHADER_INTERFACE
{
   bool (*link_shader)(ALLEGRO_SHADER *shader);
   bool (*attach_shader_source)(ALLEGRO_SHADER *shader,
         ALLEGRO_SHADER_TYPE type, const char *source);
   void (*use_shader)(ALLEGRO_SHADER *shader, bool use);
   void (*destroy_shader)(ALLEGRO_SHADER *shader);

   bool (*set_shader_sampler)(ALLEGRO_SHADER *shader, const char *name,
         ALLEGRO_BITMAP *bitmap, int unit);
   bool (*set_shader_matrix)(ALLEGRO_SHADER *shader, const char *name,
         ALLEGRO_TRANSFORM *matrix);
   bool (*set_shader_int)(ALLEGRO_SHADER *shader, const char *name, int i);
   bool (*set_shader_float)(ALLEGRO_SHADER *shader, const char *name, float f);
   bool (*set_shader_int_vector)(ALLEGRO_SHADER *shader, const char *name,
         int elem_size, int *i, int num_elems);
   bool (*set_shader_float_vector)(ALLEGRO_SHADER *shader, const char *name,
         int elem_size, float *f, int num_elems);
   bool (*set_shader_bool)(ALLEGRO_SHADER *shader, const char *name, bool b);
   bool (*set_shader_vertex_array)(ALLEGRO_SHADER *shader, float *v, int stride);
   bool (*set_shader_color_array)(ALLEGRO_SHADER *shader, unsigned char *c, int stride);
   bool (*set_shader_texcoord_array)(ALLEGRO_SHADER *shader, float *u, int stride);

   void (*set_shader)(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader);
};


#ifdef __cplusplus
}
#endif

#endif

/* vim: set sts=3 sw=3 et: */
