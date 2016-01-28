#include "simulator.h"
#include "aligned_vector.h"
#include "mpi_async_connection.h"
#include "mpi_dual_connection.h"
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include <math.h>

/*
 * DONE:
 * - vectorization
 * - simulation code correct (synchronized version)
 */
#define EPSILON 5.0e-5

/**
 * returns true, if |a-b| < deviation is true
 */
inline bool isApprox(cfloat a, cfloat b, cfloat deviation)
{
    return abs(a - b) < deviation;
}

/**
 * returns true, if |a-b| < EPSILON is true
 */
inline bool isApprox(cfloat a, cfloat b)
{
    return isApprox(a, b, EPSILON);
}

simulator::simulator(const ruleset & r) : m_rules(r)
{
}

simulator::~simulator()
{
    if (m_space != nullptr)
    {
        delete m_space;
    }
}

void simulator::initialize(aligned_matrix<float> & predefined_space)
{
    if (predefined_space.getNumRows() != m_rules.get_space_height() || predefined_space.getNumCols() != m_rules.get_space_width())
    {
        cerr << "Could not initialize simulator: Invalid predefined space size!" << endl;
        exit(-1);
    }

    cout << "Initializing ..." << endl;

    // Determine the queue size
    // If have a master simulator, we NEED a queue -> set it to SPACE_QUEUE_MAX_SIZE
    // A slave simulator or perftest-master simulator does not need a queue and it's overhead
    // Set it to 0 then, so we can use swap()
    int queue_size;

    if (!APP_UNIT_TEST && !APP_PERFTEST && mpi_get_role() == mpi_role::SIMULATOR_MASTER)
        queue_size = SPACE_QUEUE_MAX_SIZE;
    else
        queue_size = 0;

    m_space = new matrix_buffer_queue<float>(queue_size, predefined_space);

    //space_current = new vectorized_matrix<float>(predefined_space);
    //space_next = new vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());

    m_outer_masks.reserve(CACHELINE_FLOATS);
    m_inner_masks.reserve(CACHELINE_FLOATS);

    initiate_masks();

    m_offset_from_mask_center = m_inner_masks[0].getLeftOffset();
    m_outer_mask_sum = m_outer_masks[0].sum(); // the sum remains the same for all masks, supposedly
    m_inner_mask_sum = m_inner_masks[0].sum();

    m_initialized = true;
}

void simulator::initiate_masks()
{
    // Initialize masks
    /* NOTE:
        - to allow vectorization, we add additional padding to the left side of the masks
        - this increases the effective leading dimension of the matrices
        - however, using the right offset, the space and the masks will always
          be aligned to the cacheline_size (64)
        - we may still accept peel loops. this will be determined during advanced
          performance testing
     */
    for (int o = 0; o < CACHELINE_FLOATS; ++o)
    {
        aligned_matrix<float> inner_mask = aligned_matrix<float>(m_rules.get_radius_outer() * 2 + 2, m_rules.get_radius_outer() * 2 + 2, o);
        inner_mask.set_circle(m_rules.get_radius_inner(), 1, 1, o);
        m_inner_masks.push_back(inner_mask);

        aligned_matrix<float> outer_mask = aligned_matrix<float>(m_rules.get_radius_outer() * 2 + 2, m_rules.get_radius_outer() * 2 + 2, o);
        outer_mask.set_circle(m_rules.get_radius_outer(), 1, 1, o);
        outer_mask.set_circle(m_rules.get_radius_inner(), 0, 1, o);
        m_outer_masks.push_back(outer_mask);
        assert(m_inner_masks[0].getLd() == m_outer_masks[0].getLd());
        assert(m_inner_masks[0].getLeftOffset() == m_outer_masks[0].getLeftOffset());
        assert(m_inner_masks[0].getRightOffset() == m_outer_masks[0].getRightOffset());
    }
}

void simulator::initialize()
{
    cout << "Default initialization ..." << endl;

    aligned_matrix<float> * space = new aligned_matrix<float>(m_rules.get_space_width(), m_rules.get_space_height());

    SIMULATOR_INITIALIZATION_FUNCTION(space);

    // Give space to main initialization function
    initialize(*space);
}

void simulator::simulate_step()
{
    simulate_step(0, m_rules.get_space_width());
}

