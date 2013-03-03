/* Function: al_get_default_hlsl_vertex_shader
 */
char const *al_get_default_hlsl_vertex_shader(void)
{
   return
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

/* Function: al_get_default_hlsl_pixel_shader
 */
char const *al_get_default_hlsl_pixel_shader(void)
{
   return
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
