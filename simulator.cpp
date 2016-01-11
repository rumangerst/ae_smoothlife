#include "simulator.h"
#include "aligned_vector.h"
#include <assert.h>
#include <mpi.h>
#include <omp.h>

/*
 * DONE:
 * - simulation code correct (synchronized version)
 */

/*
 * TODO: Vectorization
 * 1. test it
 * 2. consider further optimizations
 * 
 * TODO: Parallelization
 */

simulator::simulator(const ruleset & r) : rules(r)
{
}

simulator::~simulator()
{
    if (space != nullptr)
    {
        delete space;
    }
}

void simulator::initialize(vectorized_matrix<float> & predefined_space)
{
    if (predefined_space.getNumRows() != rules.get_space_height() || predefined_space.getNumCols() != rules.get_space_width())
    {
        cerr << "Could not initialize simulator: Invalid predefined space size!" << endl;
        exit(-1);
    }

    cout << "Initializing ..." << endl;

    space = new matrix_buffer_queue<float>(SPACE_QUEUE_MAX_SIZE, predefined_space);

    //space_current = new vectorized_matrix<float>(predefined_space);
    //space_next = new vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());

    outer_masks.reserve(CACHELINE_FLOATS);
    inner_masks.reserve(CACHELINE_FLOATS);

    initiate_masks();
    /*
    outer_mask = vectorized_matrix<float>(rules.get_ri() * 2 + 2, rules.get_ri() * 2 + 2);
    inner_mask = vectorized_matrix<float>(rules.get_ra() * 2 + 2, rules.get_ra() * 2 + 2);

    outer_mask.set_circle(rules.get_ri(), 1, 1);
    inner_mask.set_circle(rules.get_ra(), 1, 1);
    inner_mask.set_circle(rules.get_ri(), 0, 1);
     */

    offset_from_mask_center = inner_masks[0].getLeftOffset();
    outer_mask_sum = outer_masks[0].sum(); // the sum remains the same for all masks, supposedly
    inner_mask_sum = inner_masks[0].sum();

    //TODO: test for sum over all matrices!   


    initialized = true;
}

void simulator::initiate_masks()
{
    // Initialize masks
    /* NOTE:
        - to allow vectorization, we add additional padding to the left side of the masks
        - this increases the effective leading dimension of the matrices
        - however, using the right offset, the space and the masks will always
          be aligned to the cacheline_size 64
        - at the cost of additional values to be calculated
     */
    for (int o = 0; o < CACHELINE_FLOATS; ++o)
    {
        vectorized_matrix<float> outer_mask = vectorized_matrix<float>(rules.get_ra() * 2 + 2, rules.get_ra() * 2 + 2, o);
        outer_mask.set_circle(rules.get_ri(), 1, 1, o);
        outer_masks.push_back(outer_mask);
        //outer_masks[o].print_to_console();
        //cout << endl;

        // change that back later to get_ri()
        vectorized_matrix<float> inner_mask = vectorized_matrix<float>(rules.get_ra() * 2 + 2, rules.get_ra() * 2 + 2, o);
        inner_mask.set_circle(rules.get_ra(), 1, 1, o);
        inner_mask.set_circle(rules.get_ri(), 0, 1, o);
        inner_masks.push_back(inner_mask);
        assert(inner_masks[0].getLd() == outer_masks[0].getLd());
        assert(inner_masks[0].getLeftOffset() == outer_masks[0].getLeftOffset());
        assert(inner_masks[0].getRightOffset() == outer_masks[0].getRightOffset());
        //inner_masks[o].print_info();
        //cout << endl;

        /*
        outer_masks.push_back(vectorized_matrix<float>(rules.get_ra() * 2 + 2, rules.get_ra() * 2 + 2));
        outer_masks[o].set_circle(rules.get_ri(), 1, 1, o);
        
        // change that back later to get_ri()
        inner_masks.push_back(vectorized_matrix<float>(rules.get_ra() * 2 + 2, rules.get_ra() * 2 + 2));
        inner_masks[o].set_circle(rules.get_ra(), 1, 1, o);
        inner_masks[o].set_circle(rules.get_ri(), 0, 1, o);
        assert(inner_masks[0].getLd() == outer_masks[0].getLd());
        assert(inner_masks[0].getLeftOffset() == outer_masks[0].getLeftOffset());
        assert(inner_masks[0].getRightOffset() == outer_masks[0].getRightOffset());
         */
    }
}

