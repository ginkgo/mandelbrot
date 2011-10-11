#ifndef STATISTICS_H
#define STATISTICS_H

class Statistics
{
    long _last_fps_calculation;
    long _render_start_time;
    int _frames;
    int _patches_per_frame;

    long _last_bound_n_split;
    long _total_bound_n_split;

    public:

    float  frames_per_second;
    float  ms_per_frame;
    float  ms_per_render_pass;
    float  ms_bound_n_split;
    int    patches_per_frame;
    long   opencl_memory;
    
    public:
        
    Statistics();

    void start_render();
    void end_render();

    void start_bound_n_split();
    void stop_bound_n_split();

    void inc_patch_count();

    void alloc_opencl_memory(long mem_size);
    void free_opencl_memory(long mem_size);

    void update();
    void reset_timer();

    void print();
        
};

extern Statistics statistics;

#endif
