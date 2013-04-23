float3 light_position;
float4 diffuse_color;
float alpha;

/* Ported from ex_prim_shader_pixel.glsl. See that file for the comments. */

#define AMBIENT_COLOR float4(1, 1, 1, 1)
#define AMBIENT_INTENSITY 0.01
#define SPECULAR_COLOR float4(1, 1, 1, 1)

float4 ps_main(VS_OUTPUT Input) : COLOR0
{
   float3 light_vector = light_position - Input.PixelPosition;
   float3 light_dir = normalize(light_vector);
   float3 normal_vector = normalize(Input.Normal);

   float reflectance = dot(light_dir, normal_vector);
   reflectance = max(reflectance, 0.0f);

   float specular_intensity = dot((2 * reflectance * normal_vector - light_dir), float3(0, 0, 1));
   specular_intensity = max(0, specular_intensity);
   specular_intensity = pow(specular_intensity, alpha);

   float diffuse_intensity = reflectance * 10000.0 / dot(light_vector, light_vector);

   return AMBIENT_COLOR * AMBIENT_INTENSITY + diffuse_color * diffuse_intensity + SPECULAR_COLOR * specular_intensity;
}
