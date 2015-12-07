#include "gui.h"

gui::gui()
{
}

gui::~gui()
{
    delete space_renderer;

    if(renderer != nullptr)
        SDL_DestroyRenderer(renderer);
    if(window != nullptr)
        SDL_DestroyWindow(window);

    SDL_Quit();
}

void gui::run()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        print_sdl_error("SDL_Init error");
        return;
    }

    // Load the window

    window = SDL_CreateWindow("Smooth Life", 100, 100, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window == nullptr)
    {
        print_sdl_error("SDL_CreateWindow Error");
        return;
    }

    // Create the renderer

    renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

    if(renderer == nullptr)
    {
        print_sdl_error("SDL_CreateRenderer error");
        error_occurred = true;
        return;
    }

    // The main loop

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

        //Render the scene
        update_scale();
        space_renderer->render(sim->space_current_atomic.load(),renderer,scale);
    }

    if(sim != nullptr)
        sim->running = false;
}

void gui::update_scale()
{
    if(sim != nullptr)
    {
        int w,h;
        SDL_GetWindowSize(window,&w,&h);

        scale = fmax(1, fmin( (double)w / sim->field_size_x, (double)h / sim->field_size_y ));
    }
}

