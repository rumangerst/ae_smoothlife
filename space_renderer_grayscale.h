#pragma once
#include "gui_space_renderer.h"
#include <SDL2_gfxPrimitives.h>

#include <iostream>
using namespace std;

struct space_renderer_grayscale : public gui_space_renderer
{
    space_renderer_grayscale()
    {

    }

    ~space_renderer_grayscale() override
    {

    }

    void render(matrix<double> * space, SDL_Renderer * renderer, double scale)
    {
        SDL_SetRenderDrawColor(renderer, 0,0,0,255);
        SDL_RenderClear(renderer);

        for(int x = 0; x < space->columns;++x)
        {
            for(int y = 0; y < space->rows;++y)
            {
                const double f = space->M[matrix_index(x,y,space->ld)];

                Uint8 a = (int)ceil(f * 255);

                if(scale > 1)
                {
                    roundedBoxRGBA(renderer, (int)(x * scale), (int)(y * scale), (int)(x*scale + scale), (int)(y * scale + scale),(int)(scale), a,a,a,255);
                }
                else
                {

                    SDL_SetRenderDrawColor(renderer, a,a,a,255);
                    SDL_RenderDrawPoint(renderer, (int)(x * scale), (int)(y * scale));
                }
            }
        }


        SDL_RenderPresent(renderer);
    }
};
