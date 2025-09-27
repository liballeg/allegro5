attribute vec4 al_pos;
attribute vec2 al_texcoord;

uniform mat4 al_projview_matrix;

varying vec2 texcoord;

void main()
{
   texcoord = al_texcoord;
   gl_Position = al_projview_matrix * al_pos;
}
