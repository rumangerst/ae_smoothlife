#include "gui.h"

gui::gui()
{
}

gui::~gui()
{
    free();
}

void gui::free()
{
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

    window = SDL_CreateWindow("Smooth Life SDL", 100, 100, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

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
                if(simulator_status != nullptr)
                    *simulator_status = false;
                quit = true;
            }
        }

        //Render the scene
        render();
    }

   if(simulator_status != nullptr)
        *simulator_status = false;
}

void gui::render()
{
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);

    for(int x = 0; x < space->load()->columns; ++x)
    {
        for(int y = 0; y < space->load()->rows; ++y)
        {
            const double f = space->load()->M[matrix_index(x,y,space->load()->ld)];

            Uint8 a = (int)ceil(f * 255);

            SDL_SetRenderDrawColor(renderer, a,a,a,255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);
}

