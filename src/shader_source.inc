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
 *      Default shader sources.
 *
 *      See LICENSE.txt for copyright information.
 */


#ifdef ALLEGRO_CFG_SHADER_GLSL

static const char *default_glsl_vertex_source =
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

static const char *default_glsl_pixel_source =
   #ifndef ALLEGRO_CFG_OPENGLES
   "#version 120\n" /* Must be first so we can't use #ifndef GL_ES. */
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

#endif /* ALLEGRO_CFG_SHADER_GLSL */


#ifdef ALLEGRO_CFG_SHADER_HLSL

static const char *default_hlsl_vertex_source =
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

static const char *default_hlsl_pixel_source =
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

#endif /* ALLEGRO_CFG_SHADER_HLSL */


/* vim: set ft=c sts=3 sw=3 et: */
