#define CATCH_CONFIG_MAIN
#include "catch.hpp"

//Headers for test
#include "matrix.h"
#include "matrix_buffer_queue.h"
#include "simulator.h"

TEST_CASE("Test correct alignment of matrix class", "[matrix][cache]")
{
    vectorized_matrix<float> matrix = vectorized_matrix<float>(10, 12);

    for (int row = 0; row < matrix.getNumRows(); ++row)
    {
        ulong row_data_start = (ulong) matrix.getRow_ptr(row);
        REQUIRE(row_data_start % 64 == 0); //The row is aligned to 64 byte
    }
}

SCENARIO("Test correct behaviour of matrix_buffer_queue", "[matrix][queue]")
{

    GIVEN("a 100x100 matrix buffer queue with 10 queue size and initial matrix consisting only of 0")
    {
        matrix_buffer_queue<float> queue(10, vectorized_matrix<float>(100, 100));

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
                vectorized_matrix<float> * write_matrix = queue.buffer_write_ptr();

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

            vectorized_matrix<float> first_in_queue = vectorized_matrix<float>(100, 100);

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

SCENARIO("Test optimized simulation: Initialize at center", "[simulator]")
{

    GIVEN("a 400x300 state space with state '1' at the center")
    {
        // Initialize our space
        vectorized_matrix<float> space = vectorized_matrix<float>(400, 300);

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
            unoptimized_simulator.optimize = false;
            unoptimized_simulator.initialize(space);

            simulator optimized_simulator = simulator(rules);
            optimized_simulator.optimize = true;
            optimized_simulator.initialize(space);

            WHEN("both simulators are simulated 10 steps")
            {
                for (int steps = 0; steps < 10; ++steps)
                {
                    unoptimized_simulator.simulate_step();
                    optimized_simulator.simulate_step();
                }

                THEN("both simulators calculated the same state")
                {
                    vectorized_matrix<float> space_unoptimized = unoptimized_simulator.get_current_space();
                    vectorized_matrix<float> space_optimized = optimized_simulator.get_current_space();

                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = space_unoptimized.getValue(column, row);
                            float optimized_value = space_optimized.getValue(column, row);

                            REQUIRE(unoptimized_value == optimized_value);
                        }
                    }
                }

            }
        }
    }
}

TEST_CASE("Test optimized simulation: Initialize at left/right border", "[simulator]")
{

    GIVEN("a 400x300 state space with state '1' at the left & right border")
    {
        vectorized_matrix<float> space = vectorized_matrix<float>(400, 300);

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
            unoptimized_simulator.optimize = false;
            unoptimized_simulator.initialize(space);

            simulator optimized_simulator = simulator(rules);
            optimized_simulator.optimize = true;
            optimized_simulator.initialize(space);

            WHEN("both simulators are simulated 10 steps")
            {
                for (int steps = 0; steps < 10; ++steps)
                {
                    unoptimized_simulator.simulate_step();
                    optimized_simulator.simulate_step();
                }

                THEN("both simulators calculated the same state")
                {
                    vectorized_matrix<float> space_unoptimized = unoptimized_simulator.get_current_space();
                    vectorized_matrix<float> space_optimized = optimized_simulator.get_current_space();

                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = space_unoptimized.getValue(column, row);
                            float optimized_value = space_optimized.getValue(column, row);

                            REQUIRE(unoptimized_value == optimized_value);
                        }
                    }
                }

            }
        }
    }
}

TEST_CASE("Test optimized simulation: Initialize at top/bottom border", "[simulator]")
{

    GIVEN("a 400x300 state space with state '1' at the top & bottom border")
    {
        vectorized_matrix<float> space = vectorized_matrix<float>(400, 300);

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
            unoptimized_simulator.optimize = false;
            unoptimized_simulator.initialize(space);

            simulator optimized_simulator = simulator(rules);
            optimized_simulator.optimize = true;
            optimized_simulator.initialize(space);

            WHEN("both simulators are simulated 10 steps")
            {
                for (int steps = 0; steps < 10; ++steps)
                {
                    unoptimized_simulator.simulate_step();
                    optimized_simulator.simulate_step();
                }

                THEN("both simulators calculated the same state")
                {
                    vectorized_matrix<float> space_unoptimized = unoptimized_simulator.get_current_space();
                    vectorized_matrix<float> space_optimized = optimized_simulator.get_current_space();

                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = space_unoptimized.getValue(column, row);
                            float optimized_value = space_optimized.getValue(column, row);

                            REQUIRE(unoptimized_value == optimized_value);
                        }
                    }
                }

            }
        }
    }
}
