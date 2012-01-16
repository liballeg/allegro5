#include "allegro5/allegro.h"
#include "allegro5/allegro_shader.h"
#include "allegro5/internal/aintern_shader.h"

#ifdef ALLEGRO_CFG_WANT_GLSL_SHADERS
#include "allegro5/allegro_opengl.h"
#include "shader_glsl.h"
#include "allegro5/allegro_shader_glsl.h"
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
#include "shader_hlsl.h"
#endif
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
#include <Cg/cg.h>
#include "shader_cg.h"
#endif

ALLEGRO_DEBUG_CHANNEL("shader")

#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
#define USING_OPENGL() (al_get_display_flags(al_get_current_display()) & ALLEGRO_OPENGL)
#else
#define USING_OPENGL() true
#endif

/* Function: al_create_shader
 */
ALLEGRO_SHADER *al_create_shader(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER *shader = NULL;

   if (platform == ALLEGRO_SHADER_AUTO) {
      if (USING_OPENGL()) {
         platform = ALLEGRO_SHADER_GLSL;
      }
      else {
         platform = ALLEGRO_SHADER_HLSL;
      }
   }
   else if (platform == ALLEGRO_SHADER_CG) {
      if (USING_OPENGL()) {
         platform |= ALLEGRO_SHADER_GLSL;
      }
      else {
         platform |= ALLEGRO_SHADER_HLSL;
      }
   }

#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   if (platform & ALLEGRO_SHADER_CG) {
      shader = _al_create_shader_cg(platform);
      shader->platform = platform;
      return shader;
   }
#endif
   if (platform & ALLEGRO_SHADER_GLSL) {
      shader = _al_create_shader_glsl(platform);
      shader->platform = platform;
      return shader;
   }
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   shader = _al_create_shader_hlsl(platform);
   shader->platform = platform;
#endif
   
   return shader;

}

/* Function: al_link_shader
 */
bool al_link_shader(ALLEGRO_SHADER *shader)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_link_shader_glsl(shader);
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_link_shader_hlsl(shader); // does nothing
#endif
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_link_shader_cg(shader); // does nothing
   }
#endif

   return false;
}

/* Function: al_attach_shader_source
 */
bool al_attach_shader_source(ALLEGRO_SHADER *shader, ALLEGRO_SHADER_TYPE type,
    const char *source)
{
   bool ret = false;

   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL)) {
      ret = _al_attach_shader_source_glsl(shader, type, source);
   }
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      ret = _al_attach_shader_source_cg(shader, type, source); 
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL) {
      ret = _al_attach_shader_source_hlsl(shader, type, source); 
   }
#endif

   return ret;
}

/* Function: al_attach_shader_source_file
 */
bool al_attach_shader_source_file(ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *filename)
{
   ALLEGRO_FILE *fp;
   ALLEGRO_USTR *str;
   bool ret;

   fp = al_fopen(filename, "r");
   if (!fp)
      return false;
   str = al_ustr_new("");
   for (;;) {
      char buf[512];
      size_t n;
      ALLEGRO_USTR_INFO info;

      n = al_fread(fp, buf, sizeof(buf));
      if (n <= 0)
         break;
      al_ustr_append(str, al_ref_buffer(&info, buf, n));
   }
   al_fclose(fp);
   ret = al_attach_shader_source(shader, type, al_cstr(str));
   al_ustr_free(str);
   return ret;
}

/* Function: al_get_shader_log
 */
const char *al_get_shader_log(ALLEGRO_SHADER *shader)
{
   ASSERT(shader);

   return (shader->log) ? al_cstr(shader->log) : "";
}

/* Function: al_use_shader
 */
void al_use_shader(ALLEGRO_SHADER *shader, bool use)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL)) {
      _al_use_shader_glsl(shader, use);
   }
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      _al_use_shader_cg(shader, use);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL) {
      _al_use_shader_hlsl(shader, use);
   }
#endif
}

/* Function: al_destroy_shader
 */
void al_destroy_shader(ALLEGRO_SHADER *shader)
{
   al_ustr_free(shader->vertex_copy);
   shader->vertex_copy = NULL;
   al_ustr_free(shader->pixel_copy);
   shader->pixel_copy = NULL;
   al_ustr_free(shader->log);
   shader->log = NULL;

   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL)) {
      _al_destroy_shader_glsl(shader);
   }
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      _al_destroy_shader_cg(shader);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL) {
      _al_destroy_shader_hlsl(shader);
   }
