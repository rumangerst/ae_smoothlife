#pragma once

#include <iostream>
#include <math.h>
#include <vector>
#include <random>
#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <memory>
#include "matrix.h"
#include "matrix_buffer_queue.h"
#include "ruleset.h"
#include "aligned_vector.h"
#include "communication.h"
#include <unistd.h>

using namespace std;

#define SIMULATOR_INITIALIZATION_FUNCTION space_set_splat //The function used for initialization

#define SPACE_QUEUE_MAX_SIZE 32 //the queue size used by the program
#define USE_PEELED false

/**
 * @brief Encapsulates the calculation of states
 * concept: both
 * core and field initiation routines: Ruman
 * vectorization and OpenMP parallization: Bastian
 */
class simulator
{
public:
    simulator(const ruleset & rules);
    ~simulator();

    ruleset m_rules;
    
    
    matrix_buffer_queue<float> * m_space; // Stores all calculated spaces to be fetched by local GUI or sent by MPI. 
    #define space_current m_space->buffer_read_ptr() //Redirect space_current to the read pointer provided by matrix_buffer
    #define space_next m_space->buffer_write_ptr() //redirect space_next to the write pointer provided by matrix_buffer
    
    ulong spacetime = 0;

    // we need 2 masks for each unaligned space cases (to re-align it)
    vector<aligned_matrix<float>> m_outer_masks; // masks to calculate the filling of the outer ring
    vector<aligned_matrix<float>> m_inner_masks; // masks to calculate the filling of the inner cycle
    float m_outer_mask_sum;
    float m_inner_mask_sum;
    int   m_offset_from_mask_center; // the number of grid units from the center of masks [0] to index (0,0)

    bool m_initialized = false;
    bool m_running = false;
    bool m_reinitialize = false;
    bool m_optimize = true; //use the optimized methods    


    /**
     * @brief Initializes all necessary fields. Initializes field with default initialization function
     */
    void initialize();
    
    /**
     * @brief Initialize all necessary fields
     * @param predefined_space A predefined space 
     * @MakeOver Bastian
     */
    void initialize(aligned_matrix<float> & predefined_space);
    
    /**
     * @brief Simulates 1 (or dt) steps. Simulate for whole field
     * @note Public because we'll need this for our tests
     * @MakeOver Bastian
     */
    void simulate_step();
    
    /**
     * @brief Simulates 1 (or dt) steps. Simulate only field from x_start to x_start + w. Needed for slave simulators.
     * @note Public because we'll need this for our tests
     * @MakeOver Bastian
     */
    void simulate_step(int x_start, int w);       
    
    /**
     * @brief Runs simulation as master simulator. Distributes work over MPI, but also does some work itself.
     */
    void run_simulation_master();
    
    /**
     * @brief Runs the simulation as slave simulator
     */
    void run_simulation_slave();

    
    /**
     * @brief Returns copy of the current space
     * @return 
     */
    aligned_matrix<float> get_current_space()
    {
        return aligned_matrix<float>(*space_current);
    }
    
    /**
     * @brief basically only used for testing with some capsulation
     * @param IsInner if true, the inner masks will be accessed
     * @param id of the matrix that shall be used. Is equal to 'offset'
     * @return the accumulation of all values contained in matrix[id]
     */
    float get_sum_of_mask(bool IsInner, int id) const {
        if (id < 0 or id >= get_num_of_masks()) {
            cerr << "Cannot get sum of mask: Invalid id for mask! \n";
            exit(1);
        }
        
        return (IsInner) ? m_inner_masks[id].sum() : m_outer_masks[id].sum();
    }
    
    
    int get_num_of_masks() const {
        assert(m_outer_masks.size() == m_inner_masks.size());
        return m_outer_masks.size();
    }

private:
    
    /**
     * @brief Returns the chunk that is calculated by rank of
     */
    int get_mpi_chunk_index(int of)
    {
        return (of + 1) % mpi_comm_size();
    }
    
    /**
     * @brief Returns the chunk that is calculated by this rank
     */
    int get_mpi_chunk_index()
    {
        return get_mpi_chunk_index(mpi_rank());
    }
    
    /**
     * @brief Returns the size of the border of a slave simulator chunk
     * @return 
     */
    int get_mpi_chunk_border_width()
    {
        return matrix_calc_ld_with_padding(sizeof(float), m_rules.get_radius_outer() + 1, CACHELINE_SIZE);
    }
    