void simulator::simulate_step(int x_start, int w)
{
    #pragma omp parallel for schedule(static)
    for (int x = x_start; x < x_start + w; ++x)
    {
        // get the alignment offset caused during iteration of space
        // NOTE: this is mostly cache optimized. Each mask is used over an entire y array
        int off = ((x - m_offset_from_mask_center) >= 0) ?
                (x - m_offset_from_mask_center) % CACHELINE_FLOATS :
                CACHELINE_FLOATS - ((m_offset_from_mask_center - x) % CACHELINE_FLOATS); // we calc this new inside the function in this case
        const aligned_matrix<float> const &mask_inner = m_inner_masks[off];
        const aligned_matrix<float> const &mask_outer = m_outer_masks[off];
        for (int y = 0; y < m_rules.get_space_height(); ++y)
        {
            float n;
            float m;
            
            assert(off >= 0 && off < CACHELINE_FLOATS);
            if (m_optimize)
            {
                m = getFilling(x, y, mask_inner, m_inner_mask_sum); // filling of inner circle
                n = getFilling(x, y, mask_outer, m_outer_mask_sum); // filling of outer ring
            }
            else
            {
                m = getFilling_unoptimized(x, y, m_inner_masks[0], m_inner_mask_sum); // filling of inner circle
                n = getFilling_unoptimized(x, y, m_outer_masks[0], m_outer_mask_sum); // filling of outer ring
            }

            //Calculate the new state based on fillings n and m
            //Smooth state function must be clamped to [0,1] (this is also done by author's implementation!)
            space_next->setValue(m_rules.get_is_discrete() ? discrete_state_func_1(n, m) : fmax(0, fmin(1, next_step_as_euler(x, y, n, m))), x, y);
        }
    }

    //cerr << "Disabled calc" << endl;
    ++spacetime;
}

