#pragma once

#include <iostream>
#include <atomic>
#include <vector>
#include <SDL2/SDL.h>
#include <memory>
#include "simulator.h"

using namespace std;

/**
 * @brief SDL legacy GUI. Provides basic drawing of the current state. Use OpenGL GUI for more sophisticated rendering.
 * @author Ruman
 */
class gui
{
public:

    atomic<matrix<double> *>* space = nullptr;
    atomic<bool> is_space_drawn_once; //TODO: is probably obsolete with MPI, but still still good for testing first!
    atomic<bool>* new_space_available;
    bool * simulator_status = nullptr;

    gui();
    ~gui();

    void run();
    void free();
private:

    bool error_occurred = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    void render();

    void print_sdl_error(const char * msg)
    {
        cerr << "SDL error | " << msg << ": " << SDL_GetError() << endl;
    }
};

