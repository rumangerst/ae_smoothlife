#include <iostream>
#include <math.h>
#include <vector>
#include <omp.h>
#include <memory>
#include <exception>
#include "communication.h"
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
int run_master(int argc, char ** argv)
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
            g.run(&s);
        }

#pragma omp section
        {
            cout << "Simulator is in thread " << omp_get_thread_num() << endl;
            s.run_simulation_master();
        }
    }

    return EXIT_SUCCESS;
}

#endif

/**
 * @brief Run as slave simulator. Communicate over mpi
 */
int run_slave(int argc, char ** argv)
{
    ruleset rules = ruleset_from_cli(argc, argv);
    simulator s(rules);
    s.initialize();
    s.run_simulation_slave();

    return EXIT_SUCCESS;
}

/**
 * @brief Run master simulator without GUI. Communicate over mpi
 */
int run_master_perftest(int argc, char ** argv)
{
    ruleset rules = ruleset_from_cli(argc, argv);
    simulator s(rules);
    s.initialize();
    s.run_simulation_master();

    return EXIT_SUCCESS;
}

int main(int argc, char ** argv)
{
    setup_openmp();

    try
    {
        mpi_manager mpi(argc, argv);

        if (mpi_get_role() == mpi_role::SIMULATOR_MASTER)
        {
            if (APP_PERFTEST)
            {
                return run_master_perftest(argc, argv);
            }
            else
            {
#if APP_GUI
                return run_master(argc, argv);
#else
                cerr << "Cannot run this program as master! No GUI!" << endl;
#endif
            }
        }
        else
        {
            return run_slave(argc, argv);
        }

    }
    catch (exception & ex)
    {
        cerr << "Exception | " << ex.what() << endl;

        return EXIT_FAILURE;
    }

    cerr << "Did not start anything!" << endl;
    return EXIT_FAILURE;

}