void simulator::run_simulation_slave()
{
    cout << "Simulator | Slave simulator on rank " << mpi_rank() << endl;

    // Connection from master to slave (communication)

    mpi_dual_connection<int> communication_connection = mpi_dual_connection<int>(
            0,
            false,
            true,
            APP_MPI_TAG_COMMUNICATION,
            MPI_INT,
            aligned_vector<int>{
        APP_COMMUNICATION_RUNNING
    });

    //Connection from slave to master (data)
    mpi_dual_connection<float> space_connection = mpi_dual_connection<float>(
            0,
            true,
            false,
            APP_MPI_TAG_SPACE,
            m_rules.get_space_height() * get_mpi_chunk_width(),
            MPI_FLOAT);

    // The slave has connections to the left and right rank
    int left_rank = matrix_index_wrapped(mpi_rank() - 1, 1, mpi_comm_size(), 1, mpi_comm_size());
    int right_rank = matrix_index_wrapped(mpi_rank() + 1, 1, mpi_comm_size(), 1, mpi_comm_size());

    // We use these border ids as tags for border synchronization. Each border gets it's ID, so no confusion happens
    int border_left_tag = matrix_index_wrapped(get_mpi_chunk_index(), 1, mpi_comm_size(), 1, mpi_comm_size()) + APP_MPI_TAG_BORDER_RANGE;
    int border_right_tag = matrix_index_wrapped(get_mpi_chunk_index() + 1, 1, mpi_comm_size(), 1, mpi_comm_size()) + APP_MPI_TAG_BORDER_RANGE;

    mpi_dual_connection<float> border_left_connection = mpi_dual_connection<float>(
            left_rank,
            left_rank != 0,
            true,
            border_left_tag,
            m_rules.get_space_height() * get_mpi_chunk_border_width(),
            MPI_FLOAT);
    mpi_dual_connection<float> border_right_connection = mpi_dual_connection<float>(
            right_rank,
            right_rank != 0,
            true,
            border_right_tag,
            m_rules.get_space_height() * get_mpi_chunk_border_width(),
            MPI_FLOAT);

    
    // Use broadcast to obtain the initial space from master
    cout << "Slave " << mpi_rank() << " obtains space from Master ..." << endl;
    vector<float> buffer_space = vector<float>(m_rules.get_space_width() * m_rules.get_space_height());
    MPI_Bcast(buffer_space.data(), m_rules.get_space_width() * m_rules.get_space_height(), MPI_FLOAT, 0, MPI_COMM_WORLD);

    space_current->raw_overwrite(buffer_space.data(),
                                 get_mpi_chunk_index() * get_mpi_chunk_width(),
                                 get_mpi_chunk_border_width(),
                                 get_mpi_chunk_width(),
                                 m_rules.get_space_width()); //Overwrite main space
    cout << "Slave " << mpi_rank() << " obtains space from Master ... done" << endl;                                 
    //MPI_Barrier(MPI_COMM_WORLD);   

    /*space_current->raw_overwrite(buffer_space.data(),
                                 get_mpi_chunk_index(left_rank) * get_mpi_chunk_width() + get_mpi_chunk_width() - get_mpi_chunk_border_width(),
                                 0,
                                 get_mpi_chunk_border_width(),
                                 rules.get_space_width()); //Overwrite left border with right border of left rank

    space_current->raw_overwrite(buffer_space.data(),
                                 get_mpi_chunk_index(right_rank) * get_mpi_chunk_width(),
                                 get_mpi_chunk_border_width() + get_mpi_chunk_width(),
                                 get_mpi_chunk_border_width(),
                                 rules.get_space_width()); //Overwrite right border with left border of right rank*/


    m_running = true;

    while (m_running)
    {
        if (m_reinitialize)
        {
            cout << "Slave " << mpi_rank() << " | Reinitialize ..." << endl;
            
            MPI_Bcast(buffer_space.data(), m_rules.get_space_width() * m_rules.get_space_height(), MPI_FLOAT, 0, MPI_COMM_WORLD);

            space_current->raw_overwrite(buffer_space.data(),
                                         get_mpi_chunk_index() * get_mpi_chunk_width(),
                                         get_mpi_chunk_border_width(),
                                         get_mpi_chunk_width(),
                                         m_rules.get_space_width()); //Overwrite main space
            this->m_reinitialize = false;
            //MPI_Barrier(MPI_COMM_WORLD);   
        }

        if (left_rank != 0)
        {
            space_current->raw_copy_to(border_left_connection.get_buffer_send()->data(), get_mpi_chunk_border_width(), get_mpi_chunk_border_width());
            border_left_connection.sendrecv();
        }
        else
        {
            border_left_connection.recv();
        }

        if (right_rank != 0)
        {
            space_current->raw_copy_to(border_right_connection.get_buffer_send()->data(), get_mpi_chunk_width(), get_mpi_chunk_border_width());
            border_right_connection.sendrecv();
        }
        else
        {
            border_right_connection.recv();
        }


        space_current->raw_overwrite(border_left_connection.get_buffer_recieve()->data(), 0, get_mpi_chunk_border_width());
        space_current->raw_overwrite(border_right_connection.get_buffer_recieve()->data(), get_mpi_chunk_border_width() + get_mpi_chunk_width(), get_mpi_chunk_border_width());

        /**
         * The slave simulator only has to simulate one chunk. So we call simulate_step with this size.
         * The simulator obtains its left and right borders from the neighbors via MPI. The data is stored in the border area left and right
         * to the chunk area. This border area has a size % CACHELINE_SIZE
         */
        simulate_step(get_mpi_chunk_border_width(), get_mpi_chunk_width());

        //Copy the complete field into the space buffer and the borders into their respective buffers
        space_next->raw_copy_to(space_connection.get_buffer_send()->data(), get_mpi_chunk_border_width(), get_mpi_chunk_width());
        space_connection.send();

        m_space->swap(); //The queue is disabled, use swap which yields greater performance

        //Update communication signal
        communication_connection.recv();       
       
        
        m_running = (communication_connection.get_buffer_recieve()->data()[0] & APP_COMMUNICATION_RUNNING) == APP_COMMUNICATION_RUNNING;
        m_reinitialize = (communication_connection.get_buffer_recieve()->data()[0] & APP_COMMUNICATION_REINITIALIZE) == APP_COMMUNICATION_REINITIALIZE;
    }

    cout << "Simulator | Slave shut down." << endl;
}

