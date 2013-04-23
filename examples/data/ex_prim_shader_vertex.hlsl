struct VS_INPUT
{
   float4 Position  : POSITION0;
   float3 Normal    : TEXCOORD2;
};
struct VS_OUTPUT
{
   float4 Position       : POSITION0;
   /* pixel_position and normal are used to compute the reflections in the pixel shader */
   float3 PixelPosition  : TEXCOORD0;
   float3 Normal         : TEXCOORD1;
};

float4x4 al_projview_matrix;

VS_OUTPUT vs_main(VS_INPUT Input)
{
   VS_OUTPUT Output;

   Output.Position = mul(Input.Position, al_projview_matrix);
   Output.PixelPosition = Input.Position.xyz;
   Output.Normal = Input.Normal;
   return Output;
}
