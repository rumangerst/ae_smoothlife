#pragma once

#include <iostream>
#include <string>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "ogl_shader_variable.h"

using namespace std;

/**
 * @brief Encapsulates an OpenGL shader program (pixel and vertex shader)
 * @author Ruman
 */
class ogl_shader
{
public:
    ogl_shader();
    virtual ~ogl_shader();

    virtual void free();

    bool load_program(const char * vertex_shader_code, const char * pixel_shader_code);

    bool bind();
    void unbind();

    GLuint get_program_id()
    {
        return program_id;
    }

    shader_uniform_variable_texture_unit uniform_texture0 = shader_uniform_variable_texture_unit("texture0");
    shader_uniform_variable_1f uniform_render_w = shader_uniform_variable_1f("render_w");
    shader_uniform_variable_1f uniform_render_h = shader_uniform_variable_1f("render_h");
    shader_uniform_variable_1f uniform_field_w = shader_uniform_variable_1f("field_w");
    shader_uniform_variable_1f uniform_field_h = shader_uniform_variable_1f("field_h");
    shader_uniform_variable_1f uniform_time = shader_uniform_variable_1f("time");


private:

    GLuint program_id = 0;

    void bind_variables();
    void unbind_variables();

protected:

    void print_program_log(GLuint program);
    void print_shader_log(GLuint shader);

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

};
