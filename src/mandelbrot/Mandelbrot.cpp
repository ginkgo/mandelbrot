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
}


Mandelbrot::~Mandelbrot()
{

}


void Mandelbrot::draw(const dvec2& focus, double mag)
{
    dvec2 size_h = dvec2(config.window_size().x, config.window_size().y) * mag;

    dmat3 view(size_h.x,0,0,
               0,size_h.y,0,
               focus.x,focus.y,1);
    
    shader.bind();

    shader.set_uniform("view", view);
    
    quad.draw(GL_TRIANGLE_STRIP, shader);
        
    shader.unbind();
}