void simulator::initialize()
{
    cout << "Default initialization ..." << endl;

    vectorized_matrix<float> * space = new vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());

    if (SIMULATOR_MODE == MODE_SIMULATE || SIMULATOR_MODE == MODE_TEST_INITIALIZE)
    {
        SIMULATOR_INITIALIZATION_FUNCTION(space);
    }
    else if (SIMULATOR_MODE == MODE_TEST_MASKS)
    {
        initiate_masks();
        assert(!inner_masks.empty() && !outer_masks.empty());

        //Print the two masks
        for (int x = 0; x < inner_masks[0].getNumCols(); ++x)
        {
            for (int y = 0; y < inner_masks[0].getNumRows(); ++y)
            {
                space->setValue(inner_masks[0].getValue(x, y), x, y);
            }
        }

        for (int x = 0; x < outer_masks[0].getNumCols(); ++x)
        {
            for (int y = 0; y < outer_masks[0].getNumRows(); ++y)
            {
                space->setValue(outer_masks[0].getValue(x, y), x + inner_masks[0].getNumCols() + 10, y + inner_masks[0].getNumRows() + 10);
            }
        }
    }

    else if (SIMULATOR_MODE == MODE_TEST_COLORS)
    {
        //Test the colors
        for (int x = 0; x < rules.get_space_width(); ++x)
        {
            for (int y = 0; y < rules.get_space_height(); ++y)
            {
                float m = (double) y / rules.get_space_width();
                float n = (double) x / rules.get_space_height();

                space->setValue((n + m) / 2.0, x, y);
            }
        }
    }

    else if (SIMULATOR_MODE == MODE_TEST_STATE_FUNCTION)
    {
        //Test to print s(n,m)
        for (int x = 0; x < rules.get_space_width(); ++x)
        {
            for (int y = 0; y < rules.get_space_height(); ++y)
            {
                float m = (double) y / rules.get_space_width();
                float n = (double) x / rules.get_space_height();

                space->setValue(discrete_state_func_1(n, m), x, y);
            }
        }
    }

    // Give space to main initialization function
    initialize(*space);
}

void simulator::simulate_step()
{
#pragma omp parallel for schedule(static)
    for (int x = 0; x < rules.get_space_width(); ++x)
    {
        for (int y = 0; y < rules.get_space_height(); ++y)
        {
            // get the alignment offset caused during iteration of space
            cint off = ((x - offset_from_mask_center) >= 0) ?
                    (x - offset_from_mask_center) % CACHELINE_FLOATS :
                    CACHELINE_FLOATS - ((offset_from_mask_center - x)); // looks ugly, right? Only important for right borders
            assert(off >= 0 && off < CACHELINE_FLOATS);

            float n;
            float m;
            if (optimize && (x - offset_from_mask_center) >= 0 &&
                    (x + outer_masks[off].getRightOffset() < this->space_current->getNumCols()))
            {
                n = getFilling(x, y, inner_masks[off], inner_mask_sum); // filling of inner circle
                m = getFilling(x, y, outer_masks[off], outer_mask_sum); // filling of outer ring
            }
            else
            {
                n = getFilling_unoptimized(x, y, inner_masks[0], inner_mask_sum); // filling of inner circle
                m = getFilling_unoptimized(x, y, outer_masks[0], outer_mask_sum); // filling of outer ring
            }
            //Calculate the new state based on fillings n and m
            //Smooth state function must be clamped to [0,1] (this is also done by author's implementation!)
            space_next->setValue(rules.get_is_discrete() ? discrete_state_func_1(n, m) : fmax(0, fmin(1, next_step_as_euler(x, y, n, m))), x, y);

        }
    }

    ++spacetime;
}

void simulator::run_simulation_local()
{
    cout << "Simulator | Running local simulator ..." << endl;
    running = true;

#ifdef ENABLE_PERF_MEASUREMENT
    auto perf_time_start = chrono::high_resolution_clock::now();
    ulong perf_spacetime_start = 0;
#endif

    while (running)
    {
        if (SIMULATOR_MODE == MODE_SIMULATE)
        {
            /**
             * Simulate only the first time or when the buffer_queue can enqueue the current read buffer.
             */
            if (spacetime == 0 || space->push())
            {
                simulate_step();

                if (ENABLE_PERF_MEASUREMENT)
                {
                    // NOTE: this should be done either directly after swapping or here
                    // HACK: here is best - doesn't interrupt code-flow to much :)
                    if (spacetime % 100 == 0)
                    {
                        auto perf_time_end = chrono::high_resolution_clock::now();
                        double perf_time_seconds = chrono::duration<double>(perf_time_end - perf_time_start).count();

                        cout << "Simulator | " << (spacetime - perf_spacetime_start) / perf_time_seconds << " calculations / s" << endl;

                        perf_spacetime_start = spacetime;
                        perf_time_start = chrono::high_resolution_clock::now();
                    }
                }
            }
            else
            {
                cout << "Simulator | queue full!" << endl;
            }
        }
    }

    cout << "Simulator | Local simulator shut down." << endl;


}

