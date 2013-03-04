#version 120
#ifdef GL_ES
precision mediump float;
#endif
varying vec4 varying_color;
void main()
{
   gl_FragColor = varying_color;
}
