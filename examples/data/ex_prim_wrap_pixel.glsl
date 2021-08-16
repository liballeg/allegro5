#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D al_tex;
uniform sampler2D tex2;
varying vec2 varying_texcoord;
void main()
{
    gl_FragColor = texture2D(al_tex, varying_texcoord) * texture2D(tex2, varying_texcoord);
}
