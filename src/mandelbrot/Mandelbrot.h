#pragma once

#include "common.h"

#include "Config.h"

#include "GL/VBO.h"
#include "GL/Shader.h"


class Mandelbrot
{
    
    GL::Shader shader;
    GL::VBO quad;

    public:

    Mandelbrot();
    ~Mandelbrot();
    
    void draw(const dvec2& focus, double mag);
};