#endif
}

/* Function: al_set_shader_sampler
 */
bool al_set_shader_sampler(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_sampler_glsl(shader, name, bitmap, unit);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_sampler_cg(shader, name, bitmap, unit);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_sampler_hlsl(shader, name, bitmap, unit);
#endif

   return false;
}

/* Function: al_set_shader_matrix
 */
bool al_set_shader_matrix(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_matrix_glsl(shader, name, matrix);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_matrix_cg(shader, name, matrix);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_matrix_hlsl(shader, name, matrix);
#endif

   return false;
}

/* Function: al_set_shader_int
 */
bool al_set_shader_int(ALLEGRO_SHADER *shader, const char *name, int i)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_int_glsl(shader, name, i);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_int_cg(shader, name, i);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_int_hlsl(shader, name, i);
#endif

   return false;
}

/* Function: al_set_shader_float
 */
bool al_set_shader_float(ALLEGRO_SHADER *shader, const char *name, float f)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_float_glsl(shader, name, f);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_float_cg(shader, name, f);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_float_hlsl(shader, name, f);
#endif

   return false;
}

/* Function: al_set_shader_int_vector
 */
bool al_set_shader_int_vector(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, int *i, int num_elems)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_int_vector_glsl(shader, name, elem_size, i, num_elems);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_int_vector_cg(shader, name, elem_size, i, num_elems);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_int_vector_hlsl(shader, name, elem_size, i, num_elems);
#endif

   return false;
}

/* Function: al_set_shader_float_vector
 */
bool al_set_shader_float_vector(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, float *f, int num_elems)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_float_vector_glsl(shader, name, elem_size, f, num_elems);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_float_vector_cg(shader, name, elem_size, f, num_elems);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_float_vector_hlsl(shader, name, elem_size, f, num_elems);
#endif

   return false;
}

/* Function: al_set_shader_bool
 */
bool al_set_shader_bool(ALLEGRO_SHADER *shader, const char *name, bool b)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_bool_glsl(shader, name, b);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_bool_cg(shader, name, b);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_bool_hlsl(shader, name, b);
#endif

   return false;
}

/* Function: al_set_shader_vertex_array
 */
bool al_set_shader_vertex_array(ALLEGRO_SHADER *shader, float *v, int stride)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_vertex_array_glsl(shader, v, stride);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_vertex_array_cg(shader, v, stride);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_vertex_array_hlsl(shader, v, stride);
#endif

   return false;
}

/* Function: al_set_shader_color_array
 */
bool al_set_shader_color_array(ALLEGRO_SHADER *shader, unsigned char *c, int stride)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_color_array_glsl(shader, c, stride);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_color_array_cg(shader, c, stride);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_color_array_hlsl(shader, c, stride);
#endif

   return false;
}

/* Function: al_set_shader_texcoord_array
 */
bool al_set_shader_texcoord_array(ALLEGRO_SHADER *shader, float *u, int stride)
{
   if (!(shader->platform & ALLEGRO_SHADER_CG) && (shader->platform & ALLEGRO_SHADER_GLSL))
      return _al_set_shader_texcoord_array_glsl(shader, u, stride);
#ifdef ALLEGRO_CFG_WANT_CG_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_CG) {
      return _al_set_shader_texcoord_array_cg(shader, u, stride);
   }
#endif
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   else if (shader->platform & ALLEGRO_SHADER_HLSL)
      return _al_set_shader_texcoord_array_hlsl(shader, u, stride);
#endif

   return false;
}

/* Function: al_set_shader
 */
void al_set_shader(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader)
{
#ifdef ALLEGRO_CFG_WANT_GLSL_SHADERS
   if (USING_OPENGL()) {
      _al_set_shader_glsl(display, shader);
      return;
   }
#endif
/* XXX Cg */
#ifdef ALLEGRO_CFG_WANT_HLSL_SHADERS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      _al_set_shader_hlsl(display, shader);
      return;
   }
#endif
}

/* vim: set sts=3 sw=3 et: */
