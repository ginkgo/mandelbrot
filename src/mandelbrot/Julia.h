#pragma once

#include "common.h"

#include "Config.h"

#include "GL/Shader.h"
#include "GL/Texture.h"
#include "GL/VBO.h"


class Julia
{

    int size;
    GL::Shader shader;
    GL::VBO quad;
    GL::Texture* texture;
    
    public:

    Julia(int size);
    ~Julia();
    
    void draw(const dvec2& c);
};
