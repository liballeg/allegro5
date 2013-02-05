#version 120
attribute vec4 pos;
attribute vec4 color;
attribute vec3 user_attr_0;
uniform vec2 mouse_pos;
uniform mat4 projview_matrix;
varying vec4 varying_color;
void main()
{
   float weight = 1.0f - min(length(mouse_pos - pos.xy) / 300.0f, 1.0f);
   vec4 real_pos = vec4(mouse_pos * weight + pos.xy * (1.0f - weight), pos.zw);
   varying_color = vec4(user_attr_0 * weight + color.xyz * (1.0f - weight), color.w);
   gl_Position = projview_matrix * real_pos;
}