void simulator::run_simulation_master()
{
    cout << "Simulator | Running Master MPI simulator ..." << endl;
    m_running = true;

#ifdef ENABLE_PERF_MEASUREMENT
    auto perf_time_start = chrono::high_resolution_clock::now();
    ulong perf_spacetime_start = 0;
#endif    

    vector<mpi_dual_connection<int>> communication_connections;
    vector<mpi_dual_connection<float>> space_connections;

    for (int i = 1; i < mpi_comm_size(); ++i)
    {
        // Connection from master to slave (communication)
        communication_connections.push_back(mpi_dual_connection<int>(
                                            i,
                                            true,
                                            false,
                                            APP_MPI_TAG_COMMUNICATION,
                                            MPI_INT,
                                            aligned_vector<int>{APP_COMMUNICATION_RUNNING}));
    }

    for (int i = 1; i < mpi_comm_size(); ++i)
    {
        // Connection from slave to master (calculated spaces)
        space_connections.push_back(mpi_dual_connection<float>(
                                    i,
                                    false,
                                    true,
                                    APP_MPI_TAG_SPACE,
                                    m_rules.get_space_height() * get_mpi_chunk_width(),
                                    MPI_FLOAT));
    }

    // The master has connections to the left and right rank to synchronize borders
    int left_rank = matrix_index_wrapped(mpi_rank() - 1, 1, mpi_comm_size(), 1, mpi_comm_size());
    int right_rank = matrix_index_wrapped(mpi_rank() + 1, 1, mpi_comm_size(), 1, mpi_comm_size());

    // We use these border ids as tags for border synchronization. Each border gets it's ID, so no confusion happens
    int border_left_tag = matrix_index_wrapped(get_mpi_chunk_index(), 1, mpi_comm_size(), 1, mpi_comm_size()) + APP_MPI_TAG_BORDER_RANGE;
    int border_right_tag = matrix_index_wrapped(get_mpi_chunk_index() + 1, 1, mpi_comm_size(), 1, mpi_comm_size()) + APP_MPI_TAG_BORDER_RANGE;

    mpi_dual_connection<float> border_left_connection = mpi_dual_connection<float>(
            left_rank,
            true,
            false,
            border_left_tag,
            m_rules.get_space_height() * get_mpi_chunk_border_width(),
            MPI_FLOAT);
    mpi_dual_connection<float> border_right_connection = mpi_dual_connection<float>(
            right_rank,
            true,
            false,
            border_right_tag,
            m_rules.get_space_height() * get_mpi_chunk_border_width(),
            MPI_FLOAT);

    //Send the initial field to all slaves   
    vector<float> buffer_space = vector<float>(m_rules.get_space_width() * m_rules.get_space_height());
    m_space->buffer_read_ptr()->raw_copy_to(buffer_space.data());
    MPI_Bcast(buffer_space.data(), m_rules.get_space_width() * m_rules.get_space_height(), MPI_FLOAT, 0, MPI_COMM_WORLD);

    //MPI_Barrier(MPI_COMM_WORLD);

    while (m_running)
    {
        if (m_reinitialize)
        {
            cout << "Master | Reinitialize .." << endl;
            
            SIMULATOR_INITIALIZATION_FUNCTION(space_current);
            m_reinitialize = false;

            //Resend the field if reinitialization was triggered
            vector<float> buffer_space = vector<float>(m_rules.get_space_width() * m_rules.get_space_height());
            m_space->buffer_read_ptr()->raw_copy_to(buffer_space.data());
            MPI_Bcast(buffer_space.data(), m_rules.get_space_width() * m_rules.get_space_height(), MPI_FLOAT, 0, MPI_COMM_WORLD);

            //MPI_Barrier(MPI_COMM_WORLD);
        }

        if (right_rank != 0)
        {
            /**
             * We want the right border. It starts at (chunk_index + 1) * chunk_width - chunk_border_width
             */
            int chunk_index = get_mpi_chunk_index();
            int border_start = (chunk_index + 1) * get_mpi_chunk_width() - get_mpi_chunk_border_width();
            space_current->raw_copy_to(border_right_connection.get_buffer_send()->data(), border_start, get_mpi_chunk_border_width());

            border_right_connection.send();
        }
        if (left_rank != 0)
        {
            /**
             * We want the left border. It starts at chunk_index * chunk_width
             */
            int chunk_index = get_mpi_chunk_index();
            int border_start = chunk_index * get_mpi_chunk_width();
            space_current->raw_copy_to(border_left_connection.get_buffer_send()->data(), border_start, get_mpi_chunk_border_width());

            border_left_connection.send();
        }

        /**
         * The master simulator is special in comparison to the slaves. As the master holds the complete field, it can 
         * access all data without copying/synching.
         * We want the master to have a workload, too; so we give it the first data chunk (the width of the field divided by count of ranks)
         * 
         * If we only have one rank, the master will calculate all of them
         */
        simulate_step(get_mpi_chunk_index() * get_mpi_chunk_width(), get_mpi_chunk_width());

        for (mpi_dual_connection<float> & conn : space_connections)
        {
            conn.recv();

            int chunk_index = get_mpi_chunk_index(conn.get_other_rank());
            space_next->raw_overwrite(conn.get_buffer_recieve()->data(), chunk_index * get_mpi_chunk_width(), get_mpi_chunk_width());
        }

        if (ENABLE_PERF_MEASUREMENT)
        {
            if (spacetime % 100 == 0)
            {
                auto perf_time_end = chrono::high_resolution_clock::now();
                double perf_time_seconds = chrono::duration<double>(perf_time_end - perf_time_start).count();

                cout << "Simulator | " << (spacetime - perf_spacetime_start) / perf_time_seconds << " calculations / s" << endl;

                perf_spacetime_start = spacetime;
                perf_time_start = chrono::high_resolution_clock::now();
            }
        }

        //Try to push into queue
        if (!APP_PERFTEST)
        {
            while (m_running && !m_space->push())
            {
            }
        }
        else
        {
            m_space->swap();
        }

        //Send status signal
        int communication_status = 0;

        if (m_running)
            communication_status |= APP_COMMUNICATION_RUNNING;
        if (m_reinitialize)
            communication_status |= APP_COMMUNICATION_REINITIALIZE;

        for (mpi_dual_connection<int> & conn : communication_connections)
        {
            conn.get_buffer_send()->data()[0] = communication_status;
            conn.send();
        }
    }
}

