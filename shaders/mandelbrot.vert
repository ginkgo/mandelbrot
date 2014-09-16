#version 430

precision highp float;
precision highp int;


in vec2 vertex;
out vec2 coord;

void main (void)
{
    coord = vertex;
    
    gl_Position = vec4(vertex,0,1);
}
