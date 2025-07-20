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
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_shader.h"
#include "allegro5/internal/aintern_system.h"

#ifdef ALLEGRO_CFG_SHADER_GLSL
#include "allegro5/allegro_opengl.h"
#endif

ALLEGRO_DEBUG_CHANNEL("shader")

#include "shader_source.inc"

static ALLEGRO_SHADER_PLATFORM resolve_platform(ALLEGRO_DISPLAY *display,
                                                ALLEGRO_SHADER_PLATFORM platform)
{
   if (platform == ALLEGRO_SHADER_AUTO) {
      ASSERT(display);
      if (display->flags & ALLEGRO_OPENGL) {
         platform = ALLEGRO_SHADER_GLSL;
      }
      else {
         platform = ALLEGRO_SHADER_HLSL;
      }
   }
   if (platform == ALLEGRO_SHADER_AUTO_MINIMAL) {
      ASSERT(display);
      if (display->flags & ALLEGRO_OPENGL) {
         platform = ALLEGRO_SHADER_GLSL_MINIMAL;
      }
      else {
         platform = ALLEGRO_SHADER_HLSL_MINIMAL;
      }
   }

   return platform;
}

/* Function: al_create_shader
 */
ALLEGRO_SHADER *al_create_shader(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER *shader = NULL;

   platform = resolve_platform(al_get_current_display(), platform);

   switch (platform) {
#ifdef ALLEGRO_CFG_SHADER_GLSL
      case ALLEGRO_SHADER_GLSL:
      case ALLEGRO_SHADER_GLSL_MINIMAL:
         shader = _al_create_shader_glsl(platform);
         break;
#endif
#ifdef ALLEGRO_CFG_SHADER_HLSL
      case ALLEGRO_SHADER_HLSL:
      case ALLEGRO_SHADER_HLSL_MINIMAL:
         shader = _al_create_shader_hlsl(platform, 2);
         break;
      case ALLEGRO_SHADER_HLSL_SM_3_0:
         shader = _al_create_shader_hlsl(platform, 3);
         break;
#endif
      case ALLEGRO_SHADER_AUTO:
      case ALLEGRO_SHADER_AUTO_MINIMAL:
         ASSERT(0);
         break;
      default:
         break;
   }

   if (shader) {
      ASSERT(shader->platform);
      ASSERT(shader->vt);
      shader->dtor_item = _al_register_destructor(_al_dtor_list, "shader", shader,
         (void (*)(void *))al_destroy_shader);
   }
   else {
      ALLEGRO_WARN("Failed to create shader\n");
   }
   return shader;
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

/* Function: al_build_shader
 */
bool al_build_shader(ALLEGRO_SHADER *shader)
{
   ASSERT(shader);
   return shader->vt->build_shader(shader);
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
   ALLEGRO_DISPLAY *disp;

   if (!bmp) {
      ALLEGRO_WARN("No current target bitmap.\n");
      return false;
   }
   if (al_get_bitmap_flags(bmp) & ALLEGRO_MEMORY_BITMAP) {
      ALLEGRO_WARN("Target bitmap is memory bitmap.\n");
      return false;
   }
   disp = _al_get_bitmap_display(bmp);
   ASSERT(disp);

   if (shader) {
      if (shader->vt->use_shader(shader, disp, true)) {
         _al_set_bitmap_shader_field(bmp, shader);
         ALLEGRO_DEBUG("use_shader succeeded\n");
         return true;
      }
      else {
         _al_set_bitmap_shader_field(bmp, NULL);
         ALLEGRO_ERROR("use_shader failed\n");
         if (disp->default_shader) {
            disp->default_shader->vt->use_shader(
               disp->default_shader, disp, true);
         }
         return false;
      }
   }
   else {
      if (bmp->shader) {
         bmp->shader->vt->unuse_shader(bmp->shader, disp);
         _al_set_bitmap_shader_field(bmp, NULL);
      }
      if (disp->default_shader) {
         disp->default_shader->vt->use_shader(
            disp->default_shader, disp, true);
      }
      return true;
   }
}

/* Function: al_get_current_shader
*/
ALLEGRO_SHADER *al_get_current_shader()
{
   ALLEGRO_BITMAP* bmp = al_get_target_bitmap();

   if (bmp != NULL) {
      return bmp->shader;
   }
   else {
      return NULL;
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

   _al_unregister_destructor(_al_dtor_list, shader->dtor_item);

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
   const ALLEGRO_TRANSFORM *matrix)
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
   int num_components, const int *i, int num_elems)
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
   int num_components, const float *f, int num_elems)
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

