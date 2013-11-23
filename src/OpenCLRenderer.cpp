#include "common.h"

#include "OpenCLRenderer.h"

#include "Config.h"
#include "Framebuffer.h"
#include "OpenCL.h"
#include "Projection.h"
#include "Statistics.h"

Reyes::OpenCLRenderer::OpenCLRenderer()
    : _device(config.platform_id(), config.device_id())
    , _queue(_device)
    , _framebuffer(_device, config.window_size(), config.framebuffer_tile_size(), glfwGetCurrentContext())

    , _patch_index(new PatchesIndex())
      
    , _active_patch_buffer(0)
    , _patch_buffers(config.patch_buffer_count())
    , _back_buffer(0)
    , _patch_count(0)
    , _max_block_count(square(config.reyes_patch_size()/8) * config.reyes_patches_per_pass())
    , _pos_grid(_device, 
                config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(vec4),
                CL_MEM_READ_WRITE)
    , _pxlpos_grid(_device, 
                   config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(ivec2),
                   CL_MEM_READ_WRITE)
    , _color_grid(_device, 
                  config.reyes_patches_per_pass() * square(config.reyes_patch_size()) * sizeof(vec4),
                  CL_MEM_READ_WRITE)
    , _depth_grid(_device, 
                  config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(float),
                  CL_MEM_READ_WRITE)
    , _block_index(_device, _max_block_count * sizeof(ivec4), CL_MEM_READ_WRITE)
    , _tile_locks(_device,
                  _framebuffer.size().x/8 * _framebuffer.size().y/8 * sizeof(cl_int), CL_MEM_READ_WRITE)
	, _depth_buffer(_device, _framebuffer.size().x * _framebuffer.size().y * sizeof(cl_int), CL_MEM_READ_WRITE)
    , _reyes_program()
{
    //_patch_index->enable_load_opencl_buffer();
    _patch_index->enable_retain_vector();
    
    _reyes_program.set_constant("TILE_SIZE", _framebuffer.get_tile_size());
    _reyes_program.set_constant("GRID_SIZE", _framebuffer.get_grid_size());
    _reyes_program.set_constant("PATCH_SIZE", (int)config.reyes_patch_size());
    _reyes_program.set_constant("VIEWPORT_MIN_PIXEL", ivec2(0,0));
    _reyes_program.set_constant("VIEWPORT_MAX_PIXEL", _framebuffer.size());
    _reyes_program.set_constant("VIEWPORT_SIZE_PIXEL", _framebuffer.size());
    _reyes_program.set_constant("MAX_BLOCK_COUNT", _max_block_count);
    _reyes_program.set_constant("MAX_BLOCK_ASSIGNMENTS", config.max_block_assignments());
    _reyes_program.set_constant("FRAMEBUFFER_SIZE", _framebuffer.size());
    _reyes_program.set_constant("BACKFACE_CULLING", config.backface_culling());
    _reyes_program.set_constant("CLEAR_COLOR", config.clear_color());
    _reyes_program.set_constant("CLEAR_DEPTH", 1.0f);
    _reyes_program.set_constant("PXLCOORD_SHIFT", config.subpixel_bits());
                
    _reyes_program.compile(_device, "reyes.cl");

    _dice_kernel.reset(_reyes_program.get_kernel("dice"));
    _shade_kernel.reset(_reyes_program.get_kernel("shade"));
    _sample_kernel.reset(_reyes_program.get_kernel("sample"));
    _init_tile_locks_kernel.reset(_reyes_program.get_kernel("init_tile_locks"));
    _clear_depth_buffer_kernel.reset(_reyes_program.get_kernel("clear_depth_buffer"));

    _dice_kernel->set_arg_r(1, _pos_grid);
    _dice_kernel->set_arg_r(2, _pxlpos_grid);
    _dice_kernel->set_arg_r(4, _depth_grid);

    _shade_kernel->set_arg_r(0, _pos_grid);
    _shade_kernel->set_arg_r(1, _pxlpos_grid);
    _shade_kernel->set_arg_r(2, _block_index);
    _shade_kernel->set_arg_r(3, _color_grid);

    _sample_kernel->set_arg_r(0, _block_index);
    _sample_kernel->set_arg_r(1, _pxlpos_grid);
    _sample_kernel->set_arg_r(2, _color_grid);
    _sample_kernel->set_arg_r(3, _depth_grid);
    _sample_kernel->set_arg_r(4, _tile_locks);
    _sample_kernel->set_arg_r(5, _framebuffer.get_buffer());
    _sample_kernel->set_arg_r(6, _depth_buffer);

    _clear_depth_buffer_kernel->set_arg_r(0, _depth_buffer);

    _patch_buffers.resize(config.patch_buffer_count());

    for (int i = 0; i < config.patch_buffer_count(); ++i) {
        PatchBuffer& buffer = _patch_buffers.at(i);

        if (config.patch_buffer_mode() == Config::PINNED) {
            buffer.host = NULL;
            buffer.buffer = new CL::Buffer(_device, _queue, config.reyes_patches_per_pass() * 16 * sizeof(vec4) * 2, 
                                           CL_MEM_READ_ONLY, &(buffer.host));
        } else {
            buffer.host = malloc(config.reyes_patches_per_pass() * 16 * sizeof(vec4) * 2);
            buffer.buffer = new CL::Buffer(_device, config.reyes_patches_per_pass() * 16 * sizeof(vec4) * 2, 
                                           CL_MEM_READ_ONLY, buffer.host);
        }

        assert (buffer.host != NULL);
        buffer.write_complete = CL::Event();
    }

    _back_buffer = (vec4*)(_patch_buffers.at(_active_patch_buffer).host);
	    
    _init_tile_locks_kernel->set_arg_r(0, _tile_locks);
    _queue.enq_kernel(*_init_tile_locks_kernel, _framebuffer.size().x/8 * _framebuffer.size().y/8, 64, 
                      "init tile locks", CL::Event());
}


