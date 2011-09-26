#include "Framebuffer.h"


#include "OpenCL.h"
#include "Config.h"


namespace Reyes
{

    Framebuffer::Framebuffer(CL::Device& device,
                             const ivec2& size, int tile_size) :
        _size(size),
        _tile_size(tile_size),
        _grid_size(ceil((float)size.x/tile_size), ceil((float)size.y/tile_size)),
        _act_size(_grid_size * tile_size), 
        _shader("tex_draw"),
        _clear_kernel(device, "framebuffer.cl", "clear"),
        _cl_buffer(0)
    {
        
    }

    Framebuffer::~Framebuffer()
    {
        if (_cl_buffer) {
            delete _cl_buffer;
            _cl_buffer = NULL;
        }
    }
    

    CL::Event Framebuffer::clear(CL::CommandQueue& queue, const CL::Event& e)
    {
        _clear_kernel.set_arg(0, _cl_buffer->get());
        vec4 color = config.clear_color();
        _clear_kernel.set_arg(1, vec4(powf(color.x, 2.2), 
                                      powf(color.y, 2.2),
                                      powf(color.z, 2.2), 1000));
        return queue.enq_kernel(_clear_kernel, _size.x * _size.y, 256,
                                "clear framebuffer", e);
    }


    OGLSharedFramebuffer::OGLSharedFramebuffer(CL::Device& device,
                                               const ivec2& size, int tile_size) :
        Framebuffer(device, size, tile_size),
        _tex_buffer(_act_size.x * _act_size.y * sizeof(vec4), GL_RGBA32F),
        _shared(device.share_gl()),
        _local(0)
    {
        if (_shared) {
            _cl_buffer = new CL::Buffer(device, _tex_buffer.get_buffer());
        } else {
            _cl_buffer = new CL::Buffer(device, _tex_buffer.get_size(), CL_MEM_READ_WRITE);
            _local = malloc(_tex_buffer.get_size());
        }
    }

    OGLSharedFramebuffer::~OGLSharedFramebuffer()
    {
        if (_local) {
            free(_local);
        }
    }

    CL::Event OGLSharedFramebuffer::acquire(CL::CommandQueue& queue, const CL::Event& e)
    {
        if (_shared) {
            return queue.enq_GL_acquire(_cl_buffer->get(),
                                        "acquire framebuffer", e);
        } else {
            return e;
        }
    }

    CL::Event OGLSharedFramebuffer::release(CL::CommandQueue& queue, const CL::Event& evt)
    {
        if (_shared) {
            CL::Event e = queue.enq_GL_release(_cl_buffer->get(),
                                               "release framebuffer", evt);
            return e;
        } else {

            assert(_local);
            CL::Event e = queue.enq_read_buffer(*_cl_buffer, _local, _tex_buffer.get_size(),
                                                "read framebuffer", evt);
            queue.wait_for_events(e);

            _tex_buffer.load(_local);
            return CL::Event();
        }
    }

    void OGLSharedFramebuffer::show()
    {
        glEnable(GL_TEXTURE_2D);

        _tex_buffer.bind();
        
        _shader.bind();

        _shader.set_uniform("framebuffer", _tex_buffer);
        _shader.set_uniform("bsize", _tile_size);
        _shader.set_uniform("gridsize", _grid_size);

        glBegin(GL_QUADS);
        glVertex2f(-1,-1);
        glVertex2f( 1,-1);
        glVertex2f( 1, 1);
        glVertex2f(-1, 1);
        glEnd();

        _shader.unbind();
        
        _tex_buffer.unbind();

        glfwSwapBuffers();        
    }
}
