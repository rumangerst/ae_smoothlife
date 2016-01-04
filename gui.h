#pragma once
#include "simulator.h"
#include "mpi_manager.h"

/**
 * @brief General GUI class with MPI support
 */
class gui
{
public:

    gui() 
    {
        is_space_drawn_once = false;
    }

    virtual ~gui() 
    {
    }

    /**
     * @brief Run the GUI with a local simulator
     */
    void run_local ( simulator * sim ) 
    {
#ifdef ENABLE_PERF_MEASUREMENT
        auto perf_time_start = chrono::high_resolution_clock::now();
        ulong frames_start = 0;
#endif
        
        //Connect simulator and GUI
        space = &sim->space_of_renderer;
        sim->is_space_drawn_once_by_renderer = &is_space_drawn_once;
        new_space_available = &sim->new_space_available;

        //Initialize the GUI
        if ( !initialize() ) 
        {
            cerr << "GUI | Error while initialization!" << endl;
            return;
        }
        
        cout << "Local GUI started ..." << endl;

        running = true;

        while ( running ) 
        {
            update ( running );
            
            if(new_space_available->load())
            {
                render();            
                is_space_drawn_once.store(true);
            }
        }
        
        cout << "Local GUI quit." << endl;
        
        // End simulator when GUI exits
        sim->running = false;

#ifdef ENABLE_PERF_MEASUREMENT
        ++frames_rendered;
        if ( frames_rendered%100 == 0 ) 
        {
            auto perf_time_end = chrono::high_resolution_clock::now();
            double perf_time_seconds = chrono::duration<double> ( perf_time_end - perf_time_start ).count();

            cout << "OpenGL GUI || " << ( frames_rendered - frames_start ) / perf_time_seconds << " FPS" << endl;

            frames_start = frames_rendered;
            perf_time_start = chrono::high_resolution_clock::now();
        }

#endif
    }

    /*
     * @brief Run the GUI with MPI
     */
    void run_mpi ( mpi_manager * mgr ) 
    {
    }

protected:
    
    atomic<vectorized_matrix<float> *>* space = nullptr;
    atomic<bool> is_space_drawn_once; //TODO: is probably obsolete with MPI, but still still good for testing first!
    atomic<bool>* new_space_available = nullptr;

    /**
     * @brief do any initialization tasks
     * @return true if initialization was successful, otherwise false
     */
    virtual bool initialize() = 0;
    virtual void update ( bool & running ) = 0;
    virtual void render() = 0;

private:

    bool running = false;

#ifdef ENABLE_PERF_MEASUREMENT
    ulong frames_rendered = 0; // needed for performance measurement
#endif
};
