#include <iostream>
#include <math.h>
#include <vector>
#include <memory>
#include "simulator.h"
#include "gui.h"
#include "ogl_gui.h"

using namespace std;

#define SDL_GUI false


int main()
{    
    ruleset rules = ruleset_smooth_life_l();
    simulator s(rules);
    s.initialize();

#if SDL_GUI == true
    gui g;
#else
    ogl_gui g;
#endif
    g.simulator_status = &s.running;
    g.space = &s.space_current_atomic;

    #pragma omp parallel
    {
        #pragma omp single nowait
        g.run();

        #pragma omp single nowait
        s.simulate();
    }

    return 0;
}

