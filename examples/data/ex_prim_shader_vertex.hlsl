struct VS_INPUT
{
   float4 Position  : POSITION0;
   float4 Color     : TEXCOORD1;
   float3 NewColor  : TEXCOORD2;
};
struct VS_OUTPUT
{
   float4 Position  : POSITION0;
   float4 Color     : COLOR0;
};

float2 mouse_pos;
float4x4 projview_matrix;

VS_OUTPUT vs_main(VS_INPUT Input)
{
   VS_OUTPUT Output;
   
   float weight = 1.0f - min(length(mouse_pos - Input.Position.xy) / 300.0f, 1.0f);
   float4 real_pos = float4(mouse_pos * weight + Input.Position.xy * (1.0f - weight), Input.Position.zw);
   
   Output.Position = mul(real_pos, projview_matrix);
   Output.Color = float4(Input.NewColor * weight + Input.Color.xyz * (1.0f - weight), Input.Color.w);
   return Output;
}
