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


#include "common.h"

#include "Patch.h"
#include "Config.h"

#include "OpenCL.h"
#include "Reyes.h"

#include "Statistics.h"

#include "PrefixSum.h"
#include "Buffer.h"

#include <boost/format.hpp>

void mainloop(GLFWwindow* window);
bool test_prefix_sum(const int N, bool print);
void handle_arguments(int argc, char** argv);
GLFWwindow* init_opengl(ivec2 window_size);

int main(int argc, char** argv)
{
    handle_arguments(argc, argv);

    ivec2 size = config.window_size();

	GLFWwindow* window = init_opengl(size);
	
    if (window == NULL) {
        return 1;
    }
    
    cout << endl;
    cout << "MICROPOLIS - A micropolygon rasterizer" << " (c) Thomas Weber 2012" << endl;
    cout << endl;

    glfwSetWindowTitle(window, config.window_title().c_str());

    try {

        mainloop(window);

    } catch (CL::Exception& e) {

        cerr << e.file() << ":" << e.line_no() << ": error: " <<  e.msg() << endl;

    }

    return 0;
}

void mainloop(GLFWwindow* window)
{
    // CL::Device device(config.platform_id(), config.device_id());

    // if (config.verbose()) {
    //     device.print_info();
    // }

    Reyes::Scene scene(config.input_file());
    
    Reyes::WireGLRenderer wire_renderer;
    shared_ptr<Reyes::PatchDrawer> renderer;
    
    switch (config.renderer_type()) {
    case Config::OPENCL:
        renderer.reset(new Reyes::OpenCLRenderer());
        break;
    case Config::GLTESS:
        renderer.reset(new Reyes::HWTessRenderer());
        break;
    default:
        assert(0);
    }

    bool running = true;

    statistics.reset_timer();
    double last = glfwGetTime();

    glm::dvec2 last_cursor_pos;
    glfwGetCursorPos(window, &(last_cursor_pos.x), &(last_cursor_pos.y));
    
    bool in_wire_mode = false;
    bool last_f3_state = glfwGetKey(window, GLFW_KEY_F3);

    // for (auto N : {2, 20, 100, 128, 200, 512, 800, 1000, 1024, 2048, 4096, 5000, 128*128, 128*128*128, 50000}) {
    //     if (test_prefix_sum(N, false)) cout << format("Prefix sum on %1% items succeeded") % N << endl;
    //     else                           cout << format("Prefix sum on %1% items failed") % N << endl;
    // }    
    // return;
    
    while (running) {

        glfwPollEvents();
        
        double now = glfwGetTime();
        double time_diff = now - last;
        last = now;

        // Camera navigation
        vec3 translation((glfwGetKey(window, 'A') ? -1 : 0) + (glfwGetKey(window, 'D') ? 1 : 0), 0,
                         (glfwGetKey(window, 'W') ? -1 : 0) + (glfwGetKey(window, 'S') ? 1 : 0));
        translation *= time_diff * 2;

        glm::dvec2 cursor_pos;
        glfwGetCursorPos(window, &(cursor_pos.x), &(cursor_pos.y));

        vec2 rotation(0,0);
        float zrotation = 0.0f;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
            rotation = (cursor_pos - last_cursor_pos) * 0.1;
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
            zrotation = (cursor_pos.x - last_cursor_pos.x) * 0.4f;
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS) {
            translation.x += (cursor_pos.x - last_cursor_pos.x) * -0.01;
            translation.y += (cursor_pos.y - last_cursor_pos.y) * 0.01;
        }
        last_cursor_pos = cursor_pos;

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            translation *= 2.0f;
            zrotation   *= 2.0f;
        }
        
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
            translation *= 0.25f;
            rotation    *= 0.50f;
            zrotation   *= 0.50f;
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP)) {
            config.set_bound_n_split_limit(config.bound_n_split_limit() * pow(0.75, time_diff));
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN)) {
            config.set_bound_n_split_limit(config.bound_n_split_limit() * pow(0.75,-time_diff));
        }
        
        scene.active_cam().transform = scene.active_cam().transform
            * glm::translate<float>(translation.x, translation.y, translation.z)
            * glm::rotate<float>(rotation.x, 0,1,0)
            * glm::rotate<float>(rotation.y, 1,0,0)
            * glm::rotate<float>( zrotation, 0,0,1);

        // Wireframe toggle
        bool f3_state = glfwGetKey(window, GLFW_KEY_F3);
        if (f3_state && !last_f3_state) {
            in_wire_mode = !in_wire_mode;
            statistics.reset_timer();
        }
        last_f3_state = f3_state;

        // Render scene
        statistics.start_render();
        if (in_wire_mode) {
            scene.draw(wire_renderer);
        } else {
            scene.draw(*renderer);
        }
        
        glfwSwapBuffers(window);
        statistics.end_render();

        statistics.update();

        // Check if the window has been closed
        running = running && !glfwGetKey( window, GLFW_KEY_ESCAPE );
        running = running && !glfwGetKey( window,  'Q' );
		running = running && !glfwWindowShouldClose( window );

        		
    }
}




