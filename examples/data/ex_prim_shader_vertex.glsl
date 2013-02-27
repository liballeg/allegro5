#version 120
attribute vec4 al_pos;
attribute vec4 al_color;
attribute vec3 al_user_attr_0;
uniform vec2 mouse_pos;
uniform mat4 al_projview_matrix;
varying vec4 varying_color;
void main()
{
   float weight = 1.0f - min(length(mouse_pos - al_pos.xy) / 300.0f, 1.0f);
   vec4 real_pos = vec4(mouse_pos * weight + al_pos.xy * (1.0f - weight), al_pos.zw);
   varying_color = vec4(al_user_attr_0 * weight + al_color.xyz * (1.0f - weight), al_color.w);
   gl_Position = al_projview_matrix * real_pos;
}
