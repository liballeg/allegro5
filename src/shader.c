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
         bmp->shader = shader;
         ALLEGRO_DEBUG("use_shader succeeded\n");
         return true;
      }
      else {
         bmp->shader = NULL;
         ALLEGRO_ERROR("use_shader failed\n");
         return false;
      }
   }
   else {
      if (bmp->shader) {
         ASSERT(bmp->display);
         bmp->shader->vt->unuse_shader(bmp->shader, bmp->display);
         bmp->shader = NULL;
      }
      return true;
   }
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

/* Function: al_get_default_vertex_shader
 */
char const *al_get_default_vertex_shader(ALLEGRO_SHADER_PLATFORM platform)
{
   char const *source = NULL;
   platform = resolve_platform(platform);

   if (false) {
   }
#ifdef ALLEGRO_CFG_SHADER_GLSL
   else if (platform == ALLEGRO_SHADER_GLSL) {
      source =
         "attribute vec4 " ALLEGRO_SHADER_VAR_POS ";\n"
         "attribute vec4 " ALLEGRO_SHADER_VAR_COLOR ";\n"
         "attribute vec2 " ALLEGRO_SHADER_VAR_TEXCOORD ";\n"
         "uniform mat4 " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX ";\n"
         "uniform bool " ALLEGRO_SHADER_VAR_USE_TEX_MATRIX ";\n"
         "uniform mat4 " ALLEGRO_SHADER_VAR_TEX_MATRIX ";\n"
         "varying vec4 varying_color;\n"
         "varying vec2 varying_texcoord;\n"
         "void main()\n"
         "{\n"
         "  varying_color = " ALLEGRO_SHADER_VAR_COLOR ";\n"
         "  if (" ALLEGRO_SHADER_VAR_USE_TEX_MATRIX ") {\n"
         "    vec4 uv = " ALLEGRO_SHADER_VAR_TEX_MATRIX " * vec4(" ALLEGRO_SHADER_VAR_TEXCOORD ", 0, 1);\n"
         "    varying_texcoord = vec2(uv.x, uv.y);\n"
         "  }\n"
         "  else\n"
         "    varying_texcoord = " ALLEGRO_SHADER_VAR_TEXCOORD";\n"
         "  gl_Position = " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX " * " ALLEGRO_SHADER_VAR_POS ";\n"
         "}\n";
   }
#endif
#ifdef ALLEGRO_CFG_SHADER_HLSL
   else if (platform == ALLEGRO_SHADER_HLSL) {
      source =
         "struct VS_INPUT\n"
         "{\n"
         "   float4 Position  : POSITION0;\n"
         "   float2 TexCoord  : TEXCOORD0;\n"
         "   float4 Color     : TEXCOORD1;\n"
         "};\n"
         "struct VS_OUTPUT\n"
         "{\n"
         "   float4 Position  : POSITION0;\n"
         "   float4 Color     : COLOR0;\n"
         "   float2 TexCoord  : TEXCOORD0;\n"
         "};\n"
         "\n"
         "float4x4 al_projview_matrix;\n"
         "bool al_use_tex_matrix;\n"
         "float4x4 al_tex_matrix;\n"
         "\n"
         "VS_OUTPUT vs_main(VS_INPUT Input)\n"
         "{\n"
         "   VS_OUTPUT Output;\n"
         "   Output.Color = Input.Color;\n"
         "   if (al_use_tex_matrix) {\n"
         "      Output.TexCoord = mul(float4(Input.TexCoord, 1.0f, 0.0f), al_tex_matrix).xy;\n"
         "   }\n"
         "   else {\n"
         "      Output.TexCoord = Input.TexCoord;\n"
         "   }\n"
         "   Output.Position = mul(Input.Position, al_projview_matrix);\n"
         "   return Output;\n"
         "}\n";
   }
#endif
   return source;
}

/* Function: al_get_default_pixel_shader
 */
char const *al_get_default_pixel_shader(ALLEGRO_SHADER_PLATFORM platform)
{
   char const *source = NULL;
   platform = resolve_platform(platform);

   if (false) {
   }
#ifdef ALLEGRO_CFG_SHADER_GLSL
   else if (platform == ALLEGRO_SHADER_GLSL) {
      source =
#ifndef ALLEGRO_CFG_OPENGLES
         "#version 120\n"
#endif
         "#ifdef GL_ES\n"
         "precision mediump float;\n"
         "#endif\n"
         "uniform sampler2D " ALLEGRO_SHADER_VAR_TEX ";\n"
         "uniform bool " ALLEGRO_SHADER_VAR_USE_TEX ";\n"
         "varying vec4 varying_color;\n"
         "varying vec2 varying_texcoord;\n"
         "void main()\n"
         "{\n"
         "  if (" ALLEGRO_SHADER_VAR_USE_TEX ")\n"
         "    gl_FragColor = varying_color * texture2D(" ALLEGRO_SHADER_VAR_TEX ", varying_texcoord);\n"
         "  else\n"
         "    gl_FragColor = varying_color;\n"
         "}\n";
   }
#endif
#ifdef ALLEGRO_CFG_SHADER_HLSL
   else if (platform == ALLEGRO_SHADER_HLSL) {
      source =
         "bool al_use_tex;\n"
         "texture al_tex;\n"
         "sampler2D s = sampler_state {\n"
         "   texture = <al_tex>;\n"
         "};\n"
         "\n"
         "float4 ps_main(VS_OUTPUT Input) : COLOR0\n"
         "{\n"
         "   if (al_use_tex) {\n"
         "      return Input.Color * tex2D(s, Input.TexCoord);\n"
         "   }\n"
         "   else {\n"
         "      return Input.Color;\n"
         "   }\n"
         "}\n";
   }
#endif
   return source;
}

/* vim: set sts=3 sw=3 et: */
