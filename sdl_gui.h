#pragma once

#include <iostream>
#include <atomic>
#include <vector>
#include <SDL2/SDL.h>
#include <memory>
#include <math.h>
#include "simulator.h"
#include "gui.h"

using namespace std;

/**
 * @brief SDL legacy GUI. Provides basic drawing of the current state. Use OpenGL GUI for more sophisticated rendering.
 * @author Ruman
 */
class sdl_gui : public gui
{
public:

    sdl_gui();
    ~sdl_gui() override;

    void free();
    
protected:
  
  bool initialize() override;
  void update(bool & running) override;
  void render() override;
    
private:   

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    void print_sdl_error(const char * msg)
    {
        cerr << "SDL error | " << msg << ": " << SDL_GetError() << endl;
    }
};

