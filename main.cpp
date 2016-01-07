#include <iostream>
#include <math.h>
#include <vector>
#include <omp.h>
#include <memory>
#include <exception>
#include "mpi_manager.h"

// If this exe is a simulator, use simulator class
// otherwise only include matrix
#if APP_SIM
#include "simulator.h"
#else
#include "matrix.h"
#endif

// Include the GUI headers if this is the GUI
#if APP_GUI
#include "sdl_gui.h"
#include "ogl_gui.h"
#endif

using namespace std;

#if !APP_GUI && APP_SIM
int main(int argc, char ** argv)
{
    try
    {
      mpi_manager mpi(argc, argv);
      
      cout << "Role:" << mpi.role() << endl;
    }
    catch(exception & ex)
    {
      cerr << "Exception | " << ex.what() << endl;
      
      return -1;
    }
    
    return 0;
}
#endif

#if APP_GUI && !APP_SIM
int main(int argc, char ** argv)
{
    try
    {
      mpi_manager mpi(argc, argv);
      
      cout << "Role:" << mpi.role() << endl;
    }
    catch(exception & ex)
    {
      cerr << "Exception | " << ex.what() << endl;
      
      return -1;
    }
    
    return 0;
}
#endif

#if APP_GUI && APP_SIM
int main(int argc, char ** argv)
{
    cerr << "Implement main_gui_local" << endl;
    ruleset rules = ruleset_from_cli(argc, argv);
    simulator s(rules);
    s.initialize();

    GUI_TYPE g;
    
    #pragma omp parallel
    {
        printf("OMP: Number of threads available: %i\n", omp_get_max_threads());
        #pragma omp single nowait
        g.run_local(&s);

        #pragma omp single nowait
        s.run_simulation_master();
    }

    return 0;
}
#endif

