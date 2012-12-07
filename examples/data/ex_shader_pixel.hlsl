texture tex;
sampler2D s = sampler_state {
   texture = <tex>;
};
float3 tint;
float4 ps_main(VS_OUTPUT Input) : COLOR0
{
   float4 pixel = tex2D(s, Input.TexCoord.xy);
   pixel.r *= tint.r;
   pixel.g *= tint.g;
   pixel.b *= tint.b;
   return pixel;
}
