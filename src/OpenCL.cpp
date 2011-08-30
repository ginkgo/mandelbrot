#include "OpenCL.h"
#include "Config.h"

#include "GL/glx.h"

#include "utility.h"

namespace
{
    
    void get_opencl_device(int& platform_index, int& device_index,
                           cl_platform_id &platform, cl_device_id &device)
    {
        cl_int status;

        cl_platform_id platforms[10];
        cl_uint num_platforms;

    
        status = clGetPlatformIDs(10, platforms, &num_platforms);

        OPENCL_ASSERT(status);

        if (platform_index >= int(num_platforms)) {
            if (num_platforms == 0) {
                OPENCL_EXCEPTION("No OpenCL platforms available.");
            } else {
                cerr << "WARNING: No OpenCL platform " << platform_index 
                     << " available. Falling back to platform number 0." << endl;
                platform_index = 0;
            }
        }

        cl_device_id devices[10];
        cl_uint num_devices;

        status = clGetDeviceIDs(platforms[platform_index], CL_DEVICE_TYPE_ALL, 
                                 10, devices, &num_devices);

        OPENCL_ASSERT(status);

        if (device_index >= int(num_devices)) {
            if (num_platforms == 0) {
                OPENCL_EXCEPTION("No OpenCL devices available on platform.");
            } else {
                cerr << "WARNING: No OpenCL device " << device_index 
                     << " available. Falling back to device number 0." << endl;
                device_index = 0;
            }        
        }

        device = devices[device_index];
        platform = platforms[platform_index];
    }

    cl_context create_context_with_GL(cl_platform_id platform, cl_device_id device)
    {
        cl_int status;

        cl_context_properties props[] = 
            {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 
             CL_GL_CONTEXT_KHR,   (cl_context_properties)glXGetCurrentContext(),
             CL_GLX_DISPLAY_KHR,  (cl_context_properties)XOpenDisplay(":0.0"),
             0};
        cl_context context = clCreateContext(props, 
                                             1, &device,
                                             NULL, NULL, &status);

        OPENCL_ASSERT(status);

        return context;
    }

    cl_context create_context_without_GL(cl_platform_id platform, cl_device_id device)
    {
        cl_int status;

        cl_context_properties props[] = 
            {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
        cl_context context = clCreateContext(props, 
                                             1, &device,
                                             NULL, NULL, &status);

        OPENCL_ASSERT(status);

        return context;
    }

    bool is_GPU_device(cl_device_id device)
    {
        cl_device_type type;
        cl_int status;

        status = clGetDeviceInfo(device, CL_DEVICE_TYPE,
                                 sizeof(type), &type, NULL);

        OPENCL_ASSERT(status);

        return type == CL_DEVICE_TYPE_GPU;
    }
}

namespace CL
{
    Device::Device(int platform_index, int device_index) 
    {
        cl_platform_id platform;

        get_opencl_device(platform_index, device_index, platform, _device);

        if (!config.disable_buffer_sharing() && is_GPU_device(_device)) {
            try {
                _context = create_context_with_GL(platform, _device);
                _share_gl = true;
            } catch (Exception& e) {
                _context = create_context_without_GL(platform, _device);
                _share_gl = false;
            }
        } else {
            _context = create_context_without_GL(platform, _device);
            _share_gl = false;
            
        }
    }

    Device::~Device()
    {
        clReleaseContext(_context);
    }

    Buffer::Buffer(Device& device, size_t size, cl_mem_flags flags)
    {
        cl_int status;
        _buffer = clCreateBuffer(device.get_context(), flags, size, NULL,
                                 &status);

        OPENCL_ASSERT(status);
    }

    Buffer::Buffer(Device& device, size_t size, cl_mem_flags flags, void** host_ptr)
    {
        cl_int status;
        _buffer = clCreateBuffer(device.get_context(), 
                                 flags | CL_MEM_ALLOC_HOST_PTR, 
                                 size, NULL, &status);

        OPENCL_ASSERT(status);

        status = clGetMemObjectInfo(_buffer, CL_MEM_HOST_PTR, 
                                    sizeof(void*), host_ptr, NULL);

        OPENCL_ASSERT(status);
    }

    Buffer::Buffer(Device& device, GLuint GL_buffer) 
    {
        cl_int status;

        _buffer = clCreateFromGLBuffer(device.get_context(), 
                                       CL_MEM_WRITE_ONLY, GL_buffer, &status);

        OPENCL_ASSERT(status);
    }

