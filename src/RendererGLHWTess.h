
#pragma once


#include "BoundNSplitGL.h"
#include "GL/Shader.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "Renderer.h"
#include "PatchIndex.h"


namespace Reyes
{
        
    class RendererGLHWTess : public Renderer
    {
		GL::Shader _shader;
		GL::IndirectVBO _vbo;
        
        shared_ptr<PatchIndex> _patch_index;
        shared_ptr<BoundNSplitGL> _bound_n_split;

    public:

        RendererGLHWTess();
        ~RendererGLHWTess() {};

        virtual void prepare();
        virtual void finish();
        
        virtual bool are_patches_loaded(void* patches_handle);
        virtual void load_patches(void* patches_handle, const vector<BezierPatch>& patch_data);
        
        virtual void draw_patches(void* patches_handle,
                                  const mat4& matrix,
                                  const Projection* projection,
                                  const vec4& color);
    };
    
}

