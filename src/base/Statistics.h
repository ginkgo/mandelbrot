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


#ifndef STATISTICS_H
#define STATISTICS_H

#include "common.h"

class Statistics
{
    uint64_t _last_fps_calculation;
    uint64_t _render_start_time;
    int _frames;
    int _patches_per_frame;
    
    public:

    float    frames_per_second;
    float    ms_per_frame;
    uint64_t opengl_memory;
    
    public:
        
    Statistics();

    void alloc_opengl_memory(long mem_size);
    void free_opengl_memory(long mem_size);

    void update();
    void reset_timer();

    void print();
    void dump_stats();
        
};

extern Statistics statistics;

#endif
