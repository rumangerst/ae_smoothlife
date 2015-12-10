#pragma once

#include <iostream>
#include <string>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;

template <typename T>
/**
 * @brief Encapsulates a shader variable (attribute or uniform)
 * @author Ruman
 */
class shader_variable
{
protected:
    shader_variable(const char * name)
    {
        this->name = string(name);
    }

    // The shader program
    GLuint program_id;

    // Location in the shader
    GLint location = -1;

    //Name of the variable in the shader
    string name;

public:

    /**
     * @brief Try to bind the underlying variable
     * @param program_id
     * @return
     */
    virtual bool bind(GLuint program_id) = 0;

    /**
     * @brief Set the value of this variable
     * @param value
     */
    virtual void set(T value) = 0;

    /**
     * @brief Free the underlying variable
     */
    void unbind()
    {
        location = -1;
    }

};

template <typename T>
/**
 * @brief Handles 'uniform' shader variables
 * @author Ruman
 */
class shader_uniform_variable : public shader_variable<T>
{
protected:
    shader_uniform_variable(const char * name) : shader_variable<T>(name)
    {
    }
public:

    bool bind(GLuint program_id) override
    {
        this->program_id = program_id;
        this->location = glGetUniformLocation(program_id, this->name.c_str());

        if(this->location == -1)
        {
            cerr << "Cannot get location of shader variable " << this->name << endl;
            return false;
        }

        cout << "Successfully bound shader variable " << this->name << endl;

        return true;
    }
};

/**
 * @brief Encapsulates a texture unit shader variable (uniform sampler2D texture unit)
 * @author Ruman
 */
class shader_uniform_variable_texture_unit : public shader_uniform_variable<GLuint>
{
public:
    shader_uniform_variable_texture_unit(const char * name) : shader_uniform_variable(name)
    {

    }

    ~shader_uniform_variable_texture_unit()
    {

    }

    void set(GLuint value) override
    {
        glUniform1i(location, value);
    }
};

/**
 * @brief Encapsulates a texture unit shader variable (uniform float)
 */
class shader_uniform_variable_1f : public shader_uniform_variable<float>
{
public:
    shader_uniform_variable_1f(const char * name) : shader_uniform_variable(name)
    {

    }

    ~shader_uniform_variable_1f()
    {

    }

    void set(float value) override
    {
        glUniform1f(location, value);
    }
};

