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


#ifdef A5O_CFG_SHADER_GLSL

static const char *default_glsl_vertex_source =
   "attribute vec4 " A5O_SHADER_VAR_POS ";\n"
   "attribute vec4 " A5O_SHADER_VAR_COLOR ";\n"
   "attribute vec2 " A5O_SHADER_VAR_TEXCOORD ";\n"
   "uniform mat4 " A5O_SHADER_VAR_PROJVIEW_MATRIX ";\n"
   "uniform bool " A5O_SHADER_VAR_USE_TEX_MATRIX ";\n"
   "uniform mat4 " A5O_SHADER_VAR_TEX_MATRIX ";\n"
   "varying vec4 varying_color;\n"
   "varying vec2 varying_texcoord;\n"
   "void main()\n"
   "{\n"
   "  varying_color = " A5O_SHADER_VAR_COLOR ";\n"
   "  if (" A5O_SHADER_VAR_USE_TEX_MATRIX ") {\n"
   "    vec4 uv = " A5O_SHADER_VAR_TEX_MATRIX " * vec4(" A5O_SHADER_VAR_TEXCOORD ", 0, 1);\n"
   "    varying_texcoord = vec2(uv.x, uv.y);\n"
   "  }\n"
   "  else\n"
   "    varying_texcoord = " A5O_SHADER_VAR_TEXCOORD";\n"
   "  gl_Position = " A5O_SHADER_VAR_PROJVIEW_MATRIX " * " A5O_SHADER_VAR_POS ";\n"
   "}\n";

static const char *default_glsl_pixel_source =
   "#ifdef GL_ES\n"
   "precision lowp float;\n"
   "#endif\n"
   "uniform sampler2D " A5O_SHADER_VAR_TEX ";\n"
   "uniform bool " A5O_SHADER_VAR_USE_TEX ";\n"
   "uniform bool " A5O_SHADER_VAR_ALPHA_TEST ";\n"
   "uniform int " A5O_SHADER_VAR_ALPHA_FUNCTION ";\n"
   "uniform float " A5O_SHADER_VAR_ALPHA_TEST_VALUE ";\n"
   "varying vec4 varying_color;\n"
   "varying vec2 varying_texcoord;\n"
   "\n"
   "bool alpha_test_func(float x, int op, float compare);\n"
   "\n"
   "void main()\n"
   "{\n"
   "  vec4 c;\n"
   "  if (" A5O_SHADER_VAR_USE_TEX ")\n"
   "    c = varying_color * texture2D(" A5O_SHADER_VAR_TEX ", varying_texcoord);\n"
   "  else\n"
   "    c = varying_color;\n"
   "  if (!" A5O_SHADER_VAR_ALPHA_TEST " || alpha_test_func(c.a, " A5O_SHADER_VAR_ALPHA_FUNCTION ", "
                          A5O_SHADER_VAR_ALPHA_TEST_VALUE "))\n"
   "    gl_FragColor = c;\n"
   "  else\n"
   "    discard;\n"
   "}\n"
   "\n"
   "bool alpha_test_func(float x, int op, float compare)\n"
   "{\n"
   // Note: These must be aligned with the A5O_RENDER_FUNCTION enum values.
   "  if (op == 0) return false;\n" // A5O_RENDER_NEVER
   "  else if (op == 1) return true;\n" // A5O_RENDER_ALWAYS
   "  else if (op == 2) return x < compare;\n" // A5O_RENDER_LESS
   "  else if (op == 3) return x == compare;\n" // A5O_RENDER_EQUAL
   "  else if (op == 4) return x <= compare;\n" // A5O_RENDER_LESS_EQUAL
   "  else if (op == 5) return x > compare;\n" // A5O_RENDER_GREATER
   "  else if (op == 6) return x != compare;\n" // A5O_RENDER_NOT_EQUAL
   "  else if (op == 7) return x >= compare;\n" // A5O_RENDER_GREATER_EQUAL
   "  return false;\n"
   "}\n";

static const char *default_glsl_minimal_pixel_source =
   "#ifdef GL_ES\n"
   "precision lowp float;\n"
   "#endif\n"
   "uniform sampler2D " A5O_SHADER_VAR_TEX ";\n"
   "uniform bool " A5O_SHADER_VAR_USE_TEX ";\n"
   "varying vec4 varying_color;\n"
   "varying vec2 varying_texcoord;\n"
   "\n"
   "void main()\n"
   "{\n"
   "  vec4 c;\n"
   "  if (" A5O_SHADER_VAR_USE_TEX ")\n"
   "    c = varying_color * texture2D(" A5O_SHADER_VAR_TEX ", varying_texcoord);\n"
   "  else\n"
   "    c = varying_color;\n"
   "  gl_FragColor = c;\n"
   "}\n";

#endif /* A5O_CFG_SHADER_GLSL */


#ifdef A5O_CFG_SHADER_HLSL

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
   "float4x4 " A5O_SHADER_VAR_PROJVIEW_MATRIX ";\n"
   "bool " A5O_SHADER_VAR_USE_TEX_MATRIX ";\n"
   "float4x4 " A5O_SHADER_VAR_TEX_MATRIX ";\n"
   "\n"
   "VS_OUTPUT vs_main(VS_INPUT Input)\n"
   "{\n"
   "   VS_OUTPUT Output;\n"
   "   Output.Color = Input.Color;\n"
   "   if (" A5O_SHADER_VAR_USE_TEX_MATRIX ") {\n"
   "      Output.TexCoord = mul(float4(Input.TexCoord, 1.0f, 0.0f), "
            A5O_SHADER_VAR_TEX_MATRIX ").xy;\n"
   "   }\n"
   "   else {\n"
   "      Output.TexCoord = Input.TexCoord;\n"
   "   }\n"
   "   Output.Position = mul(Input.Position, " A5O_SHADER_VAR_PROJVIEW_MATRIX ");\n"
   "   return Output;\n"
   "}\n";

static const char *default_hlsl_pixel_source =
   "bool " A5O_SHADER_VAR_USE_TEX ";\n"
   "texture " A5O_SHADER_VAR_TEX ";\n"
   "sampler2D s = sampler_state {\n"
   "   texture = <" A5O_SHADER_VAR_TEX ">;\n"
   "};\n"
   "\n"
   "float4 ps_main(VS_OUTPUT Input) : COLOR0\n"
   "{\n"
   "   if (" A5O_SHADER_VAR_USE_TEX ") {\n"
   "      return Input.Color * tex2D(s, Input.TexCoord);\n"
   "   }\n"
   "   else {\n"
   "      return Input.Color;\n"
   "   }\n"
   "}\n";

#endif /* A5O_CFG_SHADER_HLSL */


/* vim: set ft=c sts=3 sw=3 et: */
