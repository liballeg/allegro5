#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D al_tex;
uniform vec3 tint;
varying vec4 varying_color;
varying vec2 varying_texcoord;
void main()
{
   vec4 tmp = varying_color * texture2D(al_tex, varying_texcoord);
   tmp.r *= tint.r;
   tmp.g *= tint.g;
   tmp.b *= tint.b;
   gl_FragColor = tmp;
}
