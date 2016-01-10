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
     * @brief Run the GUI with a local simulator
     */
    void run_local(simulator * sim)
    {

#ifdef ENABLE_PERF_MEASUREMENT
        auto perf_time_start = chrono::high_resolution_clock::now();
        ulong frames_start = 0;
#endif

        //Initialize the GUI
        if (!initialize())
        {
            cerr << "GUI | Error while initialization!" << endl;
            sim->running = false;
            return;
        }

        // predefine the space of the renderer
        space = vectorized_matrix<float>(sim->rules.get_space_width(), sim->rules.get_space_height());

        cout << "Local GUI started ..." << endl;

        running = true;

        while (running)
        {
            // Is there a new space in simulator queue? -> update renderer space then!

            if (sim->space->get_queue_size() != 0)
            {
                sim->space->queue_pop_to(space);
            }

            update(running);
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

        cout << "Local GUI quit." << endl;

        // End simulator when GUI exits
        sim->running = false;
    }

    /*
     * @brief Run the GUI with MPI
     */
    void run_mpi(ruleset rules)
    {
        //Setup MPI
        mpi_buffer_data = aligned_vector<float>(rules.get_space_width() * rules.get_space_height() * SPACE_QUEUE_MAX_SIZE);
        app_communication_status = APP_COMMUNICATION_RUNNING;

        //Initialize the GUI
        if (initialize())
        {

            // predefine the space of the renderer
            space = vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());

            cout << "MPI GUI started ..." << endl;

            bool running = true;

            while (running)
            {
                // Is there a new space in simulator queue? -> update renderer space then!

                /*if (sim->space->get_queue_size() != 0)
                {
                    sim->space->queue_pop_to(space);
                }*/

                update(running);
                render();
            }

            cout << "MPI GUI quit." << endl;
        }
        else
        {
            cerr << "GUI | Error while initialization!" << endl;
        }
        
        //MPI shutdown
        app_communication_status &= ~APP_COMMUNICATION_RUNNING; //disable "running"

        //Wait until any async communication is finished
        while(mpi_test(&mpi_status_communication)) 
        {
            cout << "GUI | Shutdown is waiting for open communication channel ..." << endl;
        }
        
        //Send the shutdown synchronous + blocking to prevent too early destruction of class
        MPI_Ssend(&app_communication_status,1,MPI_INT,mpi_get_rank_with_role(mpi_role::SIMULATOR_MASTER),APP_MPI_TAG_COMMUNICATION,MPI_COMM_WORLD);
    }

protected:

    vectorized_matrix<float> space;

    //MPI variables
    MPI_Request mpi_status_communication;
    MPI_Request mpi_status_data_prepare;
    MPI_Request mpi_status_data_data;
    aligned_vector<float> mpi_buffer_data;
    int mpi_data_recieve_size;
    int mpi_state_data;
    int app_communication_status;
    queue<vectorized_matrix<float>> mpi_render_queue; //Not needed for local as the GUI can use the queue provided by simulator

    /**
     * @brief do any initialization tasks
     * @return true if initialization was successful, otherwise false
     */
    virtual bool initialize() = 0;
    virtual void update(bool & running) = 0;
    virtual void render() = 0;

private:

    bool running = false;

#ifdef ENABLE_PERF_MEASUREMENT
    ulong frames_rendered = 0; // needed for performance measurement
#endif
};