    /**
     * @brief Returns how much width of the field calculation a rank gets. Exits program if division cannot be done.
     * @return 
     */
    int get_mpi_chunk_width()
    {
        int ranks = mpi_comm_size();
        
        if(m_rules.get_space_width() % ranks != 0)
        {
            cerr << "Cannot divide space into same parts! Terminating." << endl;
            exit(EXIT_FAILURE);
        }
        
        return m_rules.get_space_width() / ranks;
    }
    
    /**
     * @brief prepares all offset masks (CACHELINE_SIZE / sizeof(floats) many)
     * @author Bastian
     */
    void initiate_masks();    

    void space_set_random(aligned_matrix<float>* space)
    {
        random_device rd;
        default_random_engine re(rd());
        uniform_real_distribution<float> random_state(0,1);

        for(int x = 0; x < m_rules.get_space_width(); ++x)
        {
            for(int y = 0; y < m_rules.get_space_height(); ++y)
            {
                space->setValue(random_state(re),x,y);
            }
        }
    }

    /**
     * @brief initialize_field_splat Taken from reference implementation to generate "splats"
     */
    void space_set_splat(aligned_matrix<float>* space)
    {        
        //Initialize with 0 first (needed for reinitialize)
        for(int y = 0; y < space->getNumRows(); ++y)
        {
            float * row = space->getRow_ptr(y);
            
            for(int x = 0; x < space->getNumCols(); ++x)
            {
                row[x] = 0;
            }
        }
        
        random_device rd;
        default_random_engine re(rd());

        uniform_real_distribution<float> random_idk(0,0.5);
        uniform_real_distribution<float> random_point_x(0,m_rules.get_space_width());
        uniform_real_distribution<float> random_point_y(0,m_rules.get_space_height());

        float mx, my;

        mx = 2*m_rules.get_radius_outer(); if (mx>m_rules.get_space_width()) mx=m_rules.get_space_width();
        my = 2*m_rules.get_radius_outer(); if (my>m_rules.get_space_height()) my=m_rules.get_space_height();

        for(int t=0; t<=(int)(m_rules.get_space_width()*m_rules.get_space_height()/(mx*my)); ++t)
        {
            float mx, my, dx, dy, u, l;
            int ix, iy;

            mx = random_point_x(re);
            my = random_point_y(re);
            u = m_rules.get_radius_outer()*(random_idk(re) + 0.5);

            for (iy=(int)(my-u-1); iy<=(int)(my+u+1); ++iy)
                for (ix=(int)(mx-u-1); ix<=(int)(mx+u+1); ++ix)
                {
                    dx = mx-ix;
                    dy = my-iy;
                    l = sqrt(dx*dx+dy*dy);
                    if (l<u)
                    {
                        int px = ix;
                        int py = iy;
                        while (px<  0) px+=m_rules.get_space_width();
                        while (px>=m_rules.get_space_width()) px-=m_rules.get_space_width();
                        while (py<  0) py+=m_rules.get_space_height();
                        while (py>=m_rules.get_space_height()) py-=m_rules.get_space_height();
                        if (px>=0 && px<m_rules.get_space_width() && py>=0 && py<m_rules.get_space_height())
                        {
                            space->setValue(1.0,px,py);
                        }
                    }
                }
        }
    }

    void space_set_propagate(aligned_matrix<float>* space)
    {
        random_device rd;
        default_random_engine re(rd());

        uniform_real_distribution<float> random_state(0,1);

        const float p_seed = 0.01;
        const float p_proagate = 0.3;

        //Seed
        for(int x = 0; x < m_rules.get_space_width(); ++x)
        {
            for(int y = 0; y < m_rules.get_space_height(); ++y)
            {
                space->setValue(random_state(re) <= p_seed ? 1 : 0, x,y);
            }
        }

        //Propagate
        for(int i = 0; i < 5; ++i)
        {
            for(int x = 0; x < m_rules.get_space_width(); ++x)
            {
                for(int y = 0; y < m_rules.get_space_height(); ++y)
                {
                    float f = space->getValue(x,y);

                    if(f > 0.5)
                    {
                        if(random_state(re) <= p_proagate)
                            space->setValueWrapped(1,x-1,y);
                        if(random_state(re) <= p_proagate)
                            space->setValueWrapped(1,x+1,y);
                        if(random_state(re) <= p_proagate)
                            space->setValueWrapped(1,x,y-1);
                        if(random_state(re) <= p_proagate);
                            space->setValueWrapped(1,x,y+1);
                    }
                }
            }
        }
    }