void simulator::run_simulation_master()
{
    cout << "Simulator | Running MPI Master simulator ..." << endl;
    running = true;

#ifdef ENABLE_PERF_MEASUREMENT
    auto perf_time_start = chrono::high_resolution_clock::now();
    ulong perf_spacetime_start = 0;
#endif    

    MPI_Request mpi_status_communication = MPI_REQUEST_NULL;
    MPI_Request mpi_status_data_prepare = MPI_REQUEST_NULL;
    MPI_Request mpi_status_data_data = MPI_REQUEST_NULL;
    aligned_vector<float> mpi_buffer_data_data = aligned_vector<float>(rules.get_space_width() * rules.get_space_height() * SPACE_QUEUE_MAX_SIZE); //make SPACE_QUEUE_MAX_SIZE send buffer, so we can send multiple states per batch
    int mpi_buffer_data_prepare = 0;
    int mpi_state_data = APP_MPI_STATE_DATA_IDLE;
    int mpi_app_communication = APP_COMMUNICATION_RUNNING;

    while (running || mpi_state_data != APP_MPI_STATE_DATA_IDLE)
    {
        // Calculate the next state if simulation is enabled
        // Separate the actual simulation from interfacing            

        if (SIMULATOR_MODE == MODE_SIMULATE)
        {
            /**
             * Simulate only the first time or when the buffer_queue can enqueue the current read buffer.
             */
            if (spacetime == 0 || space->push())
            {
                simulate_step();

                if (ENABLE_PERF_MEASUREMENT)
                {
                    // NOTE: this should be done either directly after swapping or here
                    // HACK: here is best - doesn't interrupt code-flow to much :)
                    if (spacetime % 100 == 0)
                    {
                        auto perf_time_end = chrono::high_resolution_clock::now();
                        double perf_time_seconds = chrono::duration<double>(perf_time_end - perf_time_start).count();

                        cout << "Simulator | " << (spacetime - perf_spacetime_start) / perf_time_seconds << " calculations / s" << endl;

                        perf_spacetime_start = spacetime;
                        perf_time_start = chrono::high_resolution_clock::now();
                    }
                }
            }
            else
            {
                //cout << "Simulation || queue full!" << endl;
            }
        }

        //cout << "Simulation || state " << mpi_state_data << endl;


        //Communication tag. Get the data from GUI and 
        if (mpi_test(&mpi_status_communication))
        {
            cout << "MPI | Recieved COMMUNICATION signal." << endl;
            running = mpi_app_communication & APP_COMMUNICATION_RUNNING == APP_COMMUNICATION_RUNNING;

            // If the GUI has sent the final message (running=false), do not recieve any other message
            if (running)
                MPI_Irecv(&mpi_app_communication, 1, MPI_INT, mpi_get_rank_with_role(mpi_role::USER_INTERFACE), APP_MPI_TAG_COMMUNICATION, MPI_COMM_WORLD, &mpi_status_communication);
            else
                cout << "MPI | The simulator was requested to shut down." << endl;
        }

        // Request data send if still supposed to run
        if (running && mpi_state_data == APP_MPI_STATE_DATA_IDLE)
        {
            //Copy queue to buffer          
            int data_size = rules.get_space_width() * rules.get_space_height();
            int data_count = 0;

            while (space->size() != 0)
            {
                space->pop(&mpi_buffer_data_data.data()[data_count * data_size]);
                ++data_count;
            }

            if (data_count != 0)
            {
                mpi_buffer_data_prepare = data_size * data_count;
                //mpi_buffer_data_prepare = 1;
                
                cout << "> SIM pushed " << mpi_buffer_data_prepare << endl;

                //Send how many objects will be sent
                MPI_Isend(&mpi_buffer_data_prepare,
                          1,
                          MPI_INT,
                          mpi_get_rank_with_role(mpi_role::USER_INTERFACE),
                          APP_MPI_TAG_DATA_PREPARE,
                          MPI_COMM_WORLD,
                          &mpi_status_data_prepare);
                mpi_state_data = APP_MPI_STATE_DATA_PREPARE;
            }
        }
        else if (mpi_state_data == APP_MPI_STATE_DATA_PREPARE && mpi_test(&mpi_status_data_prepare))
        {
            int send_size = mpi_buffer_data_prepare;            
          
            cout << "> SIM sends data :" << send_size << endl;


            // Send the buffer
            MPI_Isend(mpi_buffer_data_data.data(),
                      send_size,
                      MPI_FLOAT,
                      mpi_get_rank_with_role(mpi_role::USER_INTERFACE),
                      APP_MPI_TAG_DATA_DATA,
                      MPI_COMM_WORLD,
                      &mpi_status_data_data);

            mpi_state_data = APP_MPI_STATE_DATA_DATA;
        }
        else if (mpi_state_data == APP_MPI_STATE_DATA_DATA && mpi_test(&mpi_status_data_data))
        {
            cout << "> SIM finished" << endl;

            mpi_state_data = APP_MPI_STATE_DATA_IDLE;
        }
    }
}

