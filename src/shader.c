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
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_shader.h"

#ifdef ALLEGRO_CFG_SHADER_GLSL
#include "allegro5/allegro_opengl.h"
#endif

ALLEGRO_DEBUG_CHANNEL("shader")

#include "shader_source.inc"


static ALLEGRO_SHADER_PLATFORM resolve_platform(ALLEGRO_SHADER_PLATFORM platform)
{
   if (platform == ALLEGRO_SHADER_AUTO) {
      ALLEGRO_DISPLAY *display = al_get_current_display();
      ASSERT(display);
      if (al_get_display_flags(display) & ALLEGRO_OPENGL) {
         platform = ALLEGRO_SHADER_GLSL;
      }
      else {
         platform = ALLEGRO_SHADER_HLSL;
      }
   }

   return platform;
}

/* Function: al_create_shader
 */
ALLEGRO_SHADER *al_create_shader(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER *shader = NULL;

   platform = resolve_platform(platform);

   if (false) {
   }
#ifdef ALLEGRO_CFG_SHADER_GLSL
   else if (platform == ALLEGRO_SHADER_GLSL) {
      shader = _al_create_shader_glsl(platform);
   }
#endif
#ifdef ALLEGRO_CFG_SHADER_HLSL
   else if (platform == ALLEGRO_SHADER_HLSL) {
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

/* Function: al_get_shader_platform
 */
ALLEGRO_SHADER_PLATFORM al_get_shader_platform(ALLEGRO_SHADER *shader)
{
   ASSERT(shader);
   return shader->platform;
}

/* Function: al_use_shader
 */
bool al_use_shader(ALLEGRO_SHADER *shader)
{
   ALLEGRO_BITMAP *bmp = al_get_target_bitmap();

   if (!bmp) {
      ALLEGRO_WARN("No current target bitmap.\n");
      return false;
   }
   if (bmp->flags & ALLEGRO_MEMORY_BITMAP) {
      ALLEGRO_WARN("Target bitmap is memory bitmap.\n");
      return false;
   }
   ASSERT(bmp->display);

   if (shader) {
      if (shader->vt->use_shader(shader, bmp->display, true)) {
         _al_set_bitmap_shader_field(bmp, shader);
         ALLEGRO_DEBUG("use_shader succeeded\n");
         return true;
      }
      else {
         _al_set_bitmap_shader_field(bmp, NULL);
         ALLEGRO_ERROR("use_shader failed\n");
         if (bmp->display->default_shader) {
            bmp->display->default_shader->vt->use_shader(
               bmp->display->default_shader, bmp->display, true);
         }
         return false;
      }
   }
   else {
      if (bmp->shader) {
         bmp->shader->vt->unuse_shader(bmp->shader, bmp->display);
         _al_set_bitmap_shader_field(bmp, NULL);
      }
      if (bmp->display->default_shader) {
         bmp->display->default_shader->vt->use_shader(
            bmp->display->default_shader, bmp->display, true);
      }
      return true;
   }
}

/* Function: al_destroy_shader
 */
void al_destroy_shader(ALLEGRO_SHADER *shader)
{
   ALLEGRO_BITMAP *bmp;
   unsigned i;

   if (!shader)
      return;

   /* As a convenience, implicitly unuse the shader on the target bitmap
    * if currently used.
    */
   bmp = al_get_target_bitmap();
   if (bmp && _al_vector_contains(&shader->bitmaps, &bmp)) {
      ALLEGRO_DEBUG("implicitly unusing shader on target bitmap\n");
      al_use_shader(NULL);
   }

   al_ustr_free(shader->vertex_copy);
   shader->vertex_copy = NULL;
   al_ustr_free(shader->pixel_copy);
   shader->pixel_copy = NULL;
   al_ustr_free(shader->log);
   shader->log = NULL;

   /* Clear references to this shader from all bitmaps. */
   for (i = 0; i < _al_vector_size(&shader->bitmaps); i++) {
      ALLEGRO_BITMAP **slot = _al_vector_ref(&shader->bitmaps, i);
      ALLEGRO_BITMAP *bitmap = *slot;
      ASSERT(bitmap->shader == shader);
      bitmap->shader = NULL;
   }
   _al_vector_free(&shader->bitmaps);

   shader->vt->destroy_shader(shader);
}

/* Function: al_set_shader_sampler
 */
bool al_set_shader_sampler(const char *name,
   ALLEGRO_BITMAP *bitmap, int unit)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_sampler(shader, name, bitmap, unit);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_matrix
 */
bool al_set_shader_matrix(const char *name,
   ALLEGRO_TRANSFORM *matrix)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_matrix(shader, name, matrix);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_int
 */
bool al_set_shader_int(const char *name, int i)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_int(shader, name, i);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_float
 */
bool al_set_shader_float(const char *name, float f)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_float(shader, name, f);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_int_vector
 */
bool al_set_shader_int_vector(const char *name,
   int num_components, int *i, int num_elems)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_int_vector(shader, name, num_components, i, num_elems);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_float_vector
 */
bool al_set_shader_float_vector(const char *name,
   int num_components, float *f, int num_elems)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_float_vector(shader, name, num_components, f, num_elems);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_bool
 */
bool al_set_shader_bool(const char *name, bool b)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_bool(shader, name, b);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_vertex_array
 */
bool al_set_shader_vertex_array(float *v, int stride)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_vertex_array(shader, v, stride);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_color_array
 */
bool al_set_shader_color_array(unsigned char *c, int stride)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_color_array(shader, c, stride);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_set_shader_texcoord_array
 */
bool al_set_shader_texcoord_array(float *u, int stride)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   if ((bmp = al_get_target_bitmap()) != NULL) {
      if ((shader = bmp->shader) != NULL) {
         return shader->vt->set_shader_texcoord_array(shader, u, stride);
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
}

/* Function: al_get_default_shader_source
 */
char const *al_get_default_shader_source(ALLEGRO_SHADER_PLATFORM platform,
   ALLEGRO_SHADER_TYPE type)
{
   switch (resolve_platform(platform)) {
      case ALLEGRO_SHADER_GLSL:
#ifdef ALLEGRO_CFG_SHADER_GLSL
         switch (type) {
            case ALLEGRO_VERTEX_SHADER:
               return default_glsl_vertex_source;
            case ALLEGRO_PIXEL_SHADER:
               return default_glsl_pixel_source;
         }
#endif
         break;

      case ALLEGRO_SHADER_HLSL:
#ifdef ALLEGRO_CFG_SHADER_HLSL
         switch (type) {
            case ALLEGRO_VERTEX_SHADER:
               return default_hlsl_vertex_source;
            case ALLEGRO_PIXEL_SHADER:
               return default_hlsl_pixel_source;
         }
#endif
         break;

      case ALLEGRO_SHADER_AUTO:
         ASSERT(0);
   }
   return NULL;
}

void _al_set_bitmap_shader_field(ALLEGRO_BITMAP *bmp, ALLEGRO_SHADER *shader)
{
   ASSERT(bmp);

   if (bmp->shader != shader) {
      if (bmp->shader) {
         _al_unregister_shader_bitmap(bmp->shader, bmp);
      }
      bmp->shader = shader;
      if (bmp->shader) {
         _al_register_shader_bitmap(bmp->shader, bmp);
      }
   }
}

void _al_register_shader_bitmap(ALLEGRO_SHADER *shader, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_BITMAP **slot;
   ASSERT(shader);
   ASSERT(bmp);

   slot = _al_vector_alloc_back(&shader->bitmaps);
   *slot = bmp;
}

void _al_unregister_shader_bitmap(ALLEGRO_SHADER *shader, ALLEGRO_BITMAP *bmp)
{
   bool deleted;
   ASSERT(shader);
   ASSERT(bmp);

   deleted = _al_vector_find_and_delete(&shader->bitmaps, &bmp);
   ASSERT(deleted);
}

/* vim: set sts=3 sw=3 et: */
