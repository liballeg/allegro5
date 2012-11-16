
// FIXME: Ignores use_tex_matrix and tex_matrix.

/* Function: al_get_default_glsl_vertex_shader
 */
char const *al_get_default_glsl_vertex_shader(void)
{
   return
      "attribute vec4 pos;\n"
      "attribute vec4 color;\n"
      "attribute vec2 texcoord;\n"
      "uniform mat4 projview_matrix;\n"
      "varying vec4 varying_color;\n"
      "varying vec2 varying_texcoord;\n"
      "void main()\n"
      "{\n"
      "  varying_color = color;\n"
      "  varying_texcoord = texcoord;\n"
      "  gl_Position = projview_matrix * pos;\n"
      "}\n";
}

/* Function: al_get_default_glsl_pixel_shader
 */
char const *al_get_default_glsl_pixel_shader(void)
{
   return
      "uniform sampler2D tex;\n"
      "uniform int use_tex;\n"
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
