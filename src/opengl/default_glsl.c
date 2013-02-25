#include "allegro5/allegro.h"
#include "allegro5/allegro_shader_glsl.h"

/* Function: al_get_default_glsl_vertex_shader
 */
char const *al_get_default_glsl_vertex_shader(void)
{
   return
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

/* Function: al_get_default_glsl_pixel_shader
 */
char const *al_get_default_glsl_pixel_shader(void)
{
   return
#ifdef ALLEGRO_CFG_OPENGLES
      "precision mediump float;\n"
#endif
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

/* vim: set sts=4 sw=4 et: */
