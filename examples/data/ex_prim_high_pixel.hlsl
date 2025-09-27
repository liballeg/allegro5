// To match GLSL's
float2 mod(float2 x, float2 y) {
   return x - y * floor(x / y);
}

float4 ps_main(VS_OUTPUT Input) : COLOR0
{
   return float4(mod(Input.TexCoord / 32.0, 1.0), 0, 1);
}
