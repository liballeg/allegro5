// This is mostly identical to the default vertex shader, extracted here
// to be used with the fxc.exe tool to generate
// src/win/prim_directx_shader.inc (see that file for the command)
struct VS_INPUT
{
   float4 Position  : POSITION0;
   float2 TexCoord  : TEXCOORD0;
   float4 Color     : TEXCOORD1;
};
struct VS_OUTPUT
{
   float4 Position  : POSITION0;
   float4 Color     : COLOR0;
   float2 TexCoord  : TEXCOORD0;
};

float4x4 al_projview_matrix: register(c0);
float4x4 al_tex_matrix : register(c4);
bool al_use_tex_matrix : register(c8);

VS_OUTPUT vs_main(VS_INPUT Input)
{
   VS_OUTPUT Output;
   Output.Color = Input.Color;
   if (al_use_tex_matrix) {
      Output.TexCoord = mul(float4(Input.TexCoord, 1.0f, 0.0f), al_tex_matrix).xy;
   }
   else {
      Output.TexCoord = Input.TexCoord;
   }
   Output.Position = mul(Input.Position, al_projview_matrix);
   return Output;
}

technique TECH
{
   pass p1
   {
      VertexShader = compile vs_2_0 vs_main();
      PixelShader = null;
   }
}
