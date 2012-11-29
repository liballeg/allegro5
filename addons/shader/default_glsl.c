#include "allegro5/allegro.h"
#include "allegro5/allegro_shader_glsl.h"

/* Function: al_get_default_glsl_vertex_shader
 */
char const *al_get_default_glsl_vertex_shader(void)
{
   return
      "attribute vec4 pos;\n"
      "attribute vec4 color;\n"
      "attribute vec2 texcoord;\n"
      "uniform mat4 projview_matrix;\n"
      "uniform bool use_tex_matrix;\n"
      "uniform mat4 tex_matrix;\n"
      "varying vec4 varying_color;\n"
      "varying vec2 varying_texcoord;\n"
      "void main()\n"
      "{\n"
      "  varying_color = color;\n"
      "  if (use_tex_matrix) {\n"
      "    vec4 uv = tex_matrix * vec4(texcoord, 0, 1);\n"
      "    varying_texcoord = vec2(uv.x, uv.y);\n"
      "  }\n"
      "  else\n"
      "    varying_texcoord = texcoord;\n"
      "  gl_Position = projview_matrix * pos;\n"
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
      "uniform sampler2D tex;\n"
      "uniform bool use_tex;\n"
      "varying vec4 varying_color;\n"
      "varying vec2 varying_texcoord;\n"
      "void main()\n"
      "{\n"
      "  if (use_tex)\n"
      "    gl_FragColor = varying_color * texture2D(tex, varying_texcoord);\n"
      "  else\n"
      "    gl_FragColor = varying_color;\n"
      "}\n";
}

/* vim: set sts=4 sw=4 et: */
