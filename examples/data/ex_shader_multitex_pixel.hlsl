texture al_tex;
sampler2D s1 = sampler_state {
   texture = <al_tex>;
};
texture tex2;
sampler2D s2 = sampler_state {
   texture = <tex2>;
};

float4 ps_main(VS_OUTPUT Input) : COLOR0
{
   float4 color = tex2D(s2, frac(Input.TexCoord * 100));
   return color * tex2D(s1, Input.TexCoord);
}
