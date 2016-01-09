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
    }

    virtual ~gui() 
    {
    }

    /**
     * @brief Run the GUI with a local simulator
     */
    void run_local ( simulator * sim ) 
    {
/*#ifdef ENABLE_PERF_MEASUREMENT
        auto perf_time_start = chrono::high_resolution_clock::now();
        ulong frames_start = 0;
#endif*/
        
        //Initialize the GUI
        if ( !initialize() ) 
        {
            cerr << "GUI | Error while initialization!" << endl;
            return;
        }
        
        // predefine the space of the renderer
        space = vectorized_matrix<float>(sim->rules.get_space_width(), sim->rules.get_space_height());
        
        cout << "Local GUI started ..." << endl;

        running = true;

        while ( running ) 
        {
            // Is there a new space in simulator queue? -> update renderer space then!
            if(sim->space_queue.size() != 0)
            {
                space.overwrite(sim->space_queue.front());
                sim->space_queue.pop();
            }
            
            update ( running );
            render();      
/*#ifdef ENABLE_PERF_MEASUREMENT
                ++frames_rendered;
                if ( frames_rendered%100 == 0 ) 
                {
                    auto perf_time_end = chrono::high_resolution_clock::now();
                    double perf_time_seconds = chrono::duration<double> ( perf_time_end - perf_time_start ).count();

                    cout << "GUI || " << ( frames_rendered - frames_start ) / perf_time_seconds << " FPS" << endl;

                    frames_start = frames_rendered;
                    perf_time_start = chrono::high_resolution_clock::now();
                }

#endif*/
        }
        
        cout << "Local GUI quit." << endl;
        
        // End simulator when GUI exits
        sim->running = false;
    }

    /*
     * @brief Run the GUI with MPI
     */
    void run_mpi ( mpi_manager * mgr ) 
    {
    }

protected:
    
    vectorized_matrix<float> space;

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
