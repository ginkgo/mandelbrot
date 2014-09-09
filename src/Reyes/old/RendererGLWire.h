/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


#pragma once

#include "BoundNSplitGL.h"
#include "GL/Shader.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "PatchIndex.h"
#include "Renderer.h"

namespace Reyes
{
        
    class RendererGLWire : public Renderer
    {
		GL::Shader _shader;
		GL::IndirectVBO _vbo;

        shared_ptr<PatchIndex> _patch_index;
        shared_ptr<BoundNSplitGL> _bound_n_split;

    public:

        RendererGLWire();
        ~RendererGLWire() {};

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