float simulator::getFilling(cint at_x, cint at_y, const vectorized_matrix<float> &mask, cfloat mask_sum)
{
    // These define the rect inside the grid being accessed by mask
    //cint XB = at_x - mask.getNumCols() / 2; // aka x_begin
    //cint XE = at_x + mask.getNumCols() / 2; // aka x_end

    cint XB = at_x - mask.getLeftOffset(); // aka x_begin
    cint XE = at_x + mask.getRightOffset(); // aka x_end
    cint YB = at_y - mask.getNumRows() / 2; // aka y_begin
    cint YE = at_y + mask.getNumRows() / 2; // aka y_end

    cint sim_ld = space_current->getLd();
    cint mask_ld = mask.getLd();
    const float* const __restrict__ sim_space = this->space_current->getValues();
    const float* const __restrict__ mask_space = mask.getValues();

    assert(long(sim_space) % ALIGNMENT == 0);
    assert(long(mask_space) % ALIGNMENT == 0);
    //cout << "at_x: " << at_x << "  at_y: " << at_y << "  XB: " << XB << "  XE: " << XE << endl;
    assert((XB * sizeof (float)) % ALIGNMENT == 0 && (XE * sizeof (float)) % ALIGNMENT == 0);

    //NOTE: if XB or YB is negative, we do not need to check XE or YE - they have to be within the field boundaries
    float f = 0;
    if (XB >= 0)
    {
        if (XE < rules.get_space_width())
        {
            // NOTE: x accessible without wrapping

            if (YB >= 0)
            {
                if (YE < rules.get_space_height())
                {
                    // ideal case. no wrapping
                    for (int y = YB; y < YE; ++y)
                    {
                        cint Y = y*sim_ld;
                        cint YB_ = (y - YB) * mask_ld - XB;
                        // only activate these if you really want to check again...
                        //assert((Y+XB) % CACHELINE_FLOATS == 0);
                        //assert((YB_*sizeof(floats)) % CACHELINE_FLOATS == 0);
                        //assert(long(&sim_space[XB+Y]) % ALIGNMENT == 0);
                        //assert(long(&mask_space[XB+YB_]) % ALIGNMENT == 0);
#pragma omp simd aligned(sim_space, mask_space:64)
#pragma vector aligned
                        for (int x = XB; x < XE; ++x)
                            f += sim_space[x + Y] * mask_space[x + YB_];
                    }
                }
                else
                {

                    // special case 2. Ideally vectorized.
                    for (int y = YB; y < rules.get_space_height(); ++y)
                    {
                        cint Y = y*sim_ld;
                        cint YB_ = (y - YB) * mask_ld - XB;
#pragma omp simd aligned(sim_space, mask_space:64)
#pragma vector aligned
                        for (int x = XB; x < XE; ++x)
                            f += sim_space[x + Y] * mask_space[x + YB_];
                    }

                    // optimized, wrapped access over the top border.
                    for (int y = 0; y < YE - rules.get_space_height(); ++y)
                    {
                        cint Y = y*sim_ld;
                        cint mask_y_off = mask.getNumRows() - (YE - rules.get_space_height());
                        cint YB_ = (mask_y_off + y) * mask_ld - XB;
                        //assert(long(&sim_space[XB+Y]) % ALIGNMENT == 0);
                        //assert(long(&mask_space[XB+YB_]) % ALIGNMENT == 0);
#pragma omp simd aligned(sim_space, mask_space:64)
#pragma vector aligned
                        for (int x = XB; x < XE; ++x)
                            f += sim_space[x + Y] * mask_space[x + YB_];
                    }
                }
            }
            else
            {
                // special case 1. Ideally vectorized.
                for (int y = 0; y < YE; ++y)
                {
                    cint Y = y*sim_ld;
                    cint YB_ = (y - YB) * mask_ld - XB;
#pragma omp simd aligned(sim_space, mask_space:64)
#pragma vector aligned
                    for (int x = XB; x < XE; ++x)
                        f += sim_space[x + Y] * mask_space[x + YB_];
                }

                // optimized, wrapped access over the bottom border.
                for (int y = rules.get_space_height() + YB; y < rules.get_space_height(); ++y)
                {
                    cint Y = y*sim_ld;
                    cint mask_y_off = rules.get_space_height() + YB;
                    cint YB_ = (y - mask_y_off) * mask_ld - XB;
                    //assert(long(&sim_space[XB+Y]) % ALIGNMENT == 0);
                    //assert(long(&mask_space[XB+YB_]) % ALIGNMENT == 0);
#pragma omp simd aligned(sim_space, mask_space:64)
#pragma vector aligned
                    for (int x = XB; x < XE; ++x)
                        f += sim_space[x + Y] * mask_space[x + YB_];
                }
            }
        }
        else if ((YB >= 0) && (YE < rules.get_space_height()))
        {
            // special case 4. Non-unit strides (probably unavoidable).
            for (int x = XB; x < rules.get_space_width(); ++x)
            {
                cint XB_ = x - XB;
                for (int y = YB; y < YE; ++y)
                    f += space_current->getValue(x, y) * mask.getValue(XB_, y - YB);
            }


            // optimized, wrapped access over the right border.
            for (int x = 0; x < (XE - rules.get_space_width()); ++x)
            {
                cint mask_x_off = mask.getNumCols() - (XE - rules.get_space_width());
                for (int y = YB; y < YE; ++y)
                    f += space_current->getValue(x, y) * mask.getValue(x + mask_x_off, y - YB);
            }
        }
        else
        {
            // hard case. use wrapped version.
            for (int y = YB; y < YE; ++y)
            {
                cint Y = y;
                cint YB_ = y - YB;
                for (int x = XB; x < XE; ++x)
                    f += space_current->getValueWrapped(x, Y) * mask.getValue(x - XB, YB_);
            }
        }
    }
    else if ((YB >= 0) && (YE < rules.get_space_height()))
    {
        // special case 3. May have non-unit stride.
        for (int x = 0; x < XE; ++x)
        {
            cint XB_ = x - XB;
            for (int y = YB; y < YE; ++y)
                f += space_current->getValue(x, y) * mask.getValue(XB_, y - YB);
        }

        // NOTE: XB is negative here.
        for (int x = rules.get_space_width() + XB; x < rules.get_space_width(); ++x)
        {
            cint XB_ = rules.get_space_width() + XB;
            for (int y = YB; y < YE; ++y)
                f += space_current->getValue(x, y) * mask.getValue(x - XB_, y - YB);
        }
    }
    else
    {
        // hard case. use wrapped version.
        for (int y = YB; y < YE; ++y)
        {
            cint Y = y;
            cint YB_ = y - YB;
            for (int x = XB; x < XE; ++x)
                f += space_current->getValueWrapped(x, Y) * mask.getValue(x - XB, YB_);
        }
    }

    return f / mask_sum; // normalize f
}

