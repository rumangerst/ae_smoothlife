#include <iostream>
#include <math.h>
#include <vector>
#include <omp.h>
#include <memory>
#include <exception>
#include "mpi_manager.h"
#include "simulator.h"

// Include the GUI headers if this is the GUI
#if APP_GUI
#include "sdl_gui.h"
#include "ogl_gui.h"
#endif

using namespace std;

/**
 * Set some necessary settings for openmp
 */
void setup_openmp()
{
    // Enable nested threads
    omp_set_nested(1);
}

#if APP_GUI

/**
 * @brief Runs this simulation and GUI without MPI.
 */
int run_local(int argc, char ** argv)
{
    ruleset rules = ruleset_from_cli(argc, argv);
    simulator s(rules);
    s.initialize();

    GUI_TYPE g;

#pragma omp parallel sections
    {
#pragma omp section
        {
            cout << "GUI is in thread " << omp_get_thread_num() << endl;
            g.run_local(&s);
        }

#pragma omp section
        {
            cout << "Simulator is in thread " << omp_get_thread_num() << endl;
            s.run_simulation_local();
        }
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Run as GUI only. Communicate over mpi
 */
int run_gui(int argc, char ** argv)
{
    ruleset rules = ruleset_from_cli(argc, argv);
    GUI_TYPE g;

    g.run_mpi(rules);

    return EXIT_SUCCESS;
}


#endif

/**
 * @brief Run as simulator only. Communicate over mpi
 */
int run_simulator(int argc, char ** argv)
{
    if (mpi_get_role() == mpi_role::SIMULATOR_SLAVE)
    {
        cerr << "SIMULATOR_SLAVE not implemented!" << endl;
        return EXIT_FAILURE;
    }

    ruleset rules = ruleset_from_cli(argc, argv);
    simulator s(rules);
    s.initialize();
    s.run_simulation_master();

    return EXIT_SUCCESS;
}

/**
 * @brief Run as simulator only. Communicate over mpi
 */
int run_simulator_perftest(int argc, char ** argv)
{
    if (mpi_get_role() == mpi_role::SIMULATOR_SLAVE)
    {
        cerr << "SIMULATOR_SLAVE not implemented!" << endl;
        return EXIT_FAILURE;
    }

    ruleset rules = ruleset_from_cli(argc, argv);
    simulator s(rules);
    s.initialize();
    s.run_simulation_master_perftest();

    return EXIT_SUCCESS;
}

int main(int argc, char ** argv)
{
    setup_openmp();

    try
    {
        mpi_manager mpi(argc, argv);
        
#if APP_PERFTEST

        return run_simulator_perftest(argc, argv);

#else
        if (mpi_comm_size() == 1)
        {
#if APP_GUI
            return run_local(argc, argv);
#else
            cerr << "This application is compiled to be simulator only! Terminating." << endl;
            return EXIT_FAILURE;
#endif
        }
        else
        {
            switch (mpi_get_role())
            {
            case mpi_role::USER_INTERFACE:
#if APP_GUI
                return run_gui(argc, argv);
#else
                cerr << "This application is compiled to be simulator only! Terminating." << endl;
                return EXIT_FAILURE;
#endif
            case mpi_role::SIMULATOR_MASTER:
                return run_simulator(argc, argv);
                
            case mpi_role::SIMULATOR_SLAVE:
                return run_simulator(argc, argv);
                
            default:
                
                cerr << "Invalid role!" << endl;
                return EXIT_FAILURE;
            }
        }
#endif
    }
    catch (exception & ex)
    {
        cerr << "Exception | " << ex.what() << endl;

        return EXIT_FAILURE;
    }

    cerr << "Did not start anything!" << endl;
    return EXIT_FAILURE;

}


