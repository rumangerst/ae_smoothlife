#pragma once

#include <iostream>
#include <atomic>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL2_rotozoom.h>
#include <memory>
#include "simulator.h"
#include "gui_space_renderer.h"
#include "space_renderer_grayscale.h"
#include "space_renderer_microscope.h"

using namespace std;

class gui
{
public:

    double scale = 2;
    shared_ptr<simulator> sim = nullptr;

    gui();
    ~gui();

    void run();
private:

    bool error_occurred = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    gui_space_renderer * space_renderer = new space_renderer_microscope();


    void print_sdl_error(const char * msg)
    {
        cerr << "SDL error | " << msg << ": " << SDL_GetError() << endl;
    }

    void update_scale();

    bool render_grayscale();
    bool render_microscope();
};