bool test_prefix_sum(const int N, bool print)
{
    bool retval = true;
    
    GL::PrefixSum prefix_sum(N);

    GL::Buffer i_buffer(N * sizeof(ivec2));
    GL::Buffer o_buffer(N * sizeof(ivec2));
    GL::Buffer t_buffer(sizeof(ivec2));

    vector<ivec2> i_vec(N);
    vector<ivec2> o_vec(N);

    srand(43);
    if (print) cout << "INPUT: "; 
    for (size_t i = 0; i < (size_t)N; ++i) {
        i_vec[i] = ivec2(1, rand()%8+1);
        o_vec[i] = ivec2(0, 0);

        if (print) cout << boost::format("(%1%, %2%) ") % i_vec[i].x % i_vec[i].y;
    }
    if (print) cout << endl;

    i_buffer.bind(GL_ARRAY_BUFFER);
    i_buffer.send_subdata(i_vec.data(), 0, N * sizeof(ivec2));
    i_buffer.unbind();

    o_buffer.bind(GL_ARRAY_BUFFER);
    o_buffer.send_subdata(o_vec.data(), 0, N * sizeof(ivec2));
    o_buffer.unbind();

    prefix_sum.apply(N, i_buffer, o_buffer, t_buffer);

    ivec2 total;

    o_buffer.bind(GL_ARRAY_BUFFER);
    o_buffer.read_data(o_vec.data(), N * sizeof(ivec2));
    o_buffer.unbind();
    
    t_buffer.bind(GL_ARRAY_BUFFER);
    t_buffer.read_data(&total, sizeof(ivec2));
    t_buffer.unbind();
    
    ivec2 sum(0);
    if (print) cout << "OUTPUT: "; 
    for (size_t i = 0; i < (size_t)N; ++i) {
        sum += i_vec[i];

        if (sum != o_vec[i]) {
            retval = false;
        }
        
        if (print) cout << boost::format("(%1%, %2%) ") % o_vec[i].x % o_vec[i].y;
    }
    if (print) cout << endl;

    if (sum != total) {
        retval = false;
    }

    if (print) cout << boost::format("TOTAL: (%1%, %2%)") % total.x % total.y << endl;
    
    return retval;
}


void handle_arguments(int argc, char** argv)
{
    bool needs_resave;

    if (!Config::load_file("options.txt", config, needs_resave)) {
        cout << "Failed to load options.txt" << endl;
    }

    if (needs_resave) {
        if (config.verbosity_level() > 0) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!Config::save_file("options.txt", config)) {
            cout << "Failed to save options.txt" << endl;
        }
    }

    argc = config.parse_args(argc, argv);
    
}


/* 
 * Helper function that properly initializes the GLFW window before opening.
 */
GLFWwindow* init_opengl(ivec2 size)
{
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return GL_FALSE;
    }

    int version = FLEXT_MAJOR_VERSION * 10 + FLEXT_MINOR_VERSION;

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, config.fsaa_samples());
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    // Create window and OpenGL context
    GLFWwindow* window = glfwCreateWindow(size.x, size.y, "Window title", NULL, NULL);

	if (!window) {
        cerr << "Failed to create OpenGL window." << endl;
		return NULL;
	}
	
	glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    
    // Call flext's init function.
    int success = flextInit(window);

    if (!success) {
        glfwTerminate();
    }

    set_GL_error_callbacks();
    
    return window;
}
