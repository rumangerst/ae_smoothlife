#include "ogl_gui.h"

ogl_gui::ogl_gui()
{

}

ogl_gui::~ogl_gui()
{
    for(ogl_shader * s : shaders)
    {
        delete s;
    }
}

void ogl_gui::run()
{
    if(init() & load())
    {
        SDL_Event e;
        bool quit = false;
        while (!quit)
        {
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    if(sim != nullptr)
                        sim->running = false;
                    quit = true;
                }
            }

            update_gl();
            render();

            SDL_GL_SwapWindow(window);
        }
    }

    if(sim != nullptr)
        sim->running = false;
}

bool ogl_gui::init()
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        print_sdl_error("SDL_Init");
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    window = SDL_CreateWindow("Smooth Life OpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sdl_window_w, sdl_window_h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if(window == nullptr)
    {
        print_sdl_error("SDL_CreateWindow");
        return false;
    }

    gl_context = SDL_GL_CreateContext(window);

    if(gl_context == 0)
    {
        print_sdl_error("SDL_GL_CreateContext");
        return false;
    }

    if(SDL_GL_SetSwapInterval(1) < 0)
    {
        print_sdl_error("Set to VSync");
    }

    if(!initGL())
    {
        cout << "Cannot init OpenGL!" << endl;
        return false;
    }
    return true;
}

bool ogl_gui::load()
{
    if(!load_shaders())
        return false;

    // load texture
    space_texture.loadFromMatrix(sim->space_current_atomic.load());

    return true;
}

bool ogl_gui::initGL()
{
    //Init glew
    GLenum glew_error = glewInit();

    if(glew_error != GLEW_OK)
    {
        cout << "GLEW error: " << glewGetErrorString(glew_error) << endl;
        return false;
    }

    if(!GLEW_VERSION_2_1)
    {
        cout << "GLEW 2.1 not supportet. exiting." << endl;
        return false;
    }

    glViewport(0,0,sdl_window_w, sdl_window_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,sdl_window_w,sdl_window_h,0,1,-1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(1.0,1.0,1.0,1.0);

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    if(is_gl_error("Init GL"))
    {
        return false;
    }

    return true;
}

void ogl_gui::render()
{
    //Clear color buffer
    glClear( GL_COLOR_BUFFER_BIT );

    glLoadIdentity();

    if(sim != nullptr)
    {
        space_texture.loadFromMatrix(sim->space_current_atomic.load());

        cdouble w = sim->field_size_x;
        cdouble h = sim->field_size_y;
        cdouble scale = fmin(sdl_window_w / w, sdl_window_h / h);
        cdouble sw = scale * w;
        cdouble sh = scale * h;

        //Activate texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, space_texture.get_texture_id());

        //Tell shader the target size
        shaders[current_shader]->uniform_render_w.set(sw);
        shaders[current_shader]->uniform_render_h.set(sh);

        //Tell field size
        shaders[current_shader]->uniform_field_w.set(w);
        shaders[current_shader]->uniform_field_h.set(h);

        // Tell shader other information
        shaders[current_shader]->uniform_time.set(SDL_GetTicks());

        glTranslatef( sdl_window_w / 2.0 - sw / 2.0, sdl_window_h / 2.0 - sh / 2.0, 0.0 );

        glBegin( GL_QUADS );
        //glColor3f( 0.f, 1.f, 1.f );

        glTexCoord2d(0.0,0.0);
        glVertex2f( 0, 0 );
        glTexCoord2d(1.0,0.0);
        glVertex2f( sw, 0 );
        glTexCoord2d(1.0,1.0);
        glVertex2f( sw, sh );
        glTexCoord2d(0.0,1.0);
        glVertex2f( 0, sh );

        glEnd();

    }
}

void ogl_gui::update_gl()
{
    int w,h;

    SDL_GetWindowSize(window, &w, &h);

    if(w != sdl_window_w || h != sdl_window_h)
    {
        //Update OpenGL ortho matrix
        sdl_window_w = w;
        sdl_window_h = h;

        initGL();
    }
}

bool ogl_gui::load_shaders()
{
    ogl_shader * shader = new ogl_shader();
    /*shader->load_program(
                "void main() { gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex; }",
                "void main() { gl_FragColor = vec4( 1.0, 0.0, 0.0, 1.0 ); }");*/


    {
        const char * source_vertex =
        #include "shaders/microscope.vsh"
                ;
        const char * source_pixel =
        #include "shaders/microscope.fsh"
                ;

        if(!shader->load_program(source_vertex, source_pixel))
        {
            return false;
        }
    }

    shaders.push_back(shader);

    //todo make nicer
    current_shader = 0;
    shader->bind();
    shader->uniform_texture0.set(0);

    return true;
}

