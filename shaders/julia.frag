#version 430

precision highp float;
precision highp int;

in vec2 coord;
out vec4 frag_color;

uniform dmat3 view;
uniform int max_iterations;
uniform sampler1D tex;
uniform dvec2 c;

dvec2 csquare(dvec2 z)
{
    const double x = z.x;
    const double y = z.y;
    return dvec2(x*x-y*y, 2.0*x*y);
}

void main (void)
{
    // frag_color = vec4(0,0,1,0);
    // return;
    
    dvec2 val = (view * dvec3(coord.x,coord.y,1)).xy;
    
    int it = 0;
    dvec2 z = val;
    
    while (it < max_iterations && dot(z,z) < 4.0) {
        it++;
        z = csquare(z) + c;
    }
    
    frag_color = vec4(texture(tex,it/23.0).r, texture(tex,it/29.0).r, texture(tex,it/31.0).r, 1);
                      

    float x = it==max_iterations ? 0 : 1;
    frag_color = frag_color * x;
}
