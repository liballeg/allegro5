#ifdef GL_ES
precision mediump float;
#endif

varying vec2 texcoord;

void main()
{
   gl_FragColor = vec4(mod(texcoord / 32.0, 1.0), 0, 1);
}
