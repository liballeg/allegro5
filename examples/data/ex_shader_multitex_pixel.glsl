/* Simple fragment shader which uses the fractional texture coordinate to
 * look up the color of the second texture (scaled down by factor 100).
 */
#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D al_tex;
uniform sampler2D tex2;
varying vec2 varying_texcoord;
void main()
{
    vec4 color = texture2D(tex2, fract(varying_texcoord * 100.0));
    gl_FragColor = color * texture2D(al_tex, varying_texcoord);
}
