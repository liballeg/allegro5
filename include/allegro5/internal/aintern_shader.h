#ifndef __al_included_allegro5_internal_aintern_shader_h
#define __al_included_allegro5_internal_aintern_shader_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALLEGRO_SHADER_INTERFACE ALLEGRO_SHADER_INTERFACE;

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

struct ALLEGRO_SHADER
{
   ALLEGRO_USTR *vertex_copy;
   ALLEGRO_USTR *pixel_copy;
   ALLEGRO_USTR *log;
   ALLEGRO_SHADER_PLATFORM platform;
   ALLEGRO_SHADER_INTERFACE *vt;
};

#ifdef ALLEGRO_CFG_SHADER_GLSL
ALLEGRO_SHADER *_al_create_shader_glsl(ALLEGRO_SHADER_PLATFORM platform);
void _al_set_shader_glsl(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader);
#endif

#ifdef ALLEGRO_CFG_SHADER_HLSL
ALLEGRO_SHADER *_al_create_shader_hlsl(ALLEGRO_SHADER_PLATFORM platform);
void _al_set_shader_hlsl(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader);
#endif


#ifdef __cplusplus
}
#endif

#endif

/* vim: set sts=3 sw=3 et: */
