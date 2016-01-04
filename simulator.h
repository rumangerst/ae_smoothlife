#pragma once
#define MODE_SIMULATE 0
#define MODE_TEST_INITIALIZE 1
#define MODE_TEST_MASKS 2
#define MODE_TEST_COLORS 3
#define MODE_TEST_STATE_FUNCTION 4

#include <iostream>
#include <math.h>
#include <vector>
#include <random>
#include <atomic>
#include <chrono>
#include "matrix.h"
#include "ruleset.h"
#include "aligned_vector.h"

using namespace std;

#define FIELD_W 300
#define FIELD_H 200

#define SIMULATOR_MODE MODE_SIMULATE //Set the mode of the simulator
#define WAIT_FOR_RENDERING true // if true, calcation threads will be waiting for the renderer to give a finishing signal


/**
 * @brief Encapsulates the calculation of states
 */
class simulator
{
public:
    simulator(const ruleset & rules);
    ~simulator();

    const int field_width = FIELD_W;
    const int field_height = FIELD_H;

    const ruleset rules;
    atomic<vectorized_matrix<float>*> space_of_renderer;
    atomic<bool>* is_space_drawn_once_by_renderer; //TODO: is probably obsolete with MPI, but still still good for testing first!
    atomic<bool> new_space_available;

    ulong spacetime = 0;

    vectorized_matrix<float> outer_mask; // mask to calculate the filling of the outer ring
    vectorized_matrix<float> inner_mask; // mask to calculate the filling of the inner cycle
    float outer_mask_sum;
    float inner_mask_sum;

    bool initialized = false;
    bool running = false;

    /**
     * @brief Initializes all necessary fields
     */
    void initialize();
    
    /**
     * @brief Simulates 1 (or dt) steps
     * @note Public because we'll need this for our tests
     */
    void simulate_step();
    
    /**
     * @brief Runs the simulation including interface with GUI as master simulator
     */
    void run_simulation_master();
    
    /**
     * @brief Runs the simulation including interface with master simulator as slave simulator
     */
    void run_simulation_slave();

private:

    vectorized_matrix<float>* space_current;
    vectorized_matrix<float>* space_next;

    void initialize_field_1()
    {
        for(int x = 0; x < field_width; ++x)
        {
            for(int y = 0; y < field_height; ++y)
            {
                space_current->setValue(1,x,y);
            }
        }
    }

    void initialize_field_random()
    {
        random_device rd;
        default_random_engine re(rd());
        uniform_real_distribution<float> random_state(0,1);

        for(int x = 0; x < field_width; ++x)
        {
            for(int y = 0; y < field_height; ++y)
            {
                space_current->setValue(random_state(re),x,y);
            }
        }
    }

    /**
     * @brief initialize_field_splat Taken from reference implementation to generate "splats"
     */
    void initialize_field_splat()
    {
        random_device rd;
        default_random_engine re(rd());

        uniform_real_distribution<float> random_idk(0,0.5);
        uniform_real_distribution<float> random_point_x(0,field_width);
        uniform_real_distribution<float> random_point_y(0,field_height);

        float mx, my;

        mx = 2*rules.ra; if (mx>field_width) mx=field_width;
        my = 2*rules.ra; if (my>field_height) my=field_height;

        for(int t=0; t<=(int)(field_width*field_height/(mx*my)); ++t)
        {
            float mx, my, dx, dy, u, l;
            int ix, iy;

            mx = random_point_x(re);
            my = random_point_y(re);
            u = rules.ra*(random_idk(re) + 0.5);

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
                        while (px<  0) px+=field_width;
                        while (px>=field_width) px-=field_width;
                        while (py<  0) py+=field_height;
                        while (py>=field_height) py-=field_height;
                        if (px>=0 && px<field_width && py>=0 && py<field_height)
                        {
                            space_current->setValue(1.0,px,py);
                        }
                    }
                }
        }
    }

    void initialize_field_propagate()
    {
        random_device rd;
        default_random_engine re(rd());

        uniform_real_distribution<float> random_state(0,1);

        const float p_seed = 0.01;
        const float p_proagate = 0.3;

        //Seed
        for(int x = 0; x < field_width; ++x)
        {
            for(int y = 0; y < field_height; ++y)
            {
                space_current->setValue(random_state(re) <= p_seed ? 1 : 0, x,y);
            }
        }

        //Propagate
        for(int i = 0; i < 5; ++i)
        {
            for(int x = 0; x < field_width; ++x)
            {
                for(int y = 0; y < field_height; ++y)
                {
                    float f = space_current->getValue(x,y);

                    if(f > 0.5)
                    {
                        if(random_state(re) <= p_proagate)
                            space_current->setValueWrapped(1,x-1,y);
                        if(random_state(re) <= p_proagate)
                            space_current->setValueWrapped(1,x+1,y);
                        if(random_state(re) <= p_proagate)
                            space_current->setValueWrapped(1,x,y-1);
                        if(random_state(re) <= p_proagate);
                            space_current->setValueWrapped(1,x,y+1);
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
        return sigma1(x,a,rules.alpha_n) * ( 1.0 - sigma1(x,b,rules.alpha_n));
    }

    inline float sigmam(cfloat x, cfloat y, cfloat m)
    {
        return x * ( 1.0 - sigma1(m,0.5,rules.alpha_m)) + y*sigma1(m, 0.5, rules.alpha_m);
    }

    /**
     * @brief s Calculates the new state
     * @param n Outer filling
     * @param m Inner Filling
     * @return New state
     */
    inline float discrete_state_func_1(cfloat n, cfloat m)
    {
        //According to ref. implementation
        return sigmam(sigma2(n,rules.b1,rules.b2),sigma2(n,rules.d1,rules.d2),m);
        //return sigma2(n, sigmam(rules.b1,rules.d1,m),sigmam(rules.b2,rules.d2,m));
    }

    inline float discrete_as_euler(cfloat n, cfloat m)
    {
        return 2.0 * discrete_state_func_1(n,m) - 1.0;
    }

    inline float next_step_as_euler(cint x, cint y, cfloat n, cfloat m)
    {
        return space_current->getValue(x,y) + rules.dt * discrete_as_euler(n,m);
    }

    /**
     * @brief calculates the area around the point (x,y) based on the mask & normalizes it by mask_sum
     * @param x
     * @param y
     * @param mask a non-sparsed matrix with target set [0,1]
     * @param mask_sum the sum of all values in the given matrix (the maximal, obtainable value of this function)
     * @return a float with a value in [0,1]
     */
    float getFilling(cint x, cint y, const vectorized_matrix<float> & mask, cfloat mask_sum);

    /**
     * @brief calculates the area around the point (x,y) based on the mask & normalizes it by mask_sum
     * USEFUL FOR TESTING THE CORRECTNIS OF OPTIMIZED CODE! KEEP THIS!
     * @param x
     * @param y
     * @param mask a non-sparsed matrix with target set [0,1]
     * @param mask_sum the sum of all values in the given matrix (the maximal, obtainable value of this function)
     * @return a float with a value in [0,1]
     */
    float getFilling_unoptimized(cint x, cint y, const vectorized_matrix<float> & mask, cfloat mask_sum);
};
