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

    virtual ~gui() 
    { 
        if(mpi_render_queue != nullptr)
            delete mpi_render_queue;
    }

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
        mpi_buffer_data_data = aligned_vector<float>(rules.get_space_width() * rules.get_space_height() * SPACE_QUEUE_MAX_SIZE);
        mpi_app_communication = APP_COMMUNICATION_RUNNING;
        mpi_state_data = APP_MPI_STATE_DATA_IDLE;

        //Initialize the GUI
        if (initialize())
        {

            // predefine the space of the renderer
            space = vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());
            
            // Initialize the render queue
            mpi_render_queue = new matrix_buffer<float>(SPACE_QUEUE_MAX_SIZE, space);

            cout << "MPI GUI started ..." << endl;

            bool running = true;

            while (running || mpi_state_data != APP_MPI_STATE_DATA_IDLE)
            {          
                //Update current space if needed
                if(mpi_render_queue->get_queue_size() != 0)   
                {
                    mpi_render_queue->queue_pop_to(space);
                }
                
                update(running);
                render();            
                                
                //Handle MPI data communication if still supposed to run
                if(running && mpi_state_data == APP_MPI_STATE_DATA_IDLE)
                {
                    cout << "< GUI is idle. recv" << endl;
                    
                    MPI_Irecv(&mpi_buffer_data_prepare,
                              1,
                              MPI_INT,
                              mpi_get_rank_with_role(mpi_role::SIMULATOR_MASTER),
                              APP_MPI_TAG_DATA_PREPARE,
                              MPI_COMM_WORLD,
                              &mpi_status_data_prepare);
                    mpi_state_data = APP_MPI_STATE_DATA_PREPARE;
                }
                else if(mpi_state_data == APP_MPI_STATE_DATA_PREPARE && mpi_test(&mpi_status_data_prepare))
                {
                    cout << "< GUI got " << mpi_buffer_data_prepare << " data." << endl;
                    
                    /*MPI_Irecv(mpi_buffer_data_data.data(),
                              mpi_buffer_data_prepare * rules.get_space_width() * rules.get_space_height(),
                              MPI_FLOAT,
                              mpi_get_rank_with_role(mpi_role::SIMULATOR_MASTER),
                              APP_MPI_TAG_DATA_DATA,
                              MPI_COMM_WORLD,
                              &mpi_status_data_data);
                    mpi_state_data = APP_MPI_STATE_DATA_DATA;*/
                    
                    mpi_state_data = APP_MPI_STATE_DATA_IDLE;
                }
                else if(mpi_state_data == APP_MPI_STATE_DATA_DATA && mpi_test(&mpi_status_data_data))
                {
                    cout << "< GUI got data." << endl;
                    
                    /*//Empty the queue
                    for(int i = 0; i < mpi_buffer_data_prepare - mpi_render_queue->get_queue_capacity(); ++i)
                    {
                        mpi_render_queue->queue_pop();
                    }
                    
                    // Move data into queue
                    for(int i = 0; i < mpi_buffer_data_prepare; ++i)
                    {
                        float * src = &mpi_buffer_data_data.data()[i * mpi_buffer_data_prepare * rules.get_space_width() * rules.get_space_height()];
                        vectorized_matrix<float> * dst = mpi_render_queue->buffer_next();
                        
                        for(int y = 0; y < dst->getNumRows(); ++y)
                        {
                            for(int x = 0; x < dst->getNumCols(); ++x)
                            {
                                dst->setValue(src[matrix_index(x,y,dst->getNumCols())],x,y);
                            }
                        }
                    }*/
                    
                    mpi_state_data = APP_MPI_STATE_DATA_IDLE;
                }
                
            }            
        }
        else
        {
            cerr << "GUI | Error while initialization!" << endl;
        }
        
        cout << "MPI GUI sends shutdown ..." << endl;
        
        //MPI shutdown
        mpi_app_communication &= ~APP_COMMUNICATION_RUNNING; //disable "running"

        //Wait until any async communication is finished
        while(!mpi_test(&mpi_status_communication)) 
        {
            cout << "GUI | Shutdown is waiting for open communication channel ..." << endl;
        }
        
        //Send the shutdown synchronous + blocking to prevent too early destruction of class
        MPI_Ssend(&mpi_app_communication,1,MPI_INT,mpi_get_rank_with_role(mpi_role::SIMULATOR_MASTER),APP_MPI_TAG_COMMUNICATION,MPI_COMM_WORLD);
        
        cout << "MPI GUI quit." << endl;
    }

protected:

    vectorized_matrix<float> space;

    //MPI variables
    MPI_Request mpi_status_communication = MPI_REQUEST_NULL;
    MPI_Request mpi_status_data_prepare = MPI_REQUEST_NULL;
    MPI_Request mpi_status_data_data = MPI_REQUEST_NULL;
    aligned_vector<float> mpi_buffer_data_data;
    int mpi_buffer_data_prepare;
    int mpi_state_data = APP_MPI_STATE_DATA_IDLE;
    int mpi_app_communication;
    matrix_buffer<float> * mpi_render_queue = nullptr; //Not needed for local as the GUI can use the queue provided by simulator

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
