#include "Mandelbrot.h"

Mandelbrot::Mandelbrot()
    : shader("mandelbrot")
    , quad(4)
{
    quad.vertex(-1,-1);
    quad.vertex( 1,-1);
    quad.vertex(-1, 1);
    quad.vertex( 1, 1);
    quad.send_data(false);

    shader.bind();
    shader.set_uniform("max_iterations", (GLint)1024);
    shader.unbind();


    const int W = 256*4;
    GLfloat texdata[W];
    for (int i=0; i < W; ++i) {
        double x = i/double(W) * M_PI * 2.0;
        texdata[i] = float(sin(x) * 0.5 + 0.5);
    }

    texture = new GL::Texture(1,W,0,0,GL_RED,GL_R32F,GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR,GL_REPEAT,0,texdata);
}


Mandelbrot::~Mandelbrot()
{
    delete texture;
}


void Mandelbrot::draw(const dvec2& focus, double mag)
{
    dvec2 size_h = dvec2(config.window_size().x, config.window_size().y) * mag;

    dmat3 view(size_h.x,0,0,
               0,size_h.y,0,
               focus.x,focus.y,1);

    texture->bind();
    shader.bind();

    shader.set_uniform("view", view);
    shader.set_uniform("tex", (const GL::Tex*)texture);
    
    quad.draw(GL_TRIANGLE_STRIP, shader);
        
    shader.unbind();
    texture->unbind();
}