    Buffer::~Buffer()
    {
        clReleaseMemObject(_buffer);
    }

    size_t Buffer::get_size() const
    {
        size_t size;
        cl_int status = clGetMemObjectInfo(_buffer, CL_MEM_SIZE, sizeof(size), &size, NULL);

        OPENCL_ASSERT(status);

        return size;
    }

    CommandQueue::CommandQueue(Device& device)
    {
        cl_int status;
        _queue = clCreateCommandQueue(device.get_context(), device.get_device(),
                                      CL_QUEUE_PROFILING_ENABLE, &status);

        OPENCL_ASSERT(status);
    }

    CommandQueue::~CommandQueue()
    {
        clFinish(_queue);
        clReleaseCommandQueue(_queue);
    }


    void CommandQueue::enq_kernel(Kernel& kernel, int global_size, int local_size)
    {
        size_t offset[] = {0};
        size_t global[] = {global_size};
        size_t local[]  = {local_size};
        

        cl_int status;
        status = clEnqueueNDRangeKernel(_queue, kernel.get(),
                                         1, offset, global, local,
                                         0, NULL, NULL);

        OPENCL_ASSERT(status);
    }


    void CommandQueue::enq_kernel(Kernel& kernel, ivec2 global_size, ivec2 local_size)
    {
        size_t offset[] = {0,0};
        size_t global[] = {global_size.x,global_size.y};
        size_t local[]  = {local_size.x, local_size.y};
        

        cl_int status;
        status = clEnqueueNDRangeKernel(_queue, kernel.get(),
                                         2, offset, global, local,
                                         0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_kernel(Kernel& kernel, ivec3 global_size, ivec3 local_size)
    {
        size_t offset[] = {0,0,0};
        size_t global[] = {global_size.x,global_size.y,global_size.z};
        size_t local[]  = {local_size.x, local_size.y, local_size.z};
        

        cl_int status;
        status = clEnqueueNDRangeKernel(_queue, kernel.get(),
                                         3, offset, global, local,
                                         0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_write_buffer(Buffer& buffer, void* src,
                                        size_t len, size_t offset)
    {
        cl_int status;

        status = clEnqueueWriteBuffer(_queue, buffer.get(), CL_FALSE, 
                                      offset, len, src,
                                      0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_read_buffer(Buffer& buffer, void* src,
                                        size_t len, size_t offset)
    {
        cl_int status;

        status = clEnqueueReadBuffer(_queue, buffer.get(), CL_FALSE, 
                                     offset, len, src, 0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::finish()
    {
        cl_int status = clFinish(_queue);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_acquire(ImageBuffer& buffer)
    {
        cl_mem mem = buffer.get();
        cl_int status = clEnqueueAcquireGLObjects(_queue, 1, &mem, 
                                                  0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_release(ImageBuffer& buffer)
    {
        cl_mem mem = buffer.get();
        cl_int status = clEnqueueReleaseGLObjects(_queue, 1, &mem, 
                                                  0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_acquire(Buffer& mem)
    {
        cl_mem m = mem.get();
        cl_int status = clEnqueueAcquireGLObjects(_queue, 1, &m, 0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_release(Buffer& mem)
    {
        cl_mem m = mem.get();
        cl_int status = clEnqueueReleaseGLObjects(_queue, 1, &m, 0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_acquire(cl_mem mem)
    {
        cl_int status = clEnqueueAcquireGLObjects(_queue, 1, &mem, 0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_release(cl_mem mem)
    {
        cl_int status = clEnqueueReleaseGLObjects(_queue, 1, &mem, 0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void* CommandQueue::map_buffer(Buffer& buffer)
    {
        cl_int status;

        void* mapped = clEnqueueMapBuffer(_queue, buffer.get(), CL_TRUE, CL_MAP_READ,
                                          0, buffer.get_size(), 0, 0, 0,
                                          &status);
        OPENCL_ASSERT(status);

        return mapped;
    }

    void CommandQueue::unmap_buffer(Buffer& buffer, void* mapped)
    {
        cl_int status = clEnqueueUnmapMemObject(_queue, buffer.get(), mapped, 
                                                0, NULL, NULL);
        OPENCL_ASSERT(status);
    }

    ImageBuffer::ImageBuffer(Device& device, GL::Texture& texture, cl_mem_flags flags)
    {
        cl_int status;

        _buffer = clCreateFromGLTexture2D(device.get_context(), flags,
                                          GL_TEXTURE_2D, 0, 
                                          texture.texture_name(),
                                          &status);

        OPENCL_ASSERT(status);
    }

    ImageBuffer::~ImageBuffer()
    {
        clReleaseMemObject(_buffer);
    }

    Kernel::Kernel(Device& device, 
                   const string& filename, const string& kernelname)
    {
        string file = config.kernel_dir()+"/"+filename;

        if (!file_exists(file)) {
            OPENCL_EXCEPTION("Failed to load OpenCL kernel program '" + file + "'."); 
        }

        string file_content = read_file(file);
        
        const char* c_content = file_content.c_str();
        size_t content_size = file_content.size();

        cl_int status;

        _program = clCreateProgramWithSource(device.get_context(), 1, 
                                             &c_content, &content_size,
                                             &status);
        OPENCL_ASSERT(status);

        cl_device_id dev = device.get_device();

        status = clBuildProgram(_program, 1, &dev, "", NULL, NULL);

        if (status != CL_SUCCESS && status != CL_BUILD_PROGRAM_FAILURE) {
            OPENCL_ASSERT(status);
        }

        cl_build_status build_status;
        
        status = clGetProgramBuildInfo(_program, dev, 
                                       CL_PROGRAM_BUILD_STATUS,
                                       sizeof(build_status), &build_status, 
                                       NULL);

        OPENCL_ASSERT(status);

        if (build_status != CL_BUILD_SUCCESS) {
            size_t size = 16 * 1024;
            char *buffer = new char[size];

            status = clGetProgramBuildInfo(_program, dev,
                                           CL_PROGRAM_BUILD_LOG,
                                           size, buffer, NULL);
            OPENCL_ASSERT(status);

            cerr << buffer << endl;

            delete[] buffer;
            
            OPENCL_EXCEPTION("Failed to build program '" + file + "'");
        }



        _kernel = clCreateKernel(_program, kernelname.c_str(), &status);
        OPENCL_ASSERT(status);        
    }

    Kernel::~Kernel()
    {
        clReleaseKernel(_kernel);
        clReleaseProgram(_program);
    }


    Exception::Exception(cl_int err_code, const string& file, int line_no):
        _file(file), _line_no(line_no) 
    {
        get_errors();

        switch(err_code) {
        case CL_INVALID_PROGRAM: 
            _msg = "Invalid program"; break;
        case CL_INVALID_PROGRAM_EXECUTABLE: 
            _msg = "Program not built successfully"; break;
        case CL_INVALID_KERNEL_NAME: 
            _msg = "Kernel name not found in program"; break;
        case CL_INVALID_KERNEL_DEFINITION:
            _msg = "Invalid kernel definition"; break;
        case CL_INVALID_KERNEL:
            _msg = "Invalid kernel"; break;
        case CL_INVALID_KERNEL_ARGS:
            _msg = "(Some) kernel args not specified"; break;
        case CL_INVALID_WORK_DIMENSION:
            _msg = "Invalid work dimension"; break;
        case CL_INVALID_GLOBAL_WORK_SIZE:
            _msg = "Invalid global work size"; break;
        case CL_INVALID_GLOBAL_OFFSET:
            _msg = "Invalid global offset"; break;
        case CL_INVALID_WORK_GROUP_SIZE:
            _msg = "Invalid work group size"; break;
        case CL_INVALID_WORK_ITEM_SIZE:
            _msg = "Invalid work item size"; break;
        case CL_INVALID_IMAGE_SIZE:
            _msg = "Invalid image size"; break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            _msg = "Mem-object allocation failure"; break;
        case CL_INVALID_VALUE:
            _msg = "Invalid value"; break;
        case CL_OUT_OF_RESOURCES:
            _msg = "Out of resources"; break;
        case CL_OUT_OF_HOST_MEMORY:
            _msg = "Out of host memory"; break;
        case CL_INVALID_CONTEXT:
            _msg = "Invalid context"; break;
        case CL_INVALID_MIP_LEVEL:
            _msg = "Invalid miplevel"; break;
        case CL_INVALID_GL_OBJECT:
            _msg = "Invalid OpenGL object"; break;
        case CL_INVALID_OPERATION:
            _msg = "Invalid operation"; break;
        case CL_INVALID_PROPERTY:
            _msg = "Invalid property"; break;
        case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
            _msg = "Invalid OpenGL sharegroup reference"; break;
        case CL_INVALID_DEVICE:
            _msg = "Invalid device"; break;
        case CL_INVALID_BINARY:
            _msg = "Invalid binary"; break;
        case CL_INVALID_BUILD_OPTIONS:
            _msg = "Invalid build options"; break;
        case CL_COMPILER_NOT_AVAILABLE:
            _msg = "Compiler not available"; break;
        case CL_BUILD_PROGRAM_FAILURE:
            _msg = "Program build failure"; break;
        case CL_INVALID_COMMAND_QUEUE:
            _msg = "Invalid command queue"; break;
        case CL_INVALID_ARG_INDEX:
            _msg = "Invalid kernel argument index"; break;
        case CL_INVALID_ARG_VALUE:
            _msg = "Invalid kernel argument value"; break;
        case CL_INVALID_SAMPLER:
            _msg = "Invalid sampler object"; break;
        case CL_INVALID_ARG_SIZE:
            _msg = "Invalid kernel argument size"; break;
        case -1001:
            _msg = "Vendor ICD not correctly installed(?)"; break;
        default:
            _msg = "Unknown error: " + to_string(err_code);
        }
    }

    Exception::Exception(const string& msg, const string& file, int line_no):
        _msg(msg), _file(file), _line_no(line_no) 
    {

    }

    const string& Exception::msg()
    {
        return _msg;
    }


    template <typename T> void print_device_param(cl_device_id device,
                                                  cl_device_info param_enum,
                                                  string param_name, 
                                                  const string& indent)
    {
        T tmp;
        cl_int status;

        status = clGetDeviceInfo(device, param_enum, sizeof(T), &tmp, NULL);

        if (status == CL_SUCCESS) {
            cout << indent << "  " << param_name << ": " << tmp << endl;
        } else {
            cout << indent << "  " << param_name << ": " << "<unsupported>" << endl;
        }
    }
                 
    void print_device_workgroup_size(cl_device_id device, 
                                     const string& indent)
    {
        cl_uint max_dim;
        size_t sizes[50];

        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, 
                        sizeof(cl_uint), &max_dim, NULL);
        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, 
                        sizeof(size_t)*max_dim, sizes, NULL);

        cout << indent << "  CL_DEVICE_MAX_WORK_ITEM_SIZES:";

        for (uint i = 0; i < max_dim; ++i) {
            cout << " " << sizes[i];
        }

        cout << endl;
    }
                 
    void print_device_queue_properties(cl_device_id device, 
                                       const string& indent)
    {
        cl_command_queue_properties props;

        clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, 
                        sizeof(props), &props, NULL);
        cout << indent << "  CL_DEVICE_QUEUE_PROPERTIES:";

        if (props | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
            cout << " out-of-order:YES";
        } else {
            cout << " out-of-order:NO";
        }

        if (props | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
            cout << ", profiling:YES";
        } else {
            cout << ", profiling:NO";
        }

        cout << endl;
    }

    #define PRINT_CL_DEVICE_INFO(type, name)                \
    print_device_param<type>(_device, name, #name, indent)

    void Device::print_info()
    {
        const string indent = "    ";
        cout << endl << indent << "OpenCL Device:" << endl;
        PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_NAME);
        PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_VERSION);
        PRINT_CL_DEVICE_INFO(char[1000], CL_DRIVER_VERSION);
        if (config.verbose()) {
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_ADDRESS_BITS);
            PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_AVAILABLE);
            PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_COMPILER_AVAILABLE);
            PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_ENDIAN_LITTLE);
            PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_ERROR_CORRECTION_SUPPORT);
            PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
            PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_GLOBAL_MEM_SIZE);
            PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_HOST_UNIFIED_MEMORY);
            PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_IMAGE_SUPPORT);
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE2D_MAX_WIDTH);
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE2D_MAX_HEIGHT); 
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE3D_MAX_WIDTH);
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE3D_MAX_HEIGHT); 
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE3D_MAX_DEPTH);
            PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_LOCAL_MEM_SIZE);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_CLOCK_FREQUENCY);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_COMPUTE_UNITS);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_CONSTANT_ARGS);
            PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
            PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_MAX_MEM_ALLOC_SIZE);
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_MAX_PARAMETER_SIZE);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_READ_IMAGE_ARGS);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_SAMPLERS);
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_MAX_WORK_GROUP_SIZE);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
            print_device_workgroup_size(_device, indent);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_WRITE_IMAGE_ARGS);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MEM_BASE_ADDR_ALIGN);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF);
            PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_OPENCL_C_VERSION);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
            PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF);
            PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_PROFILE);
            PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_PROFILING_TIMER_RESOLUTION);
            print_device_queue_properties(_device, indent);
            PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_EXTENSIONS);
        }
    }
}


