uniform sampler2D tex;
uniform vec3 tint;
varying vec4 varying_color;
varying vec2 varying_texcoord;
void main()
{
   vec4 tmp = varying_color * texture2D(tex, varying_texcoord);
   tmp.r *= tint.r;
   tmp.g *= tint.g;
   tmp.b *= tint.b;
   gl_FragColor = tmp;
}
