#pragma once

#include "common.h"

#include "Config.h"

#include "GL/Shader.h"
#include "GL/Texture.h"
#include "GL/VBO.h"


class Mandelbrot
{
    
    GL::Shader shader;
    GL::VBO quad;
    GL::Texture* texture;
    
    public:

    Mandelbrot();
    ~Mandelbrot();
    
    void draw(const dvec2& focus, double mag);
};
