#pragma once
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include "matrix.h"
#include <iostream>

using namespace std;

class ogl_texture
{
public:
    ogl_texture();
    ~ogl_texture();

    bool loadFromMatrix(aligned_matrix<float> * M);
    bool loadFromPixel(GLfloat *pixels, GLuint width, GLuint height);
    void free();

    GLuint get_texture_id()
    {
        return m_texture_id;
    }

    GLuint get_width()
    {
        return m_texture_width;
    }

    GLuint get_height()
    {
        return m_texture_height;
    }

    static GLuint power_of_two(GLuint n)
    {
        //Src: http://lazyfoo.net/tutorials/OpenGL/08_non_power_of_2_textures/index.php

        if(n != 0)
        {
            --n;
            n |= (n >> 1);
            n |= (n >> 2);
            n |= (n >> 4);
            n |= (n >> 8);
            n |= (n >> 16);
            ++n;
        }

        return n;
    }

private:

    GLuint m_texture_id;
    GLuint m_texture_width;
    GLuint m_texture_height;

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