    inline float sigma1(cfloat x,cfloat a, cfloat alpha)
    {
        return 1.0 / ( 1.0 + exp(-(x-a)*4.0/alpha));
    }

    /*inline float sigma2(cfloat x,cfloat a,cfloat b, cfloat alpha)
    {
        return sigma1(x,a,alpha) * ( 1.0 - sigma1(x,b,alpha));
    }

    inline float sigmam(cfloat x,cfloat y,cfloat m, cfloat alpha)
    {
        return x * ( 1.0 - sigma1(m,0.5, alpha)) + y*sigma1(m, 0.5, alpha);
    }*/

    //Corrected rules according to ref. implementation
    inline float sigma2(cfloat x, cfloat a, cfloat b)
    {
        return sigma1(x,a,m_rules.get_alpha_n()) * ( 1.0 - sigma1(x,b,m_rules.get_alpha_n()));
    }

    inline float sigmam(cfloat x, cfloat y, cfloat m)
    {
        return x * ( 1.0 - sigma1(m,0.5,m_rules.get_alpha_m())) + y*sigma1(m, 0.5, m_rules.get_alpha_m());
    }

    /**
     * @brief s Calculates the new state
     * @param n Outer filling
     * @param m Inner Filling
     * @return New state
     */
    inline float discrete_state_func_1(cfloat outer, cfloat inner)
    {
        //According to ref. implementation
        return sigmam(sigma2(outer,m_rules.get_birth_min(),m_rules.get_birth_max()),sigma2(outer,m_rules.get_death_min(),m_rules.get_death_max()),inner);
    }

    inline float discrete_as_euler(cfloat outer, cfloat inner)
    {
        return 2.0 * discrete_state_func_1(outer, inner) - 1.0;
    }

    inline float next_step_as_euler(cint x, cint y, cfloat outer, cfloat inner)
    {
        return space_current->getValue(x,y) + m_rules.get_delta_time() * discrete_as_euler(outer,inner);
    }

    /**
     * @brief calculates the area around the point (x,y) based on the mask & normalizes it by mask_sum
     * @param at_x space x-coordinate
     * @param at_y space y-coordinate
     * @param mask a non-sparsed matrix with target set [0,1]
     * @param mask_sum the sum of all values in the given matrix (the maximal, obtainable value of this function)
     * @return a float with a value in [0,1]
     * @author Bastian
     */
    float getFilling(cint at_x, cint at_y, const aligned_matrix<float> const &mask, cfloat mask_sum);
    //float getFilling(cint at_x, cint at_y, const vector<aligned_matrix<float>> &masks, cint offset, cfloat mask_sum);

    /**
     * @brief calculates the area around the point (x,y) based on the mask & normalizes it by mask_sum
     * - this version allows peel loops. We speed up a little
     * - speed test required!
     * - edit: peeled is about 2 frames behind on my pc
     * @param at_x space x-coordinate
     * @param at_y space y-coordinate
     * @param mask a non-sparsed matrix with target set [0,1]
     * @param mask_sum the sum of all values in the given matrix (the maximal, obtainable value of this function)
     * @return a float with a value in [0,1]
     * @author Bastian
     */
    float getFilling_peeled(cint at_x, cint at_y, const vector<aligned_matrix<float>> &masks, cint offset, cfloat mask_sum);
    
    /**
     * @brief calculates the area around the point (x,y) based on the mask & normalizes it by mask_sum
     * USEFUL FOR TESTING THE CORRECTNIS OF OPTIMIZED CODE! KEEP THIS!
     * @param at_x at_x space x-coordinate
     * @param at_y space y-coordinate
     * @param mask a non-sparsed matrix with target set [0,1]
     * @param mask_sum the sum of all values in the given matrix (the maximal, obtainable value of this function)
     * @return a float with a value in [0,1]
     * @author Bastian
     */
    float getFilling_unoptimized(cint at_x, cint at_y, const aligned_matrix<float> & mask, cfloat mask_sum);
};
