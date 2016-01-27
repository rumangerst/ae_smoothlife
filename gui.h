#pragma once
#include "simulator.h"
#include "communication.h"

/**
 * @brief General GUI class with MPI support
 */
class gui
{
public:

    gui() { }

    virtual ~gui() { }

    /**
     * @brief Run the GUI with a simulator
     */
    void run(simulator * sim)
    {

#ifdef ENABLE_PERF_MEASUREMENT
        auto perf_time_start = chrono::high_resolution_clock::now();
        ulong frames_start = 0;
#endif

        //Initialize the GUI
        if (!initialize())
        {
            cerr << "GUI | Error while initialization!" << endl;
            sim->m_running = false;
            return;
        }

        // predefine the space of the renderer
        space = aligned_matrix<float>(sim->m_rules.get_space_width(), sim->m_rules.get_space_height());

        cout << "GUI started ..." << endl;

        running = true;

        while (running)
        {
            // Try to pop queue to current space
            if(sim->m_space->pop(space))
                ++spacetime;

            update(running, sim->m_reinitialize);
            render();

#ifdef ENABLE_PERF_MEASUREMENT
            ++frames_rendered;
            if (frames_rendered % 100 == 0)
            {
                auto perf_time_end = chrono::high_resolution_clock::now();
                double perf_time_seconds = chrono::duration<double> (perf_time_end - perf_time_start).count();

                cout << "GUI || " << (frames_rendered - frames_start) / perf_time_seconds << " FPS" << endl;

                frames_start = frames_rendered;
                perf_time_start = chrono::high_resolution_clock::now();
            }

#endif
        }

        cout << "GUI quit." << endl;

        // End simulator when GUI exits
        sim->m_running = false;
    }

protected:

    aligned_matrix<float> space;

    /**
     * @brief do any initialization tasks
     * @return true if initialization was successful, otherwise false
     */
    virtual bool initialize() = 0;
    virtual void update(bool & running, bool & reinitialize) = 0;
    virtual void render() = 0;
    
    int get_spacetime()
    {
        return spacetime;
    }

private:
    ulong spacetime = 0;
    bool running = false;

#ifdef ENABLE_PERF_MEASUREMENT
    ulong frames_rendered = 0; // needed for performance measurement
#endif
};
