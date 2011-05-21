#ifndef OPENCL_H
#define OPENCL_H

#include "common.h"
#include "CL/opencl.h"
#include "Texture.h"

namespace OpenCL
{

    class Device
    {
        cl_context   _context;
        cl_device_id _device;

        public:
        
        Device(int platform_index, int device_index);
        ~Device();

        cl_device_id get_device()  { return _device; }
        cl_context   get_context() { return _context; }

        void print_info();
    };

    class ImageBuffer
    {
        cl_mem _buffer;

        public:

        ImageBuffer(Device& device, Texture& texture, cl_mem_flags flags);
        ~ImageBuffer();

        cl_mem get() { return _buffer; }
    };

    class Kernel
    {
        cl_kernel _kernel;
        cl_program _program;

        public:

        Kernel(Device& device, const string& filename, const string& kernelname);
        ~Kernel();

        template<typename T> void set_arg  (cl_uint arg_index, T buffer);
        template<typename T> void set_arg_r(cl_uint arg_index, T& buffer);

        cl_kernel get() { return _kernel;}
    };

    class CommandQueue
    {
        cl_command_queue _queue;

        public:

        CommandQueue(Device& device);
        ~CommandQueue();
        
        void enq_kernel(Kernel& kernel, ivec2 global_size, ivec2 local_size);
        void enq_GL_acquire(ImageBuffer& buffer);
        void enq_GL_release(ImageBuffer& buffer);

        void finish();
    };

    class Exception
    {
        string _msg;
        string _file;
        int _line_no;

        public:

        Exception(cl_int err_code,   const string& file, int line_no);
        Exception(const string& msg, const string& file, int line_no);
        
        const string& msg();
        const string& file() { return _file; }
        const int line_no() { return _line_no; }
    };
    
}

#define OPENCL_EXCEPTION(error) throw OpenCL::Exception((error), __FILE__, __LINE__)
#define OPENCL_ASSERT(error) if ((error)!= CL_SUCCESS) throw OpenCL::Exception((error), __FILE__, __LINE__)

namespace OpenCL
{
    template<typename T> 
    inline void Kernel::set_arg(cl_uint arg_index, T value) 
    {
        cl_int status;

        status = clSetKernelArg(_kernel, arg_index, sizeof(T), &value);
        OPENCL_ASSERT(status);
    }

    template<typename T> 
    inline void Kernel::set_arg_r(cl_uint arg_index, T& value) 
    {
        cl_int status;

        status = clSetKernelArg(_kernel, arg_index, sizeof(T), &value);
        OPENCL_ASSERT(status);
    }

    template<>
    inline void Kernel::set_arg_r<ImageBuffer>(cl_uint arg_index, ImageBuffer& value)
    {
        cl_mem mem = value.get();
        set_arg(arg_index, mem);
    }
}
#endif
