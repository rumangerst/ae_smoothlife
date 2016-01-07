#include "simulator.h"
#include "aligned_vector.h"
#include <assert.h>

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

    space_current = new vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());
    space_next = new vectorized_matrix<float>(rules.get_space_width(), rules.get_space_height());
    space_of_renderer.store(space_current);

    // Initialize masks without smoothing
    /*
    outer_mask = matrix<float>(rules.ri * 2, rules.ri * 2);
    inner_mask = matrix<float>(rules.ra * 2, rules.ra * 2);

    outer_mask.set_circle(rules.ri, 1, false);
    inner_mask.set_circle(rules.ra, 1, false);
    inner_mask.set_circle(rules.ri, 0, false);*/

    // Initialize masks with smoothing
    // Either radius must be decreased by 1 or size increased by 2
    // Decrease radius = Floor integral approx.
    // Increase size = Ceiling integral approx.
    outer_mask = vectorized_matrix<float>(rules.get_ri() * 2 + 2, rules.get_ri() * 2 + 2);
    inner_mask = vectorized_matrix<float>(rules.get_ra() * 2 + 2, rules.get_ra() * 2 + 2);

    outer_mask.set_circle(rules.get_ri(), 1, 1);
    inner_mask.set_circle(rules.get_ra(), 1, 1);
    inner_mask.set_circle(rules.get_ri(), 0, 1);

    outer_mask_sum = outer_mask.sum();
    inner_mask_sum = inner_mask.sum();

#if SIMULATOR_MODE == MODE_SIMULATE || SIMULATOR_MODE == MODE_TEST_INITIALIZE
    //initialize_field_propagate();
    //initialize_field_random();
    //initialize_field_1();
     initialize_field_splat();
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

    initialized = true;
}

void simulator::simulate_step()
{
  #pragma omp parallel for schedule(static,1)
  for(int x = 0; x < rules.get_space_width(); ++x)
  {
      for(int y = 0; y < rules.get_space_height(); ++y)
      {
	  cfloat n = optimize ? getFilling(x, y, inner_mask, inner_mask_sum) : getFilling_unoptimized(x, y, inner_mask, inner_mask_sum); // filling of inner circle
	  cfloat m = optimize ? getFilling(x, y, outer_mask, outer_mask_sum) : getFilling_unoptimized(x, y, outer_mask, outer_mask_sum); // filling of outer ring

	  //Calculate the new state based on fillings n and m
	  //Smooth state function must be clamped to [0,1] (this is also done by author's implementation!)
	  space_next->setValue(rules.get_is_discrete() ? discrete_state_func_1(n,m) : fmax(0,fmin(1,next_step_as_euler(x,y,n,m))), x,y);

      }
  }

  //Swap fields
  std::swap(space_current, space_next);
  ++spacetime;
}


void simulator::run_simulation_master()
{
    cout << "Running ..." << endl;
    running = true;

    #ifdef ENABLE_PERF_MEASUREMENT
        auto perf_time_start = chrono::high_resolution_clock::now();
        ulong perf_spacetime_start = 0;
    #endif

    #pragma omp parallel
    {
        while(running)
        {     
	    // Calculate the next state if simulation is enabled
	    // Separate the actual simulation from interfacing
	    if(SIMULATOR_MODE == MODE_SIMULATE)
	      simulate_step();

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
                while(!*this->is_space_drawn_once_by_renderer && running) {} // wait for renderer... !!! always add check for "running"
            }

            space_of_renderer.store(space_current); //Tell outside (e.g. renderer)
            *this->is_space_drawn_once_by_renderer = false;
            this->new_space_available = false;
            /* NOTE: End of syncro */

            #ifdef ENABLE_PERF_MEASUREMENT
                // NOTE: this should be done either directly after swapping or here
                // HACK: here is best - doesn't interrupt code-flow to much :)
                if(spacetime % 100 == 0)
                {
                    auto perf_time_end = chrono::high_resolution_clock::now();
                    double perf_time_seconds = chrono::duration<double>(perf_time_end - perf_time_start).count();

                    cout << "Simulation || " << (spacetime - perf_spacetime_start) / perf_time_seconds << " calculations / s" << endl;

                    perf_spacetime_start = spacetime;
                    perf_time_start = chrono::high_resolution_clock::now();
                }
            #endif
        }
    }
}

