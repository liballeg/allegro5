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
   "#ifdef GL_ES\n"
   "precision lowp float;\n"
   "#endif\n"
   // Note: Those must be aligned with the ALLEGRO_RENDER_FUNCTION enum values
   "#define ALLEGRO_RENDER_NEVER 0\n"
   "#define ALLEGRO_RENDER_ALWAYS 1\n"
   "#define ALLEGRO_RENDER_LESS 2\n"
   "#define ALLEGRO_RENDER_EQUAL 3\n"
   "#define ALLEGRO_RENDER_LESS_EQUAL 4\n"
   "#define ALLEGRO_RENDER_GREATER 5\n"
   "#define ALLEGRO_RENDER_NOT_EQUAL 6\n"
   "#define ALLEGRO_RENDER_GREATER_EQUAL 7\n"
   "uniform sampler2D " ALLEGRO_SHADER_VAR_TEX ";\n"
   "uniform bool " ALLEGRO_SHADER_VAR_USE_TEX ";\n"
   "uniform int " ALLEGRO_SHADER_VAR_ALPHA_FUNCTION ";\n"
   "uniform float " ALLEGRO_SHADER_VAR_ALPHA_TEST_VALUE ";\n"
   "varying vec4 varying_color;\n"
   "varying vec2 varying_texcoord;\n"
   "void main()\n"
   "{\n"
   "  vec4 c;\n"
   "  if (" ALLEGRO_SHADER_VAR_USE_TEX ")\n"
   "    c = varying_color * texture2D(" ALLEGRO_SHADER_VAR_TEX ", varying_texcoord);\n"
   "  else\n"
   "    c = varying_color;\n"
   "  int f = " ALLEGRO_SHADER_VAR_ALPHA_FUNCTION ";\n"
   "  if (f == ALLEGRO_RENDER_ALWAYS) {gl_FragColor = c; return;}\n"
   "  if (f == ALLEGRO_RENDER_NEVER) {discard; return;}\n"
   "  float a = " ALLEGRO_SHADER_VAR_ALPHA_TEST_VALUE ";\n"
   "  if (f == ALLEGRO_RENDER_LESS) {if (c.a < a) gl_FragColor = c; else discard; return;}\n"
   "  if (f == ALLEGRO_RENDER_EQUAL) {if (c.a == a) gl_FragColor = c; else discard; return;}\n"
   "  if (f == ALLEGRO_RENDER_LESS_EQUAL) {if (c.a <= a) gl_FragColor = c; else discard; return;}\n"
   "  if (f == ALLEGRO_RENDER_GREATER) {if (c.a > a) gl_FragColor = c; else discard; return;}\n"
   "  if (f == ALLEGRO_RENDER_NOT_EQUAL) {if (c.a != a) gl_FragColor = c; else discard; return;}\n"
   "  if (f == ALLEGRO_RENDER_GREATER_EQUAL) {if (c.a >= a) gl_FragColor = c; else discard; return;}\n"
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
   "float4x4 " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX ";\n"
   "bool " ALLEGRO_SHADER_VAR_USE_TEX_MATRIX ";\n"
   "float4x4 " ALLEGRO_SHADER_VAR_TEX_MATRIX ";\n"
   "\n"
   "VS_OUTPUT vs_main(VS_INPUT Input)\n"
   "{\n"
   "   VS_OUTPUT Output;\n"
   "   Output.Color = Input.Color;\n"
   "   if (" ALLEGRO_SHADER_VAR_USE_TEX_MATRIX ") {\n"
   "      Output.TexCoord = mul(float4(Input.TexCoord, 1.0f, 0.0f), "
            ALLEGRO_SHADER_VAR_TEX_MATRIX ").xy;\n"
   "   }\n"
   "   else {\n"
   "      Output.TexCoord = Input.TexCoord;\n"
   "   }\n"
   "   Output.Position = mul(Input.Position, " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX ");\n"
   "   return Output;\n"
   "}\n";

static const char *default_hlsl_pixel_source =
   "bool " ALLEGRO_SHADER_VAR_USE_TEX ";\n"
   "texture " ALLEGRO_SHADER_VAR_TEX ";\n"
   "sampler2D s = sampler_state {\n"
   "   texture = <" ALLEGRO_SHADER_VAR_TEX ">;\n"
   "};\n"
   "\n"
   "float4 ps_main(VS_OUTPUT Input) : COLOR0\n"
   "{\n"
   "   if (" ALLEGRO_SHADER_VAR_USE_TEX ") {\n"
   "      return Input.Color * tex2D(s, Input.TexCoord);\n"
   "   }\n"
   "   else {\n"
   "      return Input.Color;\n"
   "   }\n"
   "}\n";

#endif /* ALLEGRO_CFG_SHADER_HLSL */


/* vim: set ft=c sts=3 sw=3 et: */
