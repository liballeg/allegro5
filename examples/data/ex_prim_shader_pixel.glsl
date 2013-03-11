#version 120
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 mouse_pos;
uniform vec4 color;
uniform float alpha;

varying vec2 pixel_position;
varying vec3 normal;

void main()
{
   vec3 light_vector = vec3(mouse_pos - pixel_position, 100.0f);
   vec3 light_dir = normalize(light_vector);
   vec3 normal_vector = normalize(normal);

   float attenuation = 0.2 + 10000.0 / dot(light_vector, light_vector);
   float diffuse = dot(light_dir, normal_vector);

   diffuse = max(diffuse, 0.0f);

   float specular = dot((2 * diffuse * normal_vector - light_dir), vec3(0, 0, 1));
   specular = max(0, specular);
   specular = pow(specular, alpha);

   diffuse *= attenuation;

   gl_FragColor = color * diffuse + specular;
}
