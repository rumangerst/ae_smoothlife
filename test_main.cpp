#define CATCH_CONFIG_MAIN
#include "catch.hpp"

//Headers for test
#include "matrix.h"
#include "matrix_buffer_queue.h"
#include "simulator.h"

/*
 * TODO: use space copy constructor
 */

/**
 * returns true, if |a-b| < deviation is true
 */
inline bool isApprox(cfloat a, cfloat b, cfloat deviation)
{
    return abs(a - b) < deviation;
}


/* test by ruman*/
TEST_CASE("Test correct alignment of matrix class", "[matrix][cache]")
{
    aligned_matrix<float> matrix = aligned_matrix<float>(10, 12);

    for (int row = 0; row < matrix.getNumRows(); ++row)
    {
        ulong row_data_start = (ulong) matrix.getRow_ptr(row);
        REQUIRE(row_data_start % ALIGNMENT == 0); //The row is aligned to 64 byte
    }
}

/* Test by Bastian */
SCENARIO("The area of all offset masks should be the same", "[masks]") {
    // TODO: use approx function
    GIVEN("A valid simulator with initiated masks")
    {
        ruleset rules = ruleset_smooth_life_l(400, 400);

        simulator sim = simulator(rules);
        sim.m_optimize = false;
        sim.initialize(); // initializes masks as well
        
        WHEN("Masks are initialized") {
            // check inner masks
            float sum_inner = sim.get_sum_of_mask(true, 0);
            float sum_outer = sim.get_sum_of_mask(false, 0);
            
            for (int i=1; i<sim.get_num_of_masks(); ++i) {
                REQUIRE(isApprox(sum_inner, sim.get_sum_of_mask(true, i), 5.0e-5));
                REQUIRE(isApprox(sum_outer, sim.get_sum_of_mask(false, i), 5.0e-5));
            }
        }
    }
}

/* test by bastian */
SCENARIO("Test pointer and wrapper correctness of matrix class", "[matrix][cache]")
{
    aligned_matrix<float> matrix = aligned_matrix<float>(10, 12);
    const float * data_row = matrix.getValues();
    
    WHEN("Pointers initialized")
    {
        THEN("Must point into the same address") {
            REQUIRE(long(data_row) == long(matrix.getValueWrapped_ptr(0,0)));
        }
    }

    WHEN("Pointer moved and new values grabbed")
    {
        THEN("Both must still point to the same address") {
            for (int row = 0; row < matrix.getNumRows(); ++row) {
                data_row = matrix.getRow_ptr(row);
                for (int col=0; col < matrix.getLd(); ++col ) {
                    REQUIRE(*data_row == matrix.getValue(col,row));
                    REQUIRE(*data_row == matrix.getValueWrappedLd(col,row));
                    REQUIRE(long(data_row) == long(matrix.getValue_ptr(col,row)));
                    REQUIRE(long(data_row) == long(matrix.getValueWrappedLd_ptr(col,row)));
                    data_row++;
                }
            }
        }
    }
}

