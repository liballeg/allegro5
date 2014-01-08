#ifdef GL_ES
precision mediump float;
#endif

/* This shader implements Phong shading (see http://en.wikipedia.org/wiki/Phong_reflection_model for the math).
 * It is not the most optimized version of it, as it sticks closely to the mathematics of the model. */

uniform vec3 light_position;
/* diffuse color specifies the color of the illumination outside of the highlights, but still within the light
 * radius */
uniform vec4 diffuse_color;
/* alpha is the shininess of a surface. 1 is dull, >1 is shiny. <1 is weird :) */
uniform float alpha;

varying vec3 pixel_position;
varying vec3 normal;

/* Ambient color and intensity specify the color of the illumination outside the range of the light */
#define AMBIENT_COLOR vec4(1, 1, 1, 1)
#define AMBIENT_INTENSITY 0.01
/* Specifies the color of the highlights */
#define SPECULAR_COLOR vec4(1, 1, 1, 1)

void main()
{
   /* Vector pointing from the point on the surface to the light */
   vec3 light_vector = light_position - pixel_position;
   /* Unit vector of the above */
   vec3 light_dir = normalize(light_vector);
   /* Unit vector normal to the surface (linear interpolation between vertex normals does not produce unit vectors in most cases)*/
   vec3 normal_vector = normalize(normal);

   /* Reflectance is used for both diffuse components and specular components. It needs to be non-negative */
   float reflectance = dot(light_dir, normal_vector);
   reflectance = max(reflectance, 0.0);

   /* Computes the reflection of the light and then the degree to which it points at the camera. This also is non-negative.
    * (0, 0, 1) is the vector that points at the camera */
   float specular_intensity = dot((2.0 * reflectance * normal_vector - light_dir), vec3(0.0, 0.0, 1.0));
   specular_intensity = max(0.0, specular_intensity);
   specular_intensity = pow(specular_intensity, alpha);

   /* Compute diffuse intensity by attenuating reflectance with a simple quadratic distance falloff */
   float diffuse_intensity = reflectance * 10000.0 / dot(light_vector, light_vector);

   /* The final color is the combination of the 3 light sources */
   gl_FragColor = AMBIENT_COLOR * AMBIENT_INTENSITY + diffuse_color * diffuse_intensity + SPECULAR_COLOR * specular_intensity;
}
