#version 120
attribute vec4 al_pos;
attribute vec3 al_user_attr_0;

uniform mat4 al_projview_matrix;

varying vec2 pixel_position;
varying vec3 normal;

void main()
{
   pixel_position = al_pos.xy;
   normal = al_user_attr_0;
   gl_Position = al_projview_matrix * al_pos;
}