/* Function: al_get_default_shader_source
 */
char const *al_get_default_shader_source(ALLEGRO_SHADER_PLATFORM platform,
   ALLEGRO_SHADER_TYPE type)
{
   (void)type;
   bool use_gl3_shader = false;
   ALLEGRO_DISPLAY *display = al_get_current_display();
#ifdef ALLEGRO_MACOSX
   /* Apple's glsl implementation supports either 1.20 shaders, or strictly
    * versioned 3.2+ shaders which do not use deprecated features.
    */
   if (display && (display->flags & ALLEGRO_OPENGL_3_0)) {
      use_gl3_shader = true;
   }
#endif
   if (display && (display->flags & (ALLEGRO_OPENGL_3_0 | ALLEGRO_OPENGL_FORWARD_COMPATIBLE))) {
      use_gl3_shader = true;
   }

   switch (resolve_platform(al_get_current_display(), platform)) {
      case ALLEGRO_SHADER_GLSL:
#ifdef ALLEGRO_CFG_SHADER_GLSL
         switch (type) {
            case ALLEGRO_VERTEX_SHADER:
               return use_gl3_shader ? default_glsl_vertex_source_gl3 : default_glsl_vertex_source;
            case ALLEGRO_PIXEL_SHADER:
               return use_gl3_shader ? default_glsl_pixel_source_gl3 : default_glsl_pixel_source;
         }
#endif
         break;
      case ALLEGRO_SHADER_GLSL_MINIMAL:
#ifdef ALLEGRO_CFG_SHADER_GLSL
         switch (type) {
            case ALLEGRO_VERTEX_SHADER:
               return use_gl3_shader ? default_glsl_vertex_source_gl3 : default_glsl_vertex_source;
            case ALLEGRO_PIXEL_SHADER:
               return use_gl3_shader ? default_glsl_minimal_pixel_source_gl3 : default_glsl_minimal_pixel_source;
         }
#endif
         break;
      case ALLEGRO_SHADER_HLSL:
      case ALLEGRO_SHADER_HLSL_MINIMAL:
      case ALLEGRO_SHADER_HLSL_SM_3_0:
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
      case ALLEGRO_SHADER_AUTO_MINIMAL:
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

ALLEGRO_SHADER *_al_create_default_shader(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SHADER *shader;
   ALLEGRO_SHADER_PLATFORM platform = resolve_platform(
      display,
      display->extra_settings.settings[ALLEGRO_DEFAULT_SHADER_PLATFORM]
   );

   _al_push_destructor_owner();
   shader = al_create_shader(platform);
   _al_pop_destructor_owner();

   if (!shader) {
      ALLEGRO_ERROR("Error creating default shader.\n");
      return false;
   }
   if (!al_attach_shader_source(shader, ALLEGRO_VERTEX_SHADER,
         al_get_default_shader_source(platform, ALLEGRO_VERTEX_SHADER))) {
      ALLEGRO_ERROR("al_attach_shader_source for vertex shader failed: %s\n",
         al_get_shader_log(shader));
      goto fail;
   }
   if (!al_attach_shader_source(shader, ALLEGRO_PIXEL_SHADER,
         al_get_default_shader_source(platform, ALLEGRO_PIXEL_SHADER))) {
      ALLEGRO_ERROR("al_attach_shader_source for pixel shader failed: %s\n",
         al_get_shader_log(shader));
      goto fail;
   }
   if (!al_build_shader(shader)) {
      ALLEGRO_ERROR("al_build_shader failed: %s\n", al_get_shader_log(shader));
      goto fail;
   }
   return shader;

fail:
   al_destroy_shader(shader);
   return NULL;
}

/* vim: set sts=3 sw=3 et: */
