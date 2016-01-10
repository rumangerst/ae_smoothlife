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
      
      cout << "Role:" << mpi_get_role() << endl;
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
      
      cout << "Role:" << mpi_get_role() << endl;
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
    
    // Enable nested threads
    omp_set_nested(1);
    
    #pragma omp parallel sections
    {       
        #pragma omp section
        {
            cout << "GUI is in thread "<<omp_get_thread_num() << endl;
            g.run_local(&s);
        }

        #pragma omp section
        {
            cout << "Simulator is in thread "<<omp_get_thread_num() << endl;
            s.run_simulation_master();
        }
    }

    return 0;
}
#endif

