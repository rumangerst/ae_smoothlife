#include "simulator.h"

/*
 * DONE:
 * - simulation code correct (synchronized version)
 */

/*
 * TODO: Vectorization
 * 1. optimize access for rows
 * 2. apply static access to grid
 * 3. optimize code for border-free simulators
 * 4. optimize code for border simulators ( as far as possible)
 *
 * TODO: Parallization
 */

simulator::simulator(const ruleset & r) : rules(r)
{
    new_space_available.store(false);
}

simulator::~simulator()
{
    if(initialized)
    {
        delete space_current;
        delete space_next;
    }
}

void simulator::initialize()
{
    cout << "Initializing ..." << endl;

    space_current = new vectorized_matrix<float>(field_size_x, field_size_y);
    space_next = new vectorized_matrix<float>(field_size_x, field_size_y);
    space_of_renderer.store(space_current);

    // Initialize masks with smoothing
    /*
    outer_mask = matrix<float>(rules.ri * 2, rules.ri * 2);
    inner_mask = matrix<float>(rules.ra * 2, rules.ra * 2);

    outer_mask.set_circle(rules.ri, 1, false);
    inner_mask.set_circle(rules.ra, 1, false);
    inner_mask.set_circle(rules.ri, 0, false);*/

    // Initialize masks without smoothing
    // Either radius must be decreased by 1 or size increased by 2
    // Decrease radius = Floor integral approx.
    // Increase size = Ceiling integral approx.
    outer_mask = vectorized_matrix<float>(rules.ri * 2 + 2, rules.ri * 2 + 2);
    inner_mask = vectorized_matrix<float>(rules.ra * 2 + 2, rules.ra * 2 + 2);

    outer_mask.set_circle(rules.ri, 1, true);
    inner_mask.set_circle(rules.ra, 1, true);
    inner_mask.set_circle(rules.ri, 0, true);

    outer_mask_sum = outer_mask.sum();
    inner_mask_sum = inner_mask.sum();

#if SIMULATOR_MODE == MODE_SIMULATE || SIMULATOR_MODE == MODE_TEST_INITIALIZE
    //initialize_field_propagate();
    //initialize_field_random();
    //initialize_field_1();
     initialize_field_splat();
#endif

    initialized = true;
}

void simulator::simulate()
{
    cout << "Running ..." << endl;


#if SIMULATOR_MODE == MODE_SIMULATE

    running = true;

    while(running)
    {
        for(int x = 0; x < field_size_x; ++x)
        {
            for(int y = 0; y < field_size_y; ++y)
            {
                cfloat n = filling(x, y, inner_mask, inner_mask_sum); // filling of inner circle
                cfloat m = filling(x, y, outer_mask, outer_mask_sum); // filling of outer ring

                //Calculate the new state based on fillings n and m
                //Smooth state function must be clamped to [0,1] (this is also done by author's implementation!)
                space_next->setValue(rules.discrete ? discrete_state_func_1(n,m) : fmax(0,fmin(1,next_step_as_euler(x,y,n,m))), x,y);

            }
        }

        //Swap fields
        std::swap(space_current, space_next);
        ++spacetime;

        /*
         * NOTE: Start of synchronization
         * - the master simulator (and all others) waits for the renderer to finish drawing the last image
         * - it then blocks the renderer from continuing until "space_current" has been passed over
         * - HACK: deep copy would make it so much simpler :o
         */
        if (WAIT_FOR_RENDERING) {
            //TODO: may catch a signal from MPI (from the renderer) in the future & only the gui machine has to do that
            //HACK: in case of double buffer, we can already swap, but need to wait until rendering finished
            //HACK: if we use triple buffering, 2 time steps may be calculated before calculation threads start to idle!
            this->new_space_available = true; // prevent renderer from continuing after finishing the current image
            while(!*this->is_space_drawn_once_by_renderer) {
            } // wait for renderer...
        }

        if (spacetime%100 == 0)
            cout << "sim: " << spacetime << "\n";

        //Tell outside (e.g. renderer)
        space_of_renderer.store(space_current);
        *this->is_space_drawn_once_by_renderer = false;
        this->new_space_available = false; // finish sync
    }

#elif SIMULATOR_MODE == MODE_TEST_MASKS

    //Print the two masks
    for(int x = 0; x < inner_mask.getNumCols(); ++x)
    {
        for(int y = 0; y < inner_mask.getNumRows(); ++y)
        {
            space_current->setValue(inner_mask.getValue(x,y),x,y);
        }
    }
    for(int x = 0; x < outer_mask.getNumCols(); ++x)
    {
        for(int y = 0; y < outer_mask.getNumRows(); ++y)
        {
            space_current->setValue(outer_mask.getValue(x,y),x + inner_mask.getNumCols() + 10,y + inner_mask.getNumRows() + 10);
        }
    }
     space_of_renderer.store(space_current);

#elif SIMULATOR_MODE == MODE_TEST_COLORS

    //Test the colors
    for(int x = 0; x < field_size_x; ++x)
    {
        for(int y = 0; y < field_size_y; ++y)
        {
            double m = (double)y / field_size_y;
            double n = (double)x / field_size_x;

            double &  value = space_current->getValue(x,y);
            value = (n + m) / 2.0;
        }
    }

#elif SIMULATOR_MODE == MODE_TEST_STATE_FUNCTION
    //Test to print s(n,m)
    for(int x = 0; x < field_size_x; ++x)
    {
        for(int y = 0; y < field_size_y; ++y)
        {
            double m = (double)y / field_size_y;
            double n = (double)x / field_size_x;

            double &  value = space_current->getValue(x,y);
            value = s(n,m);
        }
    }

#endif
}

float simulator::filling(cint x, cint y, const vectorized_matrix<float> &mask, cfloat mask_sum)
{
    // The theorectically considered bondaries
    cint x_begin = x - mask.getNumRows() / 2;
    cint x_end = x + mask.getNumRows() / 2;
    cint y_begin = y - mask.getNumRows() / 2;
    cint y_end = y + mask.getNumRows() / 2;

    float f = 0;

    if(x_begin >= 0 && y_begin >= 0 && x_end < field_size_x && y_end < field_size_y)
    {
        // we access points safely here
        for(int y = y_begin; y < y_end; ++y)
        {
            #pragma omp simd
            for(int x = x_begin; x < x_end; ++x)
            {
                f += space_current->getValue(x,y) * mask.getValue(x - x_begin, y - y_begin);
            }
        }
    }
    else
    {
        // we access points "over" the edges of the grid here
        for(int y = y_begin; y < y_end; ++y)
        {
            for(int x = x_begin; x < x_end; ++x)
            {
                f += space_current->getValueWrapped(x,y) * mask.getValue(x - x_begin, y - y_begin);
            }
        }
    }

    return f / mask_sum; // normalize f
}