float simulator::getFilling(cint at_x, cint at_y, const aligned_matrix<float> &mask, cfloat mask_sum)
{
    // These define the rect inside the grid being accessed by mask
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

    //NOTE: if XB or YB is negative, we do not need to check XE or YE - they have to be within the field boundaries
    float f = 0;
    if (XB >= 0)
    {
        if (XE < m_rules.get_space_width())
        {
            // NOTE: x accessible without wrapping
            if (YB >= 0)
            {
                if (YE < m_rules.get_space_height())
                {
                    assert((XB * sizeof (float)) % ALIGNMENT == 0 && (XE * sizeof (float)) % ALIGNMENT == 0);
                    // Ideal case. Access within the space without crossing edges. Tested.
                    for (int y = YB; y < YE; ++y)
                    {
                        cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                        cfloat const * m_row = mask_space + (y - YB) * mask_ld; // mask row
                        assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                        #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                        for (int x = 0; x < XE - XB; ++x)
                            f += s_row[x] * m_row[x];
                    }
                }
                else
                {
                    assert((XB * sizeof (float)) % ALIGNMENT == 0 && (XE * sizeof (float)) % ALIGNMENT == 0);
                    // both loops have the same offset & mask!
                    // special case 2. Ideally vectorized. Access over bottom border. Tested
                    for (int y = YB; y < m_rules.get_space_height(); ++y)
                    {
                        cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                        cfloat const * m_row = mask_space + (y - YB) * mask_ld; // mask row
                        assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                        #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                        for (int x = 0; x < XE - XB; ++x)
                            f += s_row[x] * m_row[x];
                    }


                    cint mask_y_off = m_rules.get_space_height() - YB; // row offset caused by prior loop
                    for (int y = 0; y < YE - m_rules.get_space_height(); ++y)
                    {
                        cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                        cfloat const * m_row = mask_space + (y + mask_y_off) * mask_ld; // mask row
                        assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                        #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                        for (int x = 0; x < XE - XB; ++x)
                            f += s_row[x] * m_row[x];
                    }
                }
            }
            else
            {
                assert((XB * sizeof (float)) % ALIGNMENT == 0 && (XE * sizeof (float)) % ALIGNMENT == 0);
                // both loops have the same offset & mask!
                // special case 1. Ideally vectorized. Access over top border. Tested
                cint mask_y_off = -YB;
                for (int y = 0; y < YE; ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                    cfloat const * m_row = mask_space + (y + mask_y_off) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < XE - XB; ++x)
                        f += s_row[x] * m_row[x];
                }


                cint mask_y_off2 = m_rules.get_space_height() + YB;
                for (int y = m_rules.get_space_height() + YB; y < m_rules.get_space_height(); ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                    cfloat const * m_row = mask_space + (y - mask_y_off2) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < XE - XB; ++x)
                        f += s_row[x] * m_row[x];
                }
            }
        }
        else if ((YB >= 0) && (YE < m_rules.get_space_height()))
        {
            // special case 4. Access of right border. Tested
            for (int y = YB; y < YE; ++y)
            {
                cfloat const * s_row = sim_space + XB + y*sim_ld;
                cfloat const * m_row = mask_space + (y - YB) * mask_ld; // mask row
                assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                for (int x = 0; x < m_rules.get_space_width() - XB; ++x)
                    f += s_row[x] * m_row[x];
            }


            cint mask_x_off = m_rules.get_space_width() - XB;
            for (int y = YB; y < YE; ++y)
            {
                cfloat const * s_row = sim_space + y*sim_ld;
                cfloat const * m_row = mask_space + mask_x_off + (y - YB) * mask_ld; // mask row
                assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                for (int x = 0; x < XE - m_rules.get_space_width(); ++x)
                    f += s_row[x] * m_row[x];
            }
        }
        else
        {
            // XE >= FW
            if (YB < 0) {
                // hard case. top right corner
                
                // wrap to bottom right
                cint mask_y_off2 = m_rules.get_space_height() + YB;
                for (int y = m_rules.get_space_height() + YB; y < m_rules.get_space_height(); ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                    cfloat const * m_row = mask_space + (y-mask_y_off2) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < m_rules.get_space_width() - XB; ++x)
                        f += s_row[x] * m_row[x];
                }
                
                // wrap to top right
                cint mask_y_off = -YB;
                for (int y = 0; y < YE; ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                    cfloat const * m_row = mask_space + (y + mask_y_off) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < m_rules.get_space_width() - XB; ++x)
                        f += s_row[x] * m_row[x];
                }
                
                // wrap to bottom left
                cint mask_x_off = m_rules.get_space_width() - XB;
                for (int y = m_rules.get_space_height() + YB; y < m_rules.get_space_height(); ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld; // space row + x_start
                    cfloat const * m_row = mask_space + mask_x_off + (y-mask_y_off2) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < XE - m_rules.get_space_width(); ++x)
                        f += s_row[x] * m_row[x];
                }
                
                // wrap to top left
                for (int y = 0; y < YE; ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld; // space row + x_start
                    cfloat const * m_row = mask_space + mask_x_off + (y + mask_y_off) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < XE - m_rules.get_space_width(); ++x)
                        f += s_row[x] * m_row[x];
                }
            } else {
                // YE >= FW
                assert(YE >= m_rules.get_space_height());
                // hard case. bottom right
                cint mask_y_off = YB;
                for (int y = YB; y < m_rules.get_space_height(); ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                    cfloat const * m_row = mask_space + (y - mask_y_off) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < m_rules.get_space_width() - XB; ++x)
                        f += s_row[x] * m_row[x];
                }
                
                cint mask_y_off2 = m_rules.get_space_height() - YB;
                for (int y = 0; y < YE - m_rules.get_space_height(); ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld + XB; // space row + x_start
                    cfloat const * m_row = mask_space + (y + mask_y_off2) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < m_rules.get_space_width() - XB; ++x)
                        f += s_row[x] * m_row[x];
                }
                
                cint mask_x_off = m_rules.get_space_width() - XB;
                for (int y = YB; y < m_rules.get_space_height(); ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld; // space row + x_start
                    cfloat const * m_row = mask_space + mask_x_off + (y - mask_y_off) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < XE - m_rules.get_space_width(); ++x)
                        f += s_row[x] * m_row[x];
                }
                
                for (int y = 0; y < YE - m_rules.get_space_height(); ++y)
                {
                    cfloat const * s_row = sim_space + y * sim_ld; // space row + x_start
                    cfloat const * m_row = mask_space + mask_x_off + (y + mask_y_off2) * mask_ld; // mask row
                    assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                    #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                    for (int x = 0; x < XE - m_rules.get_space_width(); ++x)
                        f += s_row[x] * m_row[x];
                }
            }
        }
    }
    else if ((YB >= 0) && (YE < m_rules.get_space_height()))
    {
        // special case 3. Access over left border. Tested
        cint mask_x_off = -XB;
        for (int y = YB; y < YE; ++y)
        {
            cfloat const * s_row = sim_space + y*sim_ld;
            cfloat const * m_row = mask_space + mask_x_off + (y - YB) * mask_ld; // mask row
            assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
            #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
            for (int x = 0; x < XE; ++x)
                f += s_row[x] * m_row[x];
            //f += sim_space[x + y*sim_ld] * mask_space[x + mask_x_off + (y - YB) * mask_ld];
        }

        // wrapped part. we are on the right now
        for (int y = YB; y < YE; ++y)
        {
            cfloat const * s_row = sim_space + y * sim_ld + m_rules.get_space_width() + XB;
            cfloat const * m_row = mask_space + (y - YB) * mask_ld; // mask row
            assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
            #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
            for (int x = 0; x < -XB; ++x)
                //f += sim_space[x + y*sim_ld + m_rules.get_space_width() + XB] * mask_space[x + (y - YB) * mask_ld];
                f += s_row[x] * m_row[x];
        }
    }
    else
    {
        // XB < 0
        if (YB < 0) {
            // hard case. Top left corner
            cint mask_y_off = m_rules.get_space_height() + YB;
            for (int y = m_rules.get_space_height() + YB; y < m_rules.get_space_height(); ++y)
            {
                cfloat const * s_row = sim_space + y * sim_ld + m_rules.get_space_width() + XB; // space row + x_start
                cfloat const * m_row = mask_space + (y - mask_y_off) * mask_ld; // mask row
                assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                for (int x = 0; x < -XB; ++x)
                    f += s_row[x] * m_row[x];
            }

            cint mask_y_off2 = -YB;
            for (int y = 0; y < YE; ++y)
            {
                cfloat const * s_row = sim_space + y * sim_ld + m_rules.get_space_width() + XB; // space row + x_start
                cfloat const * m_row = mask_space + (y + mask_y_off2) * mask_ld; // mask row
                assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                for (int x = 0; x < -XB; ++x)
                    f += s_row[x] * m_row[x];
            }
            
            cint mask_x_off = -XB;
            for (int y = m_rules.get_space_height() + YB; y < m_rules.get_space_height(); ++y)
            {
                cfloat const * s_row = sim_space + y * sim_ld; // space row + x_start
                cfloat const * m_row = mask_space + mask_x_off + (y - mask_y_off) * mask_ld; // mask row
                assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                for (int x = 0; x < XE; ++x)
                    f += s_row[x] * m_row[x];
            }

            for (int y = 0; y < YE; ++y)
            {
                cfloat const * s_row = sim_space + y * sim_ld; // space row + x_start
                cfloat const * m_row = mask_space + mask_x_off + (y + mask_y_off2) * mask_ld; // mask row
                assert(!(long(s_row) % ALIGNMENT || long(m_row) % ALIGNMENT));
                #pragma omp simd aligned(s_row, m_row:64) reduction(+:f)
                for (int x = 0; x < XE; ++x)
                    f += s_row[x] * m_row[x];
            }
        } else {
            // YE >= FW
            assert(YE >= m_rules.get_space_height());
            // hard case. use wrapped version.
            for (int y = YB; y < YE; ++y)
            {
                cint Y = y;
                cint YB_ = (y - YB) * mask_ld - XB;
                for (int x = XB; x < XE; ++x)
                    f += space_current->getValueWrapped(x, y) * mask_space[x + YB_];
            }
        }
    }

    return f / mask_sum; // normalize f
}

