/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Shader API.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_shader.h"

#ifdef ALLEGRO_CFG_SHADER_GLSL
#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_glsl.h"
#endif

ALLEGRO_DEBUG_CHANNEL("shader")

#ifdef ALLEGRO_CFG_SHADER_HLSL
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

   if (false) {
   }
#ifdef ALLEGRO_CFG_SHADER_GLSL
   else if (platform & ALLEGRO_SHADER_GLSL) {
      shader = _al_create_shader_glsl(platform);
   }
#endif
#ifdef ALLEGRO_CFG_SHADER_HLSL
   else if (platform & ALLEGRO_SHADER_HLSL) {
      shader = _al_create_shader_hlsl(platform);
   }
#endif

   if (shader) {
      ASSERT(shader->platform);
      ASSERT(shader->vt);
   }
   else {
      ALLEGRO_WARN("Failed to create shader\n");
   }
   return shader;
}

/* Function: al_link_shader
 */
bool al_link_shader(ALLEGRO_SHADER *shader)
{
   ASSERT(shader);
   return shader->vt->link_shader(shader);
}

/* Function: al_attach_shader_source
 */
bool al_attach_shader_source(ALLEGRO_SHADER *shader, ALLEGRO_SHADER_TYPE type,
    const char *source)
{
   ASSERT(shader);
   return shader->vt->attach_shader_source(shader, type, source);
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
   if (!fp) {
      ALLEGRO_WARN("Failed to open %s\n", filename);
      al_ustr_free(shader->log);
      shader->log = al_ustr_newf("Failed to open %s", filename);
      return false;
   }
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
   ASSERT(shader);
   shader->vt->use_shader(shader, use);
}

/* Function: al_destroy_shader
 */
void al_destroy_shader(ALLEGRO_SHADER *shader)
{
   if (!shader)
      return;

   al_ustr_free(shader->vertex_copy);
   shader->vertex_copy = NULL;
   al_ustr_free(shader->pixel_copy);
   shader->pixel_copy = NULL;
   al_ustr_free(shader->log);
   shader->log = NULL;

   shader->vt->destroy_shader(shader);
}

/* Function: al_set_shader_sampler
 */
bool al_set_shader_sampler(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit)
{
   ASSERT(shader);
   return shader->vt->set_shader_sampler(shader, name, bitmap, unit);
}

/* Function: al_set_shader_matrix
 */
bool al_set_shader_matrix(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix)
{
   ASSERT(shader);
   return shader->vt->set_shader_matrix(shader, name, matrix);
}

/* Function: al_set_shader_int
 */
bool al_set_shader_int(ALLEGRO_SHADER *shader, const char *name, int i)
{
   ASSERT(shader);
   return shader->vt->set_shader_int(shader, name, i);
}

/* Function: al_set_shader_float
 */
bool al_set_shader_float(ALLEGRO_SHADER *shader, const char *name, float f)
{
   ASSERT(shader);
   return shader->vt->set_shader_float(shader, name, f);
}

/* Function: al_set_shader_int_vector
 */
bool al_set_shader_int_vector(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, int *i, int num_elems)
{
   ASSERT(shader);
   return shader->vt->set_shader_int_vector(shader, name, elem_size, i, num_elems);
}

/* Function: al_set_shader_float_vector
 */
bool al_set_shader_float_vector(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, float *f, int num_elems)
{
   ASSERT(shader);
   return shader->vt->set_shader_float_vector(shader, name, elem_size, f, num_elems);
}

/* Function: al_set_shader_bool
 */
bool al_set_shader_bool(ALLEGRO_SHADER *shader, const char *name, bool b)
{
   ASSERT(shader);
   return shader->vt->set_shader_bool(shader, name, b);
}

/* Function: al_set_shader_vertex_array
 */
bool al_set_shader_vertex_array(ALLEGRO_SHADER *shader, float *v, int stride)
{
   ASSERT(shader);
   return shader->vt->set_shader_vertex_array(shader, v, stride);
}

/* Function: al_set_shader_color_array
 */
bool al_set_shader_color_array(ALLEGRO_SHADER *shader, unsigned char *c, int stride)
{
   ASSERT(shader);
   return shader->vt->set_shader_color_array(shader, c, stride);
}

/* Function: al_set_shader_texcoord_array
 */
bool al_set_shader_texcoord_array(ALLEGRO_SHADER *shader, float *u, int stride)
{
   ASSERT(shader);
   return shader->vt->set_shader_texcoord_array(shader, u, stride);
}

/* Function: al_set_shader
 */
void al_set_shader(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader)
{
   ASSERT(display);
   ASSERT(shader);
   shader->vt->set_shader(display, shader);
}

/* vim: set sts=3 sw=3 et: */