Reyes::OpenCLRenderer::~OpenCLRenderer()
{
    for (int i = 0; i < config.patch_buffer_count(); ++i) {
        delete _patch_buffers.at(i).buffer;

        if (config.patch_buffer_mode() == Config::UNPINNED) {
            free(_patch_buffers.at(i).host);
        }
    }
}



void Reyes::OpenCLRenderer::prepare()
{
    CL::Event e = _framebuffer.acquire(_queue, CL::Event());
    e = _framebuffer.clear(_queue, e);
    _framebuffer_cleared = _queue.enq_kernel(*_clear_depth_buffer_kernel,
                                             _framebuffer.size().x * _framebuffer.size().y, 64,
                                             "clear depthbuffer", e); 
    
    statistics.start_render();
}


void Reyes::OpenCLRenderer::finish()
{
    flush();
    
    _framebuffer.release(_queue, _last_sample);
    _framebuffer.show();
        
    for (int i = 0; i < config.patch_buffer_count(); ++i) {
        _patch_buffers.at(i).write_complete = CL::Event();
    }

    _last_sample = CL::Event();
    _framebuffer_cleared = CL::Event();

    _active_patch_buffer = 0;
    _back_buffer = (vec4*)(_patch_buffers.at(_active_patch_buffer).host);
        
    glfwSwapBuffers(glfwGetCurrentContext());

    statistics.end_render();
}



bool Reyes::OpenCLRenderer::are_patches_loaded(void* patches_handle)
{
    return _patch_index->are_patches_loaded(patches_handle);
}


void Reyes::OpenCLRenderer::load_patches(void* patches_handle, const vector<BezierPatch>& patch_data)
{
    _patch_index->load_patches(patches_handle, patch_data);
}


void Reyes::OpenCLRenderer::draw_patches(void* patches_handle,
                                         const mat4& matrix,
                                         const Projection* projection,
                                         const vec4& color)
{
    set_projection(*projection);
    
    std::vector<BezierPatch> patch_stack = _patch_index->get_patch_vector(patches_handle);

    for (BezierPatch& patch : patch_stack) {
        transform_patch(patch, matrix, patch);
    }   
    
    mat4 proj;
    projection->calc_projection(proj);
        
    int s = config.bound_n_split_limit();
        
    while (patch_stack.size() > 0) {

        BezierPatch patch = patch_stack.back();
        patch_stack.pop_back();

        BBox box;
        calc_bbox(patch, box);

        vec2 size;
        bool cull;
        projection->bound(box, size, cull);
    
        if (cull) continue;

        if (box.min.z < 0 && size.x < s && size.y < s) {
            draw_patch(patch);
        } else {
            BezierPatch p0, p1;
            pisplit_patch(patch, p0, p1, proj);
            patch_stack.push_back(p0);
            patch_stack.push_back(p1);
        }

    }
}



void Reyes::OpenCLRenderer::set_projection(const Projection& projection)
{
    mat4 proj;
    projection.calc_projection(proj);

    _dice_kernel->set_arg(3, proj);
}

void Reyes::OpenCLRenderer::draw_patch (const BezierPatch& patch) 
{
    size_t cp_index = _patch_count * 16;

    for     (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            assert(cp_index < config.reyes_patches_per_pass()  * 16);
            _back_buffer[cp_index] = vec4(patch.P[i][j], 1);
            ++cp_index;
        }
    }

    _patch_count++;
        
    if (_patch_count >= config.reyes_patches_per_pass()) {
        statistics.stop_bound_n_split();

        flush();
        _queue.wait_for_events(_patch_buffers.at(_active_patch_buffer).write_complete);

        statistics.start_bound_n_split();
    }
    
    statistics.inc_patch_count();
}

void Reyes::OpenCLRenderer::flush()
{
    if (_patch_count == 0) {
        return;
    }
 
    PatchBuffer& active  = _patch_buffers.at(_active_patch_buffer);
    _active_patch_buffer = (_active_patch_buffer+1) % config.patch_buffer_count();
    _back_buffer = (vec4*)(_patch_buffers.at(_active_patch_buffer).host);

    CL::Event e, f;
    e = _queue.enq_write_buffer(*(active.buffer), active.host, _patch_count * sizeof(vec4) * 16,
                                "write patches", CL::Event());

    int patch_size  = config.reyes_patch_size();
    int group_width = config.dice_group_width();

    _dice_kernel->set_arg_r(0, *(active.buffer));
    e = _queue.enq_kernel(*_dice_kernel,
                          ivec3(patch_size + group_width, patch_size + group_width, _patch_count),
                          ivec3(group_width, group_width, 1),
                          "dice", e | _last_sample);
    e = _queue.enq_kernel(*_shade_kernel, ivec3(patch_size, patch_size, _patch_count),  ivec3(8, 8, 1),
                          "shade", e);
    e = _queue.enq_kernel(*_sample_kernel, ivec3(8,8,_patch_count * square(patch_size/8)), ivec3(8,8,1),
                          "sample", _framebuffer_cleared | e);
                        
    active.write_complete = e;
    _last_sample = e;
    _patch_count = 0;
        
}
