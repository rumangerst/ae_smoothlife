#include "ogl_gui.h"

ogl_gui::ogl_gui() : gui()
{
}

ogl_gui::~ogl_gui()
{
    for (ogl_shader * s : m_shaders)
    {
        delete s;
    }
}

void ogl_gui::update(bool& running, bool & reinitialize)
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            running = false;
        }
        else if (e.type == SDL_KEYUP)
        {
            switch (e.key.keysym.sym)
            {
            case SDLK_s:
                switch_shader();
                break;
            case SDLK_r:
                reinitialize = true;
                break;
            }
        }
    }
}

bool ogl_gui::initialize()
{
    return initSDL() && initGL() && load();
}

bool ogl_gui::initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        print_sdl_error("SDL_Init");
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    m_window = SDL_CreateWindow("Smooth Life OpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_sdl_window_w, m_sdl_window_h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (m_window == nullptr)
    {
        print_sdl_error("SDL_CreateWindow");
        return false;
    }

    m_gl_context = SDL_GL_CreateContext(m_window);

    if (m_gl_context == 0)
    {
        print_sdl_error("SDL_GL_CreateContext");
        return false;
    }

    if (SDL_GL_SetSwapInterval(1) < 0)
    {
        print_sdl_error("Set to VSync");
    }

    return true;
}

bool ogl_gui::load()
{
    if (!load_shaders())
    {
        return false;
    }

    // load texture
    m_space_texture.loadFromMatrix(&space);

    return true;
}

bool ogl_gui::initGL()
{
    //Init glew
    GLenum glew_error = glewInit();

    if (glew_error != GLEW_OK)
    {
        cout << "GLEW error: " << glewGetErrorString(glew_error) << endl;
        return false;
    }

    if (!GLEW_VERSION_2_1)
    {
        cout << "GLEW 2.1 not supportet. exiting." << endl;
        return false;
    }

    glViewport(0, 0, m_sdl_window_w, m_sdl_window_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_sdl_window_w, m_sdl_window_h, 0, 1, -1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(1.0, 1.0, 1.0, 1.0);

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (is_gl_error("Init GL"))
    {
        return false;
    }

    return true;
}

void ogl_gui::render()
{
    //Update OpenGL transformation
    int wnd_w, wnd_h;

    SDL_GetWindowSize(m_window, &wnd_w, &wnd_h);

    if (wnd_w != m_sdl_window_w || wnd_h != m_sdl_window_h)
    {
        //Update OpenGL ortho matrix
        m_sdl_window_w = wnd_w;
        m_sdl_window_h = wnd_h;

        initGL();
    }

    //Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    m_space_texture.loadFromMatrix(&space);

    cdouble w = space.getNumCols();
    cdouble h = space.getNumRows();
    cdouble scale = fmin(m_sdl_window_w / w, m_sdl_window_h / h);
    cdouble sw = scale * w;
    cdouble sh = scale * h;
    cdouble texcoord_w = w / m_space_texture.get_width();
    cdouble texcoord_h = h / m_space_texture.get_height();

    //Activate texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_space_texture.get_texture_id());

    //Tell shader the target size
    m_shaders[m_current_shader]->m_uniform_render_w.set(sw);
    m_shaders[m_current_shader]->m_uniform_render_h.set(sh);

    //Tell field size
    m_shaders[m_current_shader]->m_uniform_field_w.set(w);
    m_shaders[m_current_shader]->m_uniform_field_h.set(h);

    // Tell m_shader other information
    m_shaders[m_current_shader]->m_uniform_time.set(SDL_GetTicks());

    glTranslatef(m_sdl_window_w / 2.0 - sw / 2.0, m_sdl_window_h / 2.0 - sh / 2.0, 0.0);

    glBegin(GL_QUADS);
    //glColor3f( 0.f, 1.f, 1.f );

    glTexCoord2d(0.0, 0.0);
    glVertex2f(0, 0);
    glTexCoord2d(texcoord_w, 0.0);
    glVertex2f(sw, 0);
    glTexCoord2d(texcoord_w, texcoord_h);
    glVertex2f(sw, sh);
    glTexCoord2d(0.0, texcoord_h);
    glVertex2f(0, sh);

    glEnd();

    //Draw OpenGL to SDL window
    SDL_GL_SwapWindow(m_window);

}

bool ogl_gui::load_shaders()
{
    // Load grayscale shader
    {
        ogl_shader * shader = new ogl_shader("Grayscale");

        const char * source_vertex =
#include "shaders/grayscale.vsh"
                ;
        const char * source_pixel =
#include "shaders/grayscale.fsh"
                ;

        if (!shader->load_program(source_vertex, source_pixel))
        {
            return false;
        }

        m_shaders.push_back(shader);
    }

    // Load microscope shader
    {
        ogl_shader * shader = new ogl_shader("Microscope");

        const char * source_vertex =
#include "shaders/microscope.vsh"
                ;
        const char * source_pixel =
#include "shaders/microscope.fsh"
                ;

        if (!shader->load_program(source_vertex, source_pixel))
        {
            return false;
        }

        m_shaders.push_back(shader);
    }

    // Load microscope 2 shader
    {
        ogl_shader * shader = new ogl_shader("Microscope 2");

        const char * source_vertex =
#include "shaders/microscope2.vsh"
                ;
        const char * source_pixel =
#include "shaders/microscope2.fsh"
                ;

        if (!shader->load_program(source_vertex, source_pixel))
        {
            return false;
        }

        m_shaders.push_back(shader);
    }

    // Load blue shader
    {
        ogl_shader * shader = new ogl_shader("Blue");

        const char * source_vertex =
#include "shaders/blue.vsh"
                ;
        const char * source_pixel =
#include "shaders/blue.fsh"
                ;

        if (!shader->load_program(source_vertex, source_pixel))
        {
            return false;
        }

        m_shaders.push_back(shader);
    }

    switch_shader();

    return true;
}

void ogl_gui::switch_shader(int shader_index)
{
    cout << "Switching to shader id" << shader_index << " (" << m_shaders[shader_index]->m_name << ")" << endl;

    if (shader_index != m_current_shader)
    {
        if (m_current_shader != -1)
        {
            m_shaders[m_current_shader]->unbind();
        }

        m_current_shader = shader_index;

        m_shaders[m_current_shader]->bind();
        m_shaders[m_current_shader]->m_uniform_texture0.set(0);

    }
}

void ogl_gui::switch_shader()
{
    switch_shader((m_current_shader + 1) % m_shaders.size());
}

