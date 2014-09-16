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


#include "Statistics.h"

#include "utility.h"
#include "Config.h"

#include <fstream>
    
Statistics statistics;

Statistics::Statistics()
    : _frames(0)
    , frames_per_second(0.0f)
    , ms_per_frame(0.0f)
    , opengl_memory(0)
{
    _last_fps_calculation = nanotime();
}

void Statistics::alloc_opengl_memory(long mem_size)
{
    opengl_memory += mem_size;
}

void Statistics::free_opengl_memory(long mem_size)
{
    opengl_memory -= mem_size;
}


void Statistics::update()
{
    ++_frames;
    uint64_t now = nanotime();
    uint64_t dur = now - _last_fps_calculation;

    if (dur > 1 * BILLION) {
        frames_per_second = (float)_frames * BILLION / dur;
        ms_per_frame = dur / ((float)_frames * MILLION);

        _last_fps_calculation = now;
        _frames = 0;

        print();
    }
}

void Statistics::reset_timer()
{
    _last_fps_calculation = nanotime();
    _frames = 0;
}

void Statistics::print()
{
    if (config.verbosity_level() > 0) {

        cout << endl
             << ms_per_frame << " ms/frame, (" << frames_per_second  << " fps)" << endl;
        
        cout << memory_size(opengl_memory) << "allocated in OpenGL context" << endl;
    } else {
        cout  << ms_per_frame << " ms/frame, (" << frames_per_second  << " fps)" << endl;
    }
}


void Statistics::dump_stats()
{
    std::ofstream fs(config.statistics_file().c_str());

    fs << "opengl_mem = " << opengl_memory << ";" << endl;
}
