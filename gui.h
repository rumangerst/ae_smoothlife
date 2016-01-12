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
            // Try to pop queue to current space
            sim->space->pop(space);

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
        // predefine the space of the renderer
        space = vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());
    
        //Setup MPI
        MPI_Request mpi_status_communication = MPI_REQUEST_NULL;
        MPI_Request mpi_status_data_prepare = MPI_REQUEST_NULL;
        MPI_Request mpi_status_data_data = MPI_REQUEST_NULL;
        aligned_vector<float> mpi_buffer_data_data = aligned_vector<float>(rules.get_space_width() * rules.get_space_height() * SPACE_QUEUE_MAX_SIZE);
        int mpi_buffer_data_data_copy_status = 0;
        int mpi_buffer_data_prepare = 0;
        int mpi_state_data = APP_MPI_STATE_DATA_IDLE;
        int mpi_app_communication = APP_COMMUNICATION_RUNNING;
        unique_ptr<matrix_buffer_queue<float>> mpi_render_queue = unique_ptr<matrix_buffer_queue<float>>(new matrix_buffer_queue<float>(SPACE_QUEUE_MAX_SIZE, space));      
        
        //Slow down renderer if queue is not full
        const int renderer_queue_slowdown_strength = 16;
        int renderer_queue_slowdown_counter = 0;

        //Initialize the GUI
        if (initialize())
        {
            cout << "MPI GUI started ..." << endl;

            bool running = true;

            while (running)
            {
                // Slow down rendering of field if not enough items are in queue.
                if(renderer_queue_slowdown_counter++ >= renderer_queue_slowdown_strength * ( (mpi_render_queue->max_size() - mpi_render_queue->size()) / (float)mpi_render_queue->max_size() ))
                {                
                    //Update current space if needed
                    mpi_render_queue->pop(space);
                    
                    renderer_queue_slowdown_counter = 0;
                }

                update(running);
                render();

                //Handle MPI data communication if still supposed to run
                if (running && mpi_state_data == APP_MPI_STATE_DATA_IDLE)
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
                else if (mpi_state_data == APP_MPI_STATE_DATA_PREPARE && mpi_test(&mpi_status_data_prepare))
                {
                    cout << "< GUI will get " << mpi_buffer_data_prepare << " data." << endl;

                    if (mpi_buffer_data_prepare % (rules.get_space_width() * rules.get_space_height()) != 0)
                    {
                        cerr << "MPI Error | GUI recieved invalid data count." << endl;
                        exit(EXIT_FAILURE);
                    }

                    //cout << "s -- ircv-data" << endl;
                    MPI_Irecv(mpi_buffer_data_data.data(),
                              mpi_buffer_data_prepare,
                              MPI_FLOAT,
                              mpi_get_rank_with_role(mpi_role::SIMULATOR_MASTER),
                              APP_MPI_TAG_DATA_DATA,
                              MPI_COMM_WORLD,
                              &mpi_status_data_data);
                    //cout << "##s -- ircv-data" << endl;
                    mpi_state_data = APP_MPI_STATE_DATA_DATA;
                    
                    //cout << "bgt" << endl;
                    //cout << "test:" << mpi_test(&mpi_status_data_data) << endl;
                    //cout << "egt" << endl;
                }
                else if (mpi_state_data == APP_MPI_STATE_DATA_DATA && mpi_test(&mpi_status_data_data))
                {
                    //Count of items in current MPI buffer
                    int input_count = mpi_buffer_data_prepare / (rules.get_space_width() * rules.get_space_height());
                    
                    //As long as there is something to copy to queue
                    while(mpi_buffer_data_data_copy_status != input_count)
                    {
                        //Try to push the read buffer into queue. If this fails, wait.
                        if(!mpi_render_queue->push())
                        {
                            break;
                        }
                        
                        int i = mpi_buffer_data_data_copy_status; //The current item to copy
                        
                        float * src = &mpi_buffer_data_data.data()[i * rules.get_space_width() * rules.get_space_height()];                         
                        vectorized_matrix<float> * dst = mpi_render_queue->buffer_write_ptr();
                        
                        assert(dst->getNumCols() == rules.get_space_width() && dst->getNumRows() == rules.get_space_height());                        
                        
                        //Write the data into write buffer
                        for(int y = 0; y < dst->getNumRows(); ++y)
                        {
                            for(int x = 0; x < dst->getNumCols(); ++x)
                            {
                                dst->setValue(src[matrix_index(x,y,dst->getNumCols())],x,y);
                            }
                        }
                        
                        ++mpi_buffer_data_data_copy_status;
                    }
                    
                    //Only recieve new message if everything was copied.
                    if(mpi_buffer_data_data_copy_status == input_count)
                    {
                        mpi_buffer_data_data_copy_status = 0;
                        mpi_state_data = APP_MPI_STATE_DATA_IDLE;
                    }
                    
                    /*cout << "< GUI got data." << endl;

                    int input_count = mpi_buffer_data_prepare / (rules.get_space_width() * rules.get_space_height());

                    //Empty the queue if needed
                    for (int i = 0; i < input_count - mpi_render_queue->capacity_left() + 2; ++i)
                    {
                        mpi_render_queue->pop();
                    }
                    
                    cout << "Will put "<<input_count << " - queue_size = " << mpi_render_queue->size() << endl;

                    // Move data into queue
                    for(int i = 0; i < input_count; ++i)
                    {
                        float * src = &mpi_buffer_data_data.data()[i * rules.get_space_width() * rules.get_space_height()]; 
                        
                        vectorized_matrix<float> * dst = mpi_render_queue->buffer_write_ptr();
                        
                        assert(dst->getNumCols() == rules.get_space_width() && dst->getNumRows() == rules.get_space_height());
                        
                        
                        //Write the data into write buffer
                        for(int y = 0; y < dst->getNumRows(); ++y)
                        {
                            for(int x = 0; x < dst->getNumCols(); ++x)
                            {
                                dst->setValue(src[matrix_index(x,y,dst->getNumCols())],x,y);
                            }
                        }
                                                
                        //Push read buffer into the queue                        
                        while(!mpi_render_queue->push())
                        {
                            mpi_render_queue->pop();
                            //cerr << "MPI render queue could not make space for new data!" << endl;
                            //exit(EXIT_FAILURE);
                        }
                    }

                    mpi_state_data = APP_MPI_STATE_DATA_IDLE;*/
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
        while (!mpi_test(&mpi_status_communication))
        {
            cout << "GUI | Shutdown is waiting for open communication channel ..." << endl;
        }

        //Send the shutdown synchronous + blocking to prevent too early destruction of class
        MPI_Ssend(&mpi_app_communication, 1, MPI_INT, mpi_get_rank_with_role(mpi_role::SIMULATOR_MASTER), APP_MPI_TAG_COMMUNICATION, MPI_COMM_WORLD);
        
        //Cancel all other requests
        mpi_cancel_if_needed(&mpi_status_data_prepare);
        mpi_cancel_if_needed(&mpi_status_data_data);

        cout << "MPI GUI quit." << endl;
    }

protected:

    vectorized_matrix<float> space;

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
