#include "sdl_gui.h"

sdl_gui::sdl_gui() : gui()
{
}

sdl_gui::~sdl_gui()
{
    free();
}

void sdl_gui::free()
{
    if(renderer != nullptr)
        SDL_DestroyRenderer(renderer);
    if(window != nullptr)
        SDL_DestroyWindow(window);

    SDL_Quit();
}

bool sdl_gui::initialize()
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        print_sdl_error("SDL_Init error");
        return false;
    }

    // Load the window

    window = SDL_CreateWindow("Smooth Life SDL", 100, 100, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window == nullptr)
    {
        print_sdl_error("SDL_CreateWindow Error");
        return false;
    }

    // Create the renderer

    renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

    if(renderer == nullptr)
    {
        print_sdl_error("SDL_CreateRenderer error");
        return false;
    }
    
    return true;
}

void sdl_gui::update( bool& running )
{
  SDL_Event e;
  
  while (SDL_PollEvent(&e))
  {
      if (e.type == SDL_QUIT)
      {
	        running = false;
      }
  }
}

void sdl_gui::render()
{
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);

    //TODO: optimize access via synchronization & de-coupling

    for(int x = 0; x < space.getNumCols(); ++x)
    {
        for(int y = 0; y < space.getNumRows(); ++y)
        {
            const double f = space.getValue(x,y);

            Uint8 a = (int)ceil(f * 255);

            SDL_SetRenderDrawColor(renderer, a,a,a,255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);
}

