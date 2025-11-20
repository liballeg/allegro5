attribute vec4 al_pos;
attribute vec4 al_color;
attribute vec2 al_texcoord;
attribute vec3 al_tex_matrix;
uniform mat4 al_projview_matrix;
varying vec4 varying_color;
varying vec2 varying_texcoord;
void main()
{
   varying_color = al_color;
   varying_texcoord = al_texcoord;
   gl_Position = al_projview_matrix * al_pos;
}
