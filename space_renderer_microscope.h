#pragma once
#include "gui_space_renderer.h"
#include <SDL2_gfxPrimitives.h>
#include <vector>
#include "gui_color.h"

using namespace std;

struct space_renderer_microscope : public gui_space_renderer
{
    color clr_neutral = color(103,129,164,1.0);
    color clr_dark = color(83,107,140,1.0);
    color clr_vdark = color(51,64,81,1.0);
    color clr_bright = color(160,175,190,1.0);
    color clr_green = color(28,89,29,1.0);
    color clr_bgreen = color(55,174,57,1.0);


    vector<color> color_def;
    vector<color> color_def_green;

    space_renderer_microscope()
    {
        color_def.push_back(clr_neutral);
        color_def.push_back(clr_dark);
        color_def.push_back(clr_vdark);
        color_def.push_back(clr_dark);
        color_def.push_back(clr_vdark);
        color_def.push_back(clr_dark);
        color_def.push_back(clr_vdark);
        color_def.push_back(clr_dark);

        color_def_green.push_back(clr_dark);
        color_def_green.push_back(clr_dark);
        color_def_green.push_back(clr_dark);
        color_def_green.push_back(clr_dark);
        color_def_green.push_back(clr_green);
        color_def_green.push_back(clr_dark);
        color_def_green.push_back(clr_bgreen);
        color_def_green.push_back(clr_dark);
        color_def_green.push_back(clr_green);
        color_def_green.push_back(clr_dark);
        color_def_green.push_back(clr_bgreen);

    }

    ~space_renderer_microscope() override
    {

    }

    void render_circle(int x, int y, int r, color clr, SDL_Renderer * renderer)
    {
        SDL_SetRenderDrawColor(renderer, bytecolor(clr.r),bytecolor(clr.g),bytecolor(clr.b),bytecolor(clr.a));

        r = r*r;

        for(int i = x - r; i < x + r; ++i)
        {
            for(int j = y - r; j < y + r;++j)
            {
                if( (i - x)*(i - x) + (j-y)*(j -j) < r )
                    SDL_RenderDrawPoint(renderer, i, j);
            }
        }
    }

    void render_circle_s(cint x, cint y, cint r, color clr, cdouble s, SDL_Renderer * renderer)
    {
        cdouble r_sq = r*r;

        Uint8 ur = bytecolor(clr.r);
        Uint8 ug = bytecolor(clr.g);
        Uint8 ub = bytecolor(clr.b);

        for(int i = x - r; i < x + r; ++i)
        {
            for(int j = y - r; j < y + r;++j)
            {
                cdouble dist_sq = (i - x)*(i - x) + (j-y)*(j -j);

                if( dist_sq  < r_sq )
                {
                    cdouble a = clamp((1.0 - (dist_sq / r_sq)) * s);
                    Uint8 ua = (int)ceil(a * 255);

                    SDL_SetRenderDrawColor(renderer, ur,ug,ub,ua);
                    SDL_RenderDrawPoint(renderer, i, j);
                }
            }
        }
    }

    void render_pixel(SDL_Renderer * renderer, cint x, cint y, color clr)
    {
        SDL_SetRenderDrawColor(renderer, bytecolor(clr.r),bytecolor(clr.g),bytecolor(clr.b),bytecolor(clr.a));
        SDL_RenderDrawPoint(renderer, x, y);
    }

    inline double filling(matrix<double> * space, cint x, cint y)
    {
        double fill = 0;

        for(int i = x - 2; i <= x + 2; ++i)
        {
            for(int j = y - 2; j < y + 2; ++j)
            {
                const double f = space->M[matrix_index_wrapped(i,j,space->columns,space->rows,space->ld)];

                if(f >= 0.95)
                    fill += f;
            }
        }

        return fill / 25.0;
    }

    void render(matrix<double> * space, SDL_Renderer * renderer, double scale)
    {

        SDL_SetRenderDrawColor(renderer, bytecolor(clr_neutral.r),bytecolor(clr_neutral.g),bytecolor(clr_neutral.b),255);
        SDL_RenderClear(renderer);


        for(int x = 0; x < space->columns;++x)
        {
            for(int y = 0; y < space->rows;++y)
            {
                const double f = space->M[matrix_index(x,y,space->ld)];
                const color clr = f < 0.95 ? lerp_color_n(color_def, f) : lerp_color_n(color_def_green, filling(space,x,y));

                render_circle(x * scale,y * scale, scale, clr, renderer);
                //render_pixel(renderer,x,y,clr);
            }
        }

        SDL_RenderPresent(renderer);
    }
};
