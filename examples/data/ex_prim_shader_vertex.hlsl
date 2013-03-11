struct VS_INPUT
{
   float4 Position  : POSITION0;
   float3 Normal    : TEXCOORD2;
};
struct VS_OUTPUT
{
   float4 Position       : POSITION0;
   float2 PixelPosition  : TEXCOORD0;
   float3 Normal         : TEXCOORD1;
};

float4x4 al_projview_matrix;

VS_OUTPUT vs_main(VS_INPUT Input)
{
   VS_OUTPUT Output;

   Output.Position = mul(Input.Position, al_projview_matrix);
   Output.PixelPosition = Input.Position.xy;
   Output.Normal = Input.Normal;
   return Output;
}
