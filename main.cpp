#include <iostream>
#include <math.h>
#include <vector>
#include <memory>

// If this exe is a simulator, use simulator class
// otherwise only include matrix
#ifdef SIM
#include "simulator.h"
#else
#include "matrix.h"
#endif

// Include the GUI headers if this is the GUI
#ifdef GUI
#include "gui.h"
#include "ogl_gui.h"
#endif

using namespace std;

#define SDL_GUI false
#define RULESET ruleset_smooth_life_l

#if !defined(GUI) && defined(SIM)
int main()
{
    //TODO: Implement
    cerr << "Implement main_sim" << endl;
    return -1;
}
#endif

#if defined(GUI) && !defined(SIM)
int main()
{
    //TODO: Implement
    cerr << "Implement main_gui" << endl;
    return -1;
}
#endif

#if defined(GUI) && defined(SIM)
int main()
{
    cerr << "Implement main_gui_local" << endl;
    ruleset rules = RULESET();
    simulator s(rules);
    s.initialize();

#if SDL_GUI == true
    gui g;
#else
    ogl_gui g;
#endif

    g.simulator_status = &s.running;
    g.space = &s.space_of_renderer;

    #pragma omp parallel
    {
        #pragma omp single nowait
        g.run();

        #pragma omp single nowait
        s.simulate();
    }

    return 0;
}
#endif

