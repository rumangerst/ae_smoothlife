#pragma once

#include <iostream>
#include <atomic>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <memory>
#include "simulator.h"

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

    void print_sdl_error(const char * msg)
    {
        cerr << "SDL error | " << msg << ": " << SDL_GetError() << endl;
    }

    void update_scale();

    bool render();
};

