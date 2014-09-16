#version 430

precision highp float;
precision highp int;

in vec2 coord;
out vec4 frag_color;

uniform dmat3 view;
uniform int max_iterations;

dvec2 csquare(dvec2 z)
{
    const double x = z.x;
    const double y = z.y;
    return dvec2(x*x-y*y, 2.0*x*y);
}

void main (void)
{
    dvec2 val = (view * dvec3(coord.x,coord.y,1)).xy;
    
    int it = 0;
    dvec2 z = vec2(0,0);
    
    while (it < max_iterations && dot(z,z) < 4.0) {
        it++;
        z = csquare(z) + val;
    }
    
    frag_color = vec4((it%16)/15.0,(it%32)/31.0,(it%64)/63.0,1);

    // float x = pow(1.0-float(it)/max_iterations,8.0);
    // frag_color = vec4(x,x,x,1);
}
