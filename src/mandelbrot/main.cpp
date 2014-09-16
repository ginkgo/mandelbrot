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

#include <boost/format.hpp>
#include <algorithm>
#include <random>

#include <Config.h>
#include <GLConfig.h>

#include "Statistics.h"

#include "GL/VBO.h"
#include "GL/Shader.h"

#include "Mandelbrot.h"
#include "Julia.h"

void mainloop(GLFWwindow* window);
bool handle_arguments(int& argc, char** argv);
GLFWwindow* init_opengl(ivec2 window_size);
void get_framebuffer_info();
void resize_window_callback(GLFWwindow* window, int width, int height);

int main(int argc, char** argv)
{
    // Parse config file and command line arguments
    if (!handle_arguments(argc, argv)) {
        return 1;
    }

    ivec2 size = config.window_size();

	GLFWwindow* window = init_opengl(size);
    
    if (window == NULL) {
        return 1;
    }
    
    glfwSetWindowTitle(window, "Mandelbrot");
    glfwSetFramebufferSizeCallback(window, resize_window_callback);
    mainloop(window);
    
    return 0;
}

void mainloop(GLFWwindow* window)
{
    bool running = true;

    double last = glfwGetTime();

    glm::dvec2 last_cursor_pos;
    glfwGetCursorPos(window, &(last_cursor_pos.x), &(last_cursor_pos.y));
    
    Keyboard keys(window);

    long long frame_no = 0;

    Mandelbrot mandelbrot;

    const dvec2 initial_focus(-0.5,0.0);
    const double initial_mag = 2.0/std::min(config.window_size().x,config.window_size().y);
    
    dvec2 focus = initial_focus;
    double mag = initial_mag;

    
    const int julia_window_size = 500;
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow* julia_window = glfwCreateWindow(julia_window_size, julia_window_size, "Julia", NULL, NULL);

    glfwMakeContextCurrent(julia_window);
    Julia julia(julia_window_size);

    glfwMakeContextCurrent(window);
    
    glfwPollEvents();
    while (running) {

        if (keys.any_pressed()) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
            last = glfwGetTime();
        }
        
        double now = glfwGetTime();
        double time_diff = now - last;
        last = now;

        glm::dvec2 cursor_pos;
        glfwGetCursorPos(window, &(cursor_pos.x), &(cursor_pos.y));

        glm::dvec2  mouse_movement = cursor_pos - last_cursor_pos;
        if (glm::length(mouse_movement) > 100) {
            mouse_movement = vec2(0,0);
        }

        if (glm::length(mouse_movement) > 0.0 && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
            focus -= mouse_movement * mag * dvec2(2,-2);
        }

        if (keys.is_down(GLFW_KEY_UP)) {
            mag *= exp(-time_diff);
        }

        if (keys.is_down(GLFW_KEY_DOWN)) {
            mag *= exp(time_diff);
        }

        if (keys.pressed('R')) {
            focus = initial_focus;
            mag = initial_mag;
        }
        
        // Make screenshot
        if (keys.pressed(GLFW_KEY_PRINT_SCREEN)) {
            make_screenshot();
        }
        
        // ---------------------------------------------------------------------
        // Draw mandelbrot
        glfwMakeContextCurrent(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mandelbrot.draw(focus, mag);
        
        glfwSwapBuffers(window);

        // ---------------------------------------------------------------------
        // Draw Julia
        glfwMakeContextCurrent(julia_window);
        glViewport(0,0, julia_window_size, julia_window_size);
        glClearColor(0,1,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        dvec2 screen_center(config.window_size().x/2.0, config.window_size().y/2.0);
        dvec2 c = (cursor_pos-screen_center)*mag*dvec2(2,-2)+focus;
        julia.draw(c);
        
        glfwSwapBuffers(julia_window);
        
        // ---------------------------------------------------------------------
        glfwMakeContextCurrent(window);
        
        // Check if the window has been closed
        running = running && keys.is_up(GLFW_KEY_ESCAPE);
        running = running && keys.is_up('Q');
		running = running && !glfwWindowShouldClose( window );
		running = running && !glfwWindowShouldClose( julia_window );

        frame_no++;

        keys.update();
        //statistics.update();
        last_cursor_pos = cursor_pos;
    }
}



bool handle_arguments(int& argc, char** argv)
{
    bool needs_resave;

    // Config
    
    if (!Config::load_file("mandelbrot.options", config, needs_resave)) {
        cout << "Failed to load mandelbrot.options" << endl;
        return false;
    }

    if (needs_resave) {
        if (config.verbosity_level() > 0) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!Config::save_file("mandelbrot.options", config)) {
            cout << "Failed to save mandelbrot.options" << endl;
            return false;
        }
    }
    
    // GLConfig
    
    if (!GLConfig::load_file("gl.options", gl_config, needs_resave)) {
        cout << "Failed to load gl.options" << endl;
        return false;
    }

    if (needs_resave) {
        if (config.verbosity_level() > 0) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!GLConfig::save_file("gl.options", gl_config)) {
            cout << "Failed to save mandelbrot.options" << endl;
            return false;
        }
    }


    config.parse_args(argc, argv); 
    gl_config.parse_args(argc, argv); 

    return true;
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

    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, gl_config.fsaa_samples());
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


/**
 * Query properties of attached OpenGL Framebuffer
 */
void get_framebuffer_info()
{
    cout << endl << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << " Framebuffer info...                                                            " << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << endl;
    GLint params[10];
    
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, params);
    switch(params[0]) {
    case GL_NONE:
        cout << "Attachment type: NONE" << endl; break;
    case GL_FRAMEBUFFER_DEFAULT:
        cout << "Attachment type: default framebuffer" << endl; break;
    case GL_RENDERBUFFER:
        cout << "Attachment type: renderbuffer" << endl; break;
    case GL_TEXTURE:
        cout << "Attachment type: texture" << endl; break;
    default:
        cout << "Attachment type: unknown" << endl; break;
    }

    std::fill(params, params+10, -1);
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, params);
    cout << "Attachment object name: " << params[0] << endl;

    GLint object_name = params[0];
    
    if (glIsRenderbuffer(object_name)) {
        cout << "Object name is a renderbuffer" << endl;
        
        std::fill(params, params+10, -1);
        glBindRenderbuffer(GL_RENDERBUFFER, object_name);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, params+0);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, params+1);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, params+2);

        cout << "Format: " << params[0] << "x" << params[1] << " @ " << params[2] << " samples" << endl;
    } else {
        cout << "Object name NOT a renderbuffer" << endl;
    }

    if (glIsBuffer(object_name)) {
        cout << "Object name is a buffer" << endl;
    } else {
        cout << "Object name NOT a buffer" << endl;
    }

    if (glIsTexture(object_name)) {
        cout << "Object name is a texture" << endl;
    } else {
        cout << "Object name NOT a texture" << endl;
    }

    if (glIsFramebuffer(object_name)) {
        cout << "Object name is a framebuffer" << endl;
    } else {
        cout << "Object name NOT a framebuffer" << endl;
    }

    cout << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << " Framebuffer info... done                                                       " << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << endl << endl;
}


void resize_window_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0,0,width, height);

    config.set_window_size(ivec2(width,height));
}