float simulator::getFilling_unoptimized(cint at_x, cint at_y, const vectorized_matrix<float> &mask, cfloat mask_sum)
{
    // The theorectically considered bondaries
    cint XB = at_x - mask.getNumCols() / 2; // aka x_begin ; Ld can be greater, than #cols!
    cint XE = at_x + mask.getNumCols() / 2; // aka x_end
    cint YB = at_y - mask.getNumRows() / 2; // aka y_begin
    cint YE = at_y + mask.getNumRows() / 2; // aka y_end

    float f = 0;

    if (XB >= 0 && YB >= 0 && XE < rules.get_space_width() && YE < rules.get_space_height())
    {
        for (int y = YB; y < YE; ++y)
        {
            cint Y = y;
            cint Y_BEG = y - YB;
            for (int x = XB; x < XE; ++x)
            {
                f += space_current->getValue(x, Y) * mask.getValue(x - XB, Y_BEG);
            }
        }
    }
    else
    {
        for (int y = YB; y < YE; ++y)
        {
            cint Y = y;
            cint Y_BEG = y - YB;
            for (int x = XB; x < XE; ++x)
            {
                f += space_current->getValueWrapped(x, Y) * mask.getValue(x - XB, Y_BEG);
            }
        }
    }

    return f / mask_sum;
}