float simulator::getFilling(cint at_x, cint at_y, const vectorized_matrix<float> &mask, cfloat mask_sum)
{
    // These define the rect inside the grid being accessed by mask
    cint XB = at_x - mask.getNumCols() / 2; // aka x_begin
    cint XE = at_x + mask.getNumCols() / 2; // aka x_end
    cint YB = at_y - mask.getNumRows() / 2; // aka y_begin
    cint YE = at_y + mask.getNumRows() / 2; // aka y_end

	/* TODO: known issues:
	 * - die accessed index 'at_x' is only sometimes aligned...
	 */
    
	cint sim_ld = space_current->getLd();
    cint mask_ld = mask.getLd();
	const float* const __restrict__ sim_space = this->space_current->getValues();
	const float* const __restrict__ mask_space = mask.getValues();
	
	assert(long(sim_space) % 64 == 0);
	assert(long(mask_space) % 64 == 0);

    //NOTE: if XB or YB is negative, we do not need to check XE or YE - they have to be within the field boundaries
	float f = 0;
    if (XB >= 0) {
        if (XE < rules.get_space_width()) {
            // NOTE: x accessible without wrapping

            if (YB >= 0) {
                if (YE < rules.get_space_height()) {
                    // ideal case. no wrapping
                    for(int y = YB; y < YE; ++y) {
                        cint Y = y*sim_ld;
                        cint YB_ = (y-YB)*mask_ld;
						__assume_aligned(mask_space, ALIGNMENT);
						__assume_aligned(sim_space, ALIGNMENT);
                        for(int x = XB; x < XE; ++x) {
                            //f += space_current->getValue(x,Y) * mask.getValue(x - XB, YB_);
							f += sim_space[x + Y] * mask_space[(x-XB) + YB_];
                        }
                    }
                } else {
                    // special case 2. Idially vectorized.
                    for(int y=YB; y<rules.get_space_height(); ++y) {
                        cint YB_ = y-YB;
                        for(int x=XB; x<XE; ++x) {
                            f += space_current->getValue(x,y) * mask.getValue(x-XB,YB_);
                        }
                    }

                    // optimized, wrapped access over the bottom border.
                    for(int y=0; y<YE-rules.get_space_height(); ++y) {
                        cint mask_y_off = mask.getNumRows() - (YE-rules.get_space_height());
                        for(int x=XB; x<XE; ++x) {
                            f += space_current->getValue(x,y) * mask.getValue(x-XB, mask_y_off+y);
                        }
                    }
                }
            } else {
                // special case 1. Idially vectorized.
                for(int y=0; y<YE; ++y) {
                    cint YB_ = y-YB;
                    for(int x=XB; x<XE; ++x) {
                        f += space_current->getValue(x,y) * mask.getValue(x-XB,YB_);
                    }
                }

                // optimized, wrapped access over the bottom border.
                for(int y=rules.get_space_height()+YB; y<rules.get_space_height(); ++y) {
                    cint mask_y_off = rules.get_space_height() + YB;
                    for(int x=XB; x<XE; ++x) {
                        f += space_current->getValue(x,y) * mask.getValue(x-XB, y-mask_y_off);
                    }
                }
            }

        } else if ((YB >= 0) && (YE < rules.get_space_height())) {
            // special case 4. Non-unit strides (probably unavoidable).
            for(int x=XB; x<rules.get_space_width(); ++x) {
                cint XB_ = x-XB;
                for(int y=YB; y<YE; ++y) {
                    f += space_current->getValue(x,y) * mask.getValue(XB_, y-YB);
                }
            }


            // optimized, wrapped access over the bottom border.
            for(int x=0; x<(XE-rules.get_space_width()); ++x) {
                cint mask_x_off = mask.getNumCols() - (XE-rules.get_space_width());
                for(int y=YB; y<YE; ++y) {
                    f += space_current->getValue(x,y) * mask.getValue(x+mask_x_off, y-YB);
                }
            }
        } else {
            // hard case. use wrapped version.
            for(int y = YB; y < YE; ++y) {
                cint Y=y;
                cint YB_ = y-YB;
                for(int x = XB; x < XE; ++x) {
                    f += space_current->getValueWrapped(x,Y) * mask.getValue(x - XB, YB_);
                }
            }
        }
    } else if ((YB >= 0) && (YE < rules.get_space_height())) {
        // special case 3. May have non-unit stride.
        for(int x=0; x<XE; ++x) {
            cint XB_ = x-XB;
            for(int y=YB; y<YE; ++y) {
                f += space_current->getValue(x,y) * mask.getValue(XB_, y-YB);
            }
        }

        // NOTE: XB is negative here.
        for(int x=rules.get_space_width()+XB; x<rules.get_space_width(); ++x) {
            cint XB_ = rules.get_space_width() + XB;
            for(int y=YB; y<YE; ++y) {
                f += space_current->getValue(x,y) * mask.getValue(x-XB_, y-YB);
            }
        }
    } else {
        // hard case. use wrapped version.
        for(int y = YB; y < YE; ++y) {
            cint Y=y;
            cint YB_ = y-YB;
            for(int x = XB; x < XE; ++x) {
                f += space_current->getValueWrapped(x,Y) * mask.getValue(x - XB, YB_);
            }
        }
    }

    return f / mask_sum; // normalize f
}


float simulator::getFilling_unoptimized(cint at_x, cint at_y, const vectorized_matrix<float> &mask, cfloat mask_sum) {
    // The theorectically considered bondaries
    cint XB = at_x - mask.getNumCols() / 2; // aka x_begin ; Ld can be greater, than #cols!
    cint XE = at_x + mask.getNumCols() / 2; // aka x_end
    cint YB = at_y - mask.getNumRows() / 2; // aka y_begin
    cint YE = at_y + mask.getNumRows() / 2; // aka y_end

    float f = 0;

    if (XB >= 0 && YB >= 0 && XE < rules.get_space_width() && YE < rules.get_space_height())
    {
        for(int y = YB; y < YE; ++y) {
            cint Y = y;
            cint Y_BEG = y-YB;
            for(int x = XB; x < XE; ++x) {
                f += space_current->getValue(x,Y) * mask.getValue(x - XB, Y_BEG);
            }
        }
    }
    else
    {
        for(int y = YB; y < YE; ++y) {
            cint Y = y;
            cint Y_BEG = y-YB;
            for(int x = XB; x < XE; ++x) {
                f += space_current->getValueWrapped(x,Y) * mask.getValue(x - XB, Y_BEG);
            }
        }
    }

    return f/mask_sum;
}
