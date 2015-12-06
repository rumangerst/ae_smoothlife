#include <iostream>
#include <math.h>
#include <vector>
#include <memory>
#include "simulator.h"
#include "gui.h"

using namespace std;


int main()
{
    ruleset rules = ruleset_smooth_life_l_test();
    simulator s(rules);
    s.initialize();

    gui g;
    g.sim = shared_ptr<simulator>(&s);

    #pragma omp parallel
    {
        #pragma omp single nowait
        g.run();

        #pragma omp single nowait
        s.simulate();
    }

    //s.simulate();




    return 0;
}

