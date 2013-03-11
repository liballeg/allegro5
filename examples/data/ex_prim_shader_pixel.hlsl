float2 mouse_pos;
float4 color;
float alpha;

float4 ps_main(VS_OUTPUT Input) : COLOR0
{
   float3 light_vector = float3(mouse_pos - Input.PixelPosition, 100.0f);
   float3 light_dir = normalize(light_vector);
   float3 normal_vector = normalize(Input.Normal);

   float attenuation = 0.2 + 10000.0 / dot(light_vector, light_vector);
   float diffuse = dot(light_dir, normal_vector);

   diffuse = max(diffuse, 0.0f);

   float specular = dot((2 * diffuse * normal_vector - light_dir), float3(0, 0, 1));
   specular = max(0, specular);
   specular = pow(specular, alpha);

   diffuse *= attenuation;

   return color * diffuse + specular;
}
