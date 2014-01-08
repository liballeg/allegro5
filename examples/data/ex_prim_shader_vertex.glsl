attribute vec4 al_pos;
attribute vec3 al_user_attr_0;

uniform mat4 al_projview_matrix;

/* pixel_position and normal are used to compute the reflections in the pixel shader */
varying vec3 pixel_position;
varying vec3 normal;

void main()
{
   pixel_position = al_pos.xyz;
   normal = al_user_attr_0;
   gl_Position = al_projview_matrix * al_pos;
}
