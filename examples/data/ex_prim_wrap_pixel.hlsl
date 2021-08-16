texture al_tex;
sampler2D s1 : register(s0) = sampler_state {
   texture = <al_tex>;
};
texture tex2;
sampler2D s2 : register(s1) = sampler_state {
   texture = <tex2>;
};

float4 ps_main(VS_OUTPUT Input) : COLOR0
{
   return tex2D(s1, Input.TexCoord) * tex2D(s2, Input.TexCoord);
}
