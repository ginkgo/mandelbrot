#pragma once

#include "common.h"

#include "CL/OpenCL.h"
#include "CL/PrefixSum.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "PatchRange.h"
#include "Projection.h"

namespace Reyes
{

    struct Batch
    {
        size_t patch_count;
        CL::Buffer& patch_buffer;
        CL::Buffer& patch_ids;
        CL::Buffer& patch_min;
        CL::Buffer& patch_max;
        CL::Event transfer_done;
    };
    
    class PatchIndex;    


    
    class BoundNSplitCL
    {

    public:

        virtual ~BoundNSplitCL() {};
        
    public:

        virtual void init(void* patches_handle, const mat4& matrix, const Projection* projection) = 0;
        virtual bool done() = 0;
        virtual void finish() = 0;

        virtual Batch do_bound_n_split(CL::Event& ready) = 0;
    };

    
    
    class BoundNSplitCLCPU : public BoundNSplitCL
    {

        CL::CommandQueue& _queue;
                
        shared_ptr<PatchIndex> _patch_index;

        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        vector<PatchRange> _stack;

        mat4 _mv;
        mat4 _mvp;
        const Projection* _projection;

        
    public:

        
        BoundNSplitCLCPU(CL::Device& device, CL::CommandQueue& queue,
                             shared_ptr<PatchIndex>& patch_index);

        virtual void init(void* patches_handle,
                          const mat4& matrix, const Projection* projection);
        virtual bool done();
        virtual void finish();

        virtual Batch do_bound_n_split(CL::Event& ready);

        
    private:

        
        CL::UserEvent _bound_n_split_event;

        enum BatchStatus {
            INACTIVE,    // Currently unused
            SET_UP,      // Data set up and transfer queued
            ACCEPTED     // Picked up by rasterization stages
        };
                
        
        struct BatchRecord 
        {
            BatchStatus status;
            
            CL::TransferBuffer patch_ids;
            CL::TransferBuffer patch_min;
            CL::TransferBuffer patch_max;
            
            CL::Event transferred;
            CL::Event rasterizer_done;

            BatchRecord(size_t batch_size, CL::Device& device, CL::CommandQueue& queue);

            BatchRecord(BatchRecord&& other);         
            BatchRecord& operator=(BatchRecord&& other);

            void transfer(CL::CommandQueue& queue, size_t patch_count, const CL::Event& events);
            
            void accept(CL::Event& event);
            CL::Event finish(CL::CommandQueue& queue);
        };

        vector<BatchRecord> _batch_records;
        size_t _next_batch_record;


    private:

        
        static void vsplit_range(const PatchRange& r, vector<PatchRange>& stack);
        static void hsplit_range(const PatchRange& r, vector<PatchRange>& stack);
        static void bound_patch_range (const PatchRange& r, const BezierPatch& p,
                                       const mat4& mv, const mat4& mvp,
                                       BBox& box, float& vlen, float& hlen);

        
    };


    class BoundNSplitCLLocal : public BoundNSplitCL
    {
        
        CL::CommandQueue& _queue;                
        shared_ptr<PatchIndex> _patch_index;
        
        CL::Program _bound_n_split_program;

        shared_ptr<CL::Kernel> _bound_n_split_kernel;
        shared_ptr<CL::Kernel> _init_range_buffers_kernel;
        shared_ptr<CL::Kernel> _init_projection_buffer_kernel;
        shared_ptr<CL::Kernel> _init_count_buffers_kernel;

        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        mat4 _active_matrix;

        size_t _in_buffers_size;
        size_t _in_buffer_stride;
        CL::Buffer _in_pids_buffer;
        CL::Buffer _in_mins_buffer;
        CL::Buffer _in_maxs_buffer;
        CL::Buffer _in_range_cnt_buffer;
        
        CL::Buffer _out_pids_buffer;
        CL::Buffer _out_mins_buffer;
        CL::Buffer _out_maxs_buffer;
        CL::TransferBuffer _out_range_cnt_buffer;

        CL::Buffer _projection_buffer;

        CL::Event _ready;

        bool _done;
        int _iteration_count;
        
    public:


        BoundNSplitCLLocal(CL::Device& device, CL::CommandQueue& queue,
                           shared_ptr<PatchIndex>& patch_index);
        

        virtual void init(void* patches_handle,
                          const mat4& matrix, const Projection* projection);
        virtual bool done();
        virtual void finish();

        virtual Batch do_bound_n_split(CL::Event& ready);
    };


    class BoundNSplitCLMultipass : public BoundNSplitCL
    {

        CL::CommandQueue& _queue;
        shared_ptr<PatchIndex> _patch_index;
        
        CL::Program _bound_n_split_program;

        shared_ptr<CL::Kernel> _bound_kernel;
        shared_ptr<CL::Kernel> _move_kernel;
        shared_ptr<CL::Kernel> _init_ranges_kernel;
        shared_ptr<CL::Kernel> _init_projection_buffer_kernel;
        
        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        mat4 _active_matrix;
        int _stack_height;
        
        CL::Buffer _pid_stack;
        CL::Buffer _depth_stack;
        CL::Buffer _min_stack;
        CL::Buffer _max_stack;

        CL::TransferBuffer _split_ranges_cnt_buffer;
        
        CL::Buffer _out_pids_buffer;
        CL::Buffer _out_mins_buffer;
        CL::Buffer _out_maxs_buffer;
        CL::TransferBuffer _out_range_cnt_buffer;

        CL::Buffer _bound_flags;
        CL::Buffer _split_flags;
        CL::Buffer _draw_flags;
        
        CL::Buffer _pid_pad;
        CL::Buffer _depth_pad;
        CL::Buffer _min_pad;
        CL::Buffer _max_pad;

        CL::Buffer _projection_buffer;

        CL::Event _ready;

        CL::PrefixSum _prefix_sum;
        
    public:

        
        BoundNSplitCLMultipass(CL::Device& device, CL::CommandQueue& queue,
                               shared_ptr<PatchIndex>& patch_index);
        

        virtual void init(void* patches_handle,
                          const mat4& matrix, const Projection* projection);
        virtual bool done();
        virtual void finish();

        virtual Batch do_bound_n_split(CL::Event& ready);
        
    };
    
    
}
