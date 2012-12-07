attribute vec4 pos;
attribute vec4 color;
attribute vec2 texcoord;
uniform mat4 projview_matrix;
varying vec4 varying_color;
varying vec2 varying_texcoord;
void main()
{
   varying_color = color;
   varying_texcoord = texcoord;
   gl_Position = projview_matrix * pos;
}
