#pragma once

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <iostream>
#include <vector>
#include <atomic>
#include <memory>
#include <chrono>
#include "gui.h"
#include "simulator.h"
#include "ogl_shader.h"
#include "ogl_texture.h"

using namespace std;

class ogl_gui : public gui
{
public:  

    ogl_gui();
    ~ogl_gui() override;
    
protected:
  
  bool initialize() override;
  void update( bool& running, bool & reinitialize ) override;
  void render() override;
   
private:

    SDL_Window * m_window = nullptr;
    SDL_GLContext m_gl_context;

    int m_sdl_window_w = 800;
    int m_sdl_window_h = 600;


    vector<ogl_shader *> m_shaders;
    int m_current_shader = -1;

    ogl_texture m_space_texture;
   
    bool initSDL();
    bool initGL();

    bool load();

    bool load_shaders();

    void update_gl();
  

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

};
