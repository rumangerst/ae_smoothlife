#pragma once

#include <iostream>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;

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

    void set_uniform_texture0(GLuint unit);
    void set_uniform_render_w(float w);
    void set_uniform_render_h(float h);
    void set_uniform_field_w(float w);
    void set_uniform_field_h(float h);

private:

    GLuint program_id = 0;

    // Shader variable locations
    GLint uniform_texture0;
    GLint uniform_render_w;
    GLint uniform_render_h;
    GLint uniform_field_w;
    GLint uniform_field_h;

    bool shader_uniform_location(const char * name, GLint & location);

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
