#ifndef __al_included_allegro5_internal_aintern_shader_h
#define __al_included_allegro5_internal_aintern_shader_h

#include "allegro5/internal/aintern_list.h"
#include "allegro5/internal/aintern_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct A5O_SHADER_INTERFACE A5O_SHADER_INTERFACE;

struct A5O_SHADER_INTERFACE
{
   bool (*attach_shader_source)(A5O_SHADER *shader,
         A5O_SHADER_TYPE type, const char *source);
   bool (*build_shader)(A5O_SHADER *shader);
   bool (*use_shader)(A5O_SHADER *shader, A5O_DISPLAY *dpy,
            bool set_projview_matrix_from_display);
   void (*unuse_shader)(A5O_SHADER *shader, A5O_DISPLAY *dpy);
   void (*destroy_shader)(A5O_SHADER *shader);
   void (*on_lost_device)(A5O_SHADER *shader);
   void (*on_reset_device)(A5O_SHADER *shader);

   bool (*set_shader_sampler)(A5O_SHADER *shader, const char *name,
         A5O_BITMAP *bitmap, int unit);
   bool (*set_shader_matrix)(A5O_SHADER *shader, const char *name,
         const A5O_TRANSFORM *matrix);
   bool (*set_shader_int)(A5O_SHADER *shader, const char *name, int i);
   bool (*set_shader_float)(A5O_SHADER *shader, const char *name, float f);
   bool (*set_shader_int_vector)(A5O_SHADER *shader, const char *name,
         int elem_size, const int *i, int num_elems);
   bool (*set_shader_float_vector)(A5O_SHADER *shader, const char *name,
         int elem_size, const float *f, int num_elems);
   bool (*set_shader_bool)(A5O_SHADER *shader, const char *name, bool b);
};

struct A5O_SHADER
{
   A5O_USTR *vertex_copy;
   A5O_USTR *pixel_copy;
   A5O_USTR *log;
   A5O_SHADER_PLATFORM platform;
   A5O_SHADER_INTERFACE *vt;
   _AL_VECTOR bitmaps; /* of A5O_BITMAP pointers */
   _AL_LIST_ITEM *dtor_item;
};

/* In most cases you should use _al_set_bitmap_shader_field. */
void _al_set_bitmap_shader_field(A5O_BITMAP *bmp, A5O_SHADER *shader);
void _al_register_shader_bitmap(A5O_SHADER *shader, A5O_BITMAP *bmp);
void _al_unregister_shader_bitmap(A5O_SHADER *shader, A5O_BITMAP *bmp);

A5O_SHADER *_al_create_default_shader(A5O_DISPLAY *display);

#ifdef A5O_CFG_SHADER_GLSL
A5O_SHADER *_al_create_shader_glsl(A5O_SHADER_PLATFORM platform);
void _al_set_shader_glsl(A5O_DISPLAY *display, A5O_SHADER *shader);
#endif

#ifdef A5O_CFG_SHADER_HLSL
A5O_SHADER *_al_create_shader_hlsl(A5O_SHADER_PLATFORM platform, int shader_model);
void _al_set_shader_hlsl(A5O_DISPLAY *display, A5O_SHADER *shader);
#endif


#ifdef __cplusplus
}
#endif

#endif

/* vim: set sts=3 sw=3 et: */
