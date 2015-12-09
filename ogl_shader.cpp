#include "ogl_shader.h"

ogl_shader::ogl_shader()
{
}

ogl_shader::~ogl_shader()
{
    free();
}

void ogl_shader::free()
{
    glDeleteProgram(program_id);
}

bool ogl_shader::bind()
{
    glUseProgram(program_id);

    if(is_gl_error("Binding shader"))
    {
        print_program_log(program_id);
        return false;
    }

    // Bind shader attributes and uniforms
    shader_uniform_location("texture0", uniform_texture0);
    shader_uniform_location("render_w", uniform_render_w);
    shader_uniform_location("render_h", uniform_render_h);
    shader_uniform_location("field_w", uniform_field_w);
    shader_uniform_location("field_h", uniform_field_h);

    return true;
}

void ogl_shader::unbind()
{
    glUseProgram(0);
}

void ogl_shader::print_shader_log(GLuint shader)
{
    //Make sure name is shader
    if( glIsShader( shader ) )
    {
        //Shader log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;

        //Get info string length
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );

        //Allocate string
        char* infoLog = new char[ maxLength ];

        //Get info log
        glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );

        if( infoLogLength > 0 )
        {
            //Print Log
            cout << infoLog << endl;
        }

        //Deallocate string
        delete[] infoLog;
    }
    else
    {
        cout << "Name " << shader << " is not a shader" << endl;
    }
}

void ogl_shader::print_program_log(GLuint program)
{
    //Make sure name is shader
    if( glIsProgram( program ) )
    {
        //Program log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;

        //Get info string length
        glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );

        //Allocate string
        char* infoLog = new char[ maxLength ];

        //Get info log
        glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );

        if( infoLogLength > 0 )
        {
            //Print Log
            cout << infoLog << endl;

        }

        //Deallocate string
        delete[] infoLog;
    }
    else
    {
        cout << "Name " << program << " is not a program" << endl;
    }
}

bool ogl_shader::load_program(const char *vertex_shader_code, const char *pixel_shader_code)
{
    program_id = glCreateProgram();

    //Compile vertex shader and check for errors
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_code, 0);

    glCompileShader(vertex_shader);

    GLint vertex_shader_compiled = GL_FALSE;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_compiled);

    if(vertex_shader_compiled != GL_TRUE)
    {
        cout << "Error while compiling shader " << vertex_shader << endl;
        print_shader_log(vertex_shader);

        return false;
    }

    glAttachShader(program_id, vertex_shader);

    //Compile pixel shader and check for errors
    GLuint pixel_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pixel_shader, 1, &pixel_shader_code, 0);

    glCompileShader(pixel_shader);

    GLint pixel_shader_compiled = GL_FALSE;
    glGetShaderiv(pixel_shader, GL_COMPILE_STATUS, &pixel_shader_compiled);

    if(pixel_shader_compiled != GL_TRUE)
    {
        cout << "Error while compiling shader " << pixel_shader << endl;
        print_shader_log(pixel_shader);

        return false;
    }

    glAttachShader(program_id, pixel_shader);

    //Link shader
    glLinkProgram(program_id);

    GLint program_linked = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &program_linked);

    //Cleanup
    glDeleteShader(vertex_shader);
    glDeleteShader(pixel_shader);

    if(program_linked != GL_TRUE)
    {
        cout << "Error while linking " << program_id << endl;
        print_program_log(program_id);

        return false;
    }

    return true;
}

bool ogl_shader::shader_uniform_location(const char *name, GLint &location)
{
    location = glGetUniformLocation(program_id, name);

    if(location == -1)
    {
        cerr << "Cannot get location of shader variable " << name << endl;
        return false;
    }

    return true;
}

void ogl_shader::set_uniform_texture0(GLuint unit)
{
    glUniform1i(uniform_texture0, unit);
}

void ogl_shader::set_uniform_render_w(float w)
{
    glUniform1f(uniform_render_w, w);
}

void ogl_shader::set_uniform_render_h(float h)
{
    glUniform1f(uniform_render_h, h);
}

void ogl_shader::set_uniform_field_w(float w)
{
    glUniform1f(uniform_field_w, w);
}

void ogl_shader::set_uniform_field_h(float h)
{
    glUniform1f(uniform_field_h, h);
}
