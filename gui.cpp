#include "gui.h"

gui::gui()
{
}

gui::~gui()
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

    window = SDL_CreateWindow("Smooth Life", 100, 100, 640, 480, SDL_WINDOW_SHOWN);

    if (window == nullptr)
    {
        print_sdl_error("SDL_CreateWindow Error");
        return;
    }

    // Create the renderer

    renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

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
        if(!render())
        {
            quit = true;
        }
    }
}

bool gui::render()
{
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);

    //filledCircleRGBA(renderer,200,200,(int)3,255,255,255,255);

    for(int x = 0; x < sim->field_size_x;++x)
    {
        for(int y = 0; y < sim->field_size_y;++y)
        {
            Uint8 a = 0;

            const double f = sim->field.M[matrix_index(x,y,sim->field_ld)];

            a = (int)ceil(f * 255);

            //filledCircleRGBA(renderer,(int)(x * scale),(int)(y * scale),(int)(scale * 0.5),a,a,a,255);
            roundedBoxRGBA(renderer, (int)(x * scale), (int)(y * scale), (int)(x*scale + scale), (int)(y * scale + scale),(int)(scale), a,a,a,255);


            /*SDL_SetRenderDrawColor(renderer, a,a,a,255);
            SDL_RenderDrawPoint(renderer, (int)(x * scale), (int)(y * scale));*/
        }
    }


    SDL_RenderPresent(renderer);

    return true;
}

