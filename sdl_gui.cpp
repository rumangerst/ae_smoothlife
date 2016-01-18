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
    SDL_SetRenderDrawColor(renderer, 0,100,200,255);
    SDL_RenderClear(renderer);
    
     // How many points are needed?
    int wnd_w;
    int wnd_h;
    SDL_GetWindowSize ( window, &wnd_w, &wnd_h );
    float factor = fmax(1, fmin((float)wnd_w / space.getNumCols(), (float)wnd_h / space.getNumRows()));
    
    int render_w = floor(factor * space.getNumCols());
    int render_h = floor(factor * space.getNumRows());   
    
    for(int y = 0; y < render_h; ++y)
    {
        float * row = space.getRow_ptr(floor(y / factor));
        
        for(int x = 0; x < render_w; ++x)
        {
            int x_idx = floor(x / factor);
            float f = row[x_idx];
            Uint8 a = (int)ceil(f * 255);
            
            int x_render = wnd_w / 2 - render_w / 2 + x;
            int y_render = wnd_h / 2 - render_h / 2 + y;
            
            SDL_SetRenderDrawColor(renderer, a,a,a,255);
            SDL_RenderDrawPoint(renderer, x_render, y_render);            
        }
    }
    
    SDL_RenderPresent(renderer);
}