float simulator::getFilling_peeled(cint at_x, cint at_y, const vector<aligned_matrix<float>> &masks, cint offset, cfloat mask_sum)
{
    assert(offset >= 0);
    aligned_matrix<float> const &mask = masks[offset];

    // These define the rect inside the grid being accessed by mask
    cint XB = at_x - floor(mask.getNumCols() / 2); // aka x_begin
    cint XE = at_x + ceil(mask.getNumCols() / 2); // aka x_end
    cint YB = at_y - floor(mask.getNumRows() / 2); // aka y_begin
    cint YE = at_y + ceil(mask.getNumRows() / 2); // aka y_end

    cint sim_ld = space_current->getLd();
    cint mask_ld = mask.getLd();
    const float* const __restrict__ sim_space = this->space_current->getValues();
    const float* const __restrict__ mask_space = mask.getValues();

    assert(long(sim_space) % ALIGNMENT == 0);
    assert(long(mask_space) % ALIGNMENT == 0);

    //NOTE: if XB or YB is negative, we do not need to check XE or YE - they have to be within the field boundaries
    float f = 0;
    if (XB >= 0)
    {
        if (XE < m_rules.get_space_width())
        {
            // NOTE: x accessible without wrapping
            if (YB >= 0)
            {
                if (YE < m_rules.get_space_height())
                {
                    // ideal case. no wrapping
                    for (int y = YB; y < YE; ++y)
                    {
                        cint Y = y*sim_ld;
                        cint YB_ = offset + (y - YB) * mask_ld - XB;
                        __assume_aligned(sim_space, 64);
                        __assume_aligned(mask_space, 64);
                        for (int x = XB; x < XE; ++x)
                            f += sim_space[x + Y] * mask_space[x + YB_];
                    }
                }
                else
                {

                    // special case 2. Ideally vectorized. Access over bottom border
                    for (int y = YB; y < m_rules.get_space_height(); ++y)
                    {
                        cint Y = y*sim_ld;
                        cint YB_ = offset + (y - YB) * mask_ld - XB;
                        __assume_aligned(sim_space, 64);
                        __assume_aligned(mask_space, 64);
                        for (int x = XB; x < XE; ++x)
                            f += sim_space[x + Y] * mask_space[x + YB_];
                    }

                    // optimized, wrapped access over the top border.
                    for (int y = 0; y < YE - m_rules.get_space_height(); ++y)
                    {
                        cint Y = y*sim_ld;
                        cint mask_y_off = mask.getNumRows() - (YE - m_rules.get_space_height());
                        cint YB_ = offset + (mask_y_off + y) * mask_ld - XB;
                        __assume_aligned(sim_space, 64);
                        __assume_aligned(mask_space, 64);
                        for (int x = XB; x < XE; ++x)
                            f += sim_space[x + Y] * mask_space[x + YB_];
                    }
                }
            }
            else
            {
                // special case 1. Ideally vectorized. Access over top border
                for (int y = 0; y < YE; ++y)
                {
                    cint Y = y*sim_ld;
                    cint YB_ = offset + (y - YB) * mask_ld - XB;
                    __assume_aligned(sim_space, 64);
                    __assume_aligned(mask_space, 64);
                    for (int x = XB; x < XE; ++x)
                        f += sim_space[x + Y] * mask_space[x + YB_];
                }

                // optimized, wrapped access over the bottom border.
                for (int y = m_rules.get_space_height() + YB; y < m_rules.get_space_height(); ++y)
                {
                    cint Y = y*sim_ld;
                    cint mask_y_off = m_rules.get_space_height() + YB;
                    cint YB_ = offset + (y - mask_y_off) * mask_ld - XB;
                    __assume_aligned(sim_space, 64);
                    __assume_aligned(mask_space, 64);
                    for (int x = XB; x < XE; ++x)
                        f += sim_space[x + Y] * mask_space[x + YB_];
                }
            }
        }
        else if ((YB >= 0) && (YE < m_rules.get_space_height()))
        {
            // special case 4. Access of right border
            for (int y = YB; y < YE; ++y)
            {
                cint Y = y*sim_ld;
                cint YB_ = offset + (y - YB) * mask_ld - XB;
                __assume_aligned(sim_space, 64);
                __assume_aligned(mask_space, 64);
                for (int x = XB; x < m_rules.get_space_width(); ++x)
                    f += sim_space[x + Y] * mask_space[x + YB_];
            }

            cint mask_x_off = m_rules.get_space_width() - XB + 1; // should be +1
            // semi-optimized, wrapped access over the right border
            for (int y = YB; y < YE; ++y)
            {
                cint Y = y*sim_ld;
                cint YB_ = offset + mask_x_off + (y - YB) * mask_ld;
                __assume_aligned(sim_space, 64);
                __assume_aligned(mask_space, 64);
                for (int x = 0; x < (XE - m_rules.get_space_width()); ++x)
                    f += sim_space[x + Y] * mask_space[x + YB_];
            }
        }
        else
        {
            // hard case. use wrapped version.
            for (int y = YB; y < YE; ++y)
            {
                cint Y = y;
                cint YB_ = offset + (y - YB) * mask_ld - XB;
                for (int x = XB; x < XE; ++x)
                    f += space_current->getValueWrapped(x, y) * mask_space[x + YB_];
            }
        }
    }
    else if ((YB >= 0) && (YE < m_rules.get_space_height()))
    {
        // special case 3. Access over left border
        for (int y = YB; y < YE; ++y)
        {
            cint Y = y*sim_ld;
            cint YB_ = offset + (y - YB) * mask_ld - XB;
            __assume_aligned(sim_space, 64);
            __assume_aligned(mask_space, 64);
            for (int x = 0; x < XE; ++x)
                f += sim_space[x + Y] * mask_space[x + YB_];
            //f += space_current->getValue(x, y) * mask.getValue(x - XB, y - YB);
        }

        // NOTE: XB is negative here.
        for (int y = YB; y < YE; ++y)
        {
            cint Y = y*sim_ld;
            cint XB_ = m_rules.get_space_width() + XB;
            cint YB_ = offset + (y - YB) * mask_ld - XB_;
            __assume_aligned(sim_space, 64);
            __assume_aligned(mask_space, 64);
            for (int x = m_rules.get_space_width() + XB; x < m_rules.get_space_width(); ++x)
                f += sim_space[x + Y] * mask_space[x + YB_];
            //f += space_current->getValue(x, y) * mask.getValue(x - XB_, y - YB);
        }
    }
    else
    {
        // hard case. use wrapped version.
        for (int y = YB; y < YE; ++y)
        {
            cint Y = y;
            cint YB_ = offset + (y - YB) * mask_ld - XB;

            for (int x = XB; x < XE; ++x)
                f += space_current->getValueWrapped(x, y) * mask_space[x + YB_];
        }
    }

    return f / mask_sum; // normalize f
}

float simulator::getFilling_unoptimized(cint at_x, cint at_y, const aligned_matrix<float> &mask, cfloat mask_sum)
{
    // The theorectically considered bondaries
    cint XB = at_x - mask.getNumCols() / 2; // aka x_begin ; Ld can be greater, than #cols!
    cint XE = at_x + mask.getNumCols() / 2; // aka x_end
    cint YB = at_y - mask.getNumRows() / 2; // aka y_begin
    cint YE = at_y + mask.getNumRows() / 2; // aka y_end

    float f = 0;

    if (XB >= 0 && YB >= 0 && XE < m_rules.get_space_width() && YE < m_rules.get_space_height())
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
