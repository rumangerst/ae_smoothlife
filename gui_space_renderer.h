#pragma once
#include "matrix.h"
#include <SDL2/SDL.h>

struct gui_space_renderer
{

    gui_space_renderer()
    {

    }

    virtual ~gui_space_renderer() {}
    virtual void render(matrix<double> * space, SDL_Renderer * renderer, double scale) = 0;
};