/* test by ruman */
SCENARIO("Test correct behaviour of matrix_buffer_queue", "[matrix][queue]")
{

    GIVEN("a 100x100 matrix buffer queue with 10 queue size and initial matrix consisting only of 0")
    {
        matrix_buffer_queue<float> queue(10, aligned_matrix<float>(100, 100));

        WHEN("Checking the queue size")
        {

            THEN("The queue does not contain anything")
            {
                REQUIRE(queue.size() == 0);
            }
        }

        WHEN("Inserting 5 matrices composed only of 1,2,...5")
        {
            for (int i = 1; i <= 5; ++i)
            {
                aligned_matrix<float> * write_matrix = queue.buffer_write_ptr();

                for (int y = 0; y < write_matrix->getNumRows(); ++y)
                {
                    for (int x = 0; x < write_matrix->getNumCols(); ++x)
                    {
                        write_matrix->setValue(i, x, y);
                    }
                }

                REQUIRE(write_matrix->getValue(0, 0) == i);
                REQUIRE(queue.push());
            }

            REQUIRE(queue.push());

            THEN("The queue contains 6 items")
            {
                REQUIRE(queue.size() == 6);
            }

            aligned_matrix<float> first_in_queue = aligned_matrix<float>(100, 100);

            WHEN("Removing the first item")
            {
                REQUIRE(queue.pop(first_in_queue));

                THEN("we get a matrix with 0")
                {
                    REQUIRE(first_in_queue.getValue(0, 0) == 0);
                }

                WHEN("removing the next item")
                {
                    REQUIRE(queue.pop(first_in_queue));

                    THEN("we get a matrix with 1")
                    {
                        REQUIRE(first_in_queue.getValue(0, 0) == 1);
                    }

                    WHEN("removing the 3rd item")
                    {
                        REQUIRE(queue.pop(first_in_queue));

                        THEN("we get a matrix with 2")
                        {
                            REQUIRE(first_in_queue.getValue(0, 0) == 2);
                        }
                    }
                }
            }
        }
    }
}

/* test by ruman */
SCENARIO("Test optimized simulation: Initialize at center", "[simulator]")
{

    GIVEN("a 400x300 state space with state '1' at the center")
    {
        // Initialize our space
        aligned_matrix<float> space = aligned_matrix<float>(400, 300);

        for (int column = 100; column < 300; ++column)
        {
            for (int row = 50; row < 250; ++row)
            {
                space.setValue(1, column, row);
            }
        }

        GIVEN("one unoptimized and an optimized simulator")
        {
            ruleset rules = ruleset_smooth_life_l(space.getNumCols(), space.getNumRows());

            simulator unoptimized_simulator = simulator(rules);
            unoptimized_simulator.m_optimize = false;
            unoptimized_simulator.initialize(space);

            simulator optimized_simulator = simulator(rules);
            optimized_simulator.m_optimize = true;
            optimized_simulator.initialize(*(new aligned_matrix<float>(space)));

            WHEN("both simulators are simulated 10 steps")
            {
                for (int steps = 0; steps < 10; ++steps)
                {
                    unoptimized_simulator.simulate_step();
                    optimized_simulator.simulate_step();
                }

                THEN("both simulators calculated the same state")
                {
                    aligned_matrix<float> space_unoptimized = unoptimized_simulator.get_current_space();
                    aligned_matrix<float> space_optimized = optimized_simulator.get_current_space();

                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = space_unoptimized.getValue(column, row);
                            float optimized_value = space_optimized.getValue(column, row);

                            REQUIRE(isApprox(unoptimized_value, optimized_value, 0.5e-5));
                        }
                    }
                }

            }
        }
    }
}


/* test by ruman */
SCENARIO("Test optimized simulation: Initialize at left/right border with short time", "[simulator]")
{

    GIVEN("a 400x300 state space with state '1' at the left & right border")
    {
        aligned_matrix<float> space = aligned_matrix<float>(400, 300);

        for (int column = -100; column < 100; ++column)
        {
            for (int row = 0; row < 300; ++row)
            {
                space.setValueWrapped(1, column, row);
            }
        }

        GIVEN("one unoptimized and an optimized simulator")
        {
            ruleset rules = ruleset_smooth_life_l(space.getNumCols(), space.getNumRows());

            simulator unoptimized_simulator = simulator(rules);
            unoptimized_simulator.m_optimize = false;
            unoptimized_simulator.initialize(space);

            simulator optimized_simulator = simulator(rules);
            optimized_simulator.m_optimize = true;
            optimized_simulator.initialize(*(new aligned_matrix<float>(space))); // matrix copy constructor!

            WHEN("both simulators are simulated 10 steps")
            {
                for (int steps = 0; steps < 10; ++steps)
                {
                    unoptimized_simulator.simulate_step();
                    optimized_simulator.simulate_step();
                }

                THEN("both simulators calculated the same state")
                {
                    aligned_matrix<float> space_unoptimized = unoptimized_simulator.get_current_space();
                    aligned_matrix<float> space_optimized = optimized_simulator.get_current_space();

                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = space_unoptimized.getValue(column, row);
                            float optimized_value = space_optimized.getValue(column, row);

                            REQUIRE(isApprox(unoptimized_value, optimized_value, 0.5e-5));
                        }
                    }
                }

            }
        }
    }
}

