#include "ogl_texture.h"

ogl_texture::ogl_texture()
{
    m_texture_id = 0;
    m_texture_width = 0;
    m_texture_height = 0;
}

ogl_texture::~ogl_texture()
{
    free();
}

bool ogl_texture::loadFromMatrix(aligned_matrix<float> * M)
{
    // The texture size should be power of two
    cint tex_w = power_of_two(M->getNumCols());
    cint tex_h = power_of_two(M->getNumRows());

    vector<GLfloat> pixels(tex_w * tex_h);

    for(int x = 0; x < M->getNumCols(); ++x)
    {
        for(int y = 0; y < M->getNumRows(); ++y)
        {
            cfloat f = M->getValue(x,y);
            //const unsigned char v = floor(f * 255);

            //pixels[matrix_index(x,y,texture_width)] = v << 24 | v << 16 | v << 8 | 255;

             pixels[matrix_index(x,y,tex_w)] = f;
        }
    }

    return loadFromPixel(pixels.data(), tex_w, tex_h);
}

bool ogl_texture::loadFromPixel(GLfloat *pixels, GLuint width, GLuint height)
{
    //cout << "creating new texture: loading from pixels. w=" << width << " h=" << height << endl;

    free();

    m_texture_width = width;
    m_texture_height = height;    

    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    if(is_gl_error("loading texture from pixel"))
    {
        return false;
    }

    return true;
}

void ogl_texture::free()
{
    if(m_texture_id != 0)
    {
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
    }

    m_texture_width = 0;
    m_texture_height = 0;
}

