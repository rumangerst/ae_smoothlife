#pragma once

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <iostream>
#include <vector>
#include <atomic>
#include <memory>
#include "simulator.h"
#include "ogl_shader.h"
#include "ogl_texture.h"

using namespace std;

class ogl_gui
{
public:
     atomic<matrix<float> *>* space = nullptr;
     atomic<bool> is_space_drawn_once; //TODO: is probably obsolete with MPI, but still still good for testing first!
     atomic<bool>* new_space_available; // if true, wait for the master simulator to change the current space
     bool * simulator_status = nullptr;

     ulong num_images_rendered = 0; // just for debugging

    ogl_gui();
    ~ogl_gui();

    /**
     * @brief runs an (in)finite loop for the opengl rederer
     */
    void run();

    /**
     * @brief checks whether or not a new image should be processed (imo)
     * @return TRUE for asynchronize rendering or certain properties
     */
    bool allowNextStep();

private:

    SDL_Window * window = nullptr;
    SDL_GLContext gl_context;

    int sdl_window_w = 800;
    int sdl_window_h = 600;


    vector<ogl_shader *> shaders;
    int current_shader = -1;

    ogl_texture space_texture;

    bool init();
    bool initGL();

    bool load();

    bool load_shaders();

    void update_gl();

    void render();

    void print_sdl_error(const char * msg)
    {
        cerr << "SDL error | " << msg << ": " << SDL_GetError() << endl;
    }

    void print_gl_error(const char * msg, GLenum err)
    {
        cerr << "OpenGL error | " << msg << ": " << gluErrorString(err) << endl;
    }

    bool is_gl_error(const char * msg)
    {
        GLenum error = glGetError();

        if(error == GL_NO_ERROR)
        {
            return false;
        }
        else
        {
            print_gl_error(msg, error);
            return true;
        }
    }

    void switch_shader(int shader_index);
    void switch_shader();

    void handle_events(SDL_Event e);

};