/* test by bastian */
SCENARIO("Test optimized simulation: Initialize at top/bottom border with short time", "[simulator]")
{
    GIVEN("a 400x300 state space with state '1' at the top & bottom border")
    {
        aligned_matrix<float> space = aligned_matrix<float>(400, 300);

        for (int column = 0; column < 400; ++column)
        {
            for (int row = -50; row < 50; ++row)
            {
                space.setValueWrapped(1, column, row);
            }
        }

        GIVEN("one unoptimized and an optimized simulator")
        {
            ruleset rules = ruleset_smooth_life_l(space.getNumCols(), space.getNumRows());

            simulator unoptimized_simulator = simulator(rules);
            unoptimized_simulator.m_optimize = false;
            unoptimized_simulator.initialize(space);

            simulator optimized_simulator = simulator(rules);
            optimized_simulator.m_optimize = true;
            optimized_simulator.initialize(*(new aligned_matrix<float>(space)));

            WHEN("both simulators are simulated 10 steps")
            {
                for (int steps = 0; steps < 10; ++steps)
                {
                    unoptimized_simulator.simulate_step();
                    optimized_simulator.simulate_step();
                }

                THEN("both simulators calculated the same state")
                {
                    aligned_matrix<float> space_unoptimized = unoptimized_simulator.get_current_space();
                    aligned_matrix<float> space_optimized = optimized_simulator.get_current_space();

                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = space_unoptimized.getValue(column, row);
                            float optimized_value = space_optimized.getValue(column, row);

                            REQUIRE(isApprox(unoptimized_value, optimized_value, 0.5e-5));
                        }
                    }
                }

            }
        }
    }
}

/* test by bastian */
SCENARIO("Test optimized simulation: Initialize at left/right border with long time", "[simulator]")
{
    GIVEN("a 400x300 state space with state '1' at the left & right border")
    {
        aligned_matrix<float> space = aligned_matrix<float>(400, 300);

        for (int column = -100; column < 100; ++column)
        {
            for (int row = 0; row < 300; ++row)
            {
                space.setValueWrapped(1, column, row);
            }
        }

        GIVEN("one unoptimized and an optimized simulator")
        {
            ruleset rules = ruleset_smooth_life_l(space.getNumCols(), space.getNumRows());

            simulator unoptimized_simulator = simulator(rules);
            unoptimized_simulator.m_optimize = false;
            unoptimized_simulator.initialize(space);

            simulator optimized_simulator = simulator(rules);
            optimized_simulator.m_optimize = true;
            optimized_simulator.initialize(*(new aligned_matrix<float>(space)));

            WHEN("both simulators are simulated 10 steps")
            {
                for (int steps = 0; steps < 100; ++steps)
                {
                    if (steps%10 == 0) cout << "step: " << steps << endl;
                    unoptimized_simulator.simulate_step();
                    optimized_simulator.simulate_step();
                }

                THEN("both simulators calculated the same state")
                {
                    aligned_matrix<float> space_unoptimized = unoptimized_simulator.get_current_space();
                    aligned_matrix<float> space_optimized = optimized_simulator.get_current_space();
                    bool hasValues = false;
                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = space_unoptimized.getValue(column, row);
                            float optimized_value = space_optimized.getValue(column, row);
                            hasValues |= unoptimized_value == 0.0 || optimized_value == 0.0;
                            REQUIRE(isApprox(unoptimized_value, optimized_value, 0.5e-5));
                        }
                    }
                    
                    cout << "Field was empty: " << !hasValues << endl;
                }

            }
        }
    }
}