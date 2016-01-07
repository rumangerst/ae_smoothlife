#define CATCH_CONFIG_MAIN
#include "catch.hpp"

//Headers for test
#include "matrix.h"
#include "simulator.h"

TEST_CASE("Test correct alignment of matrix class", "[matrix]")
{
    vectorized_matrix<float> matrix = vectorized_matrix<float>(10, 12);

    for (int row = 0; row < matrix.getNumRows(); ++row)
    {
        ulong row_data_start = (ulong) matrix.getRow_ptr(row);
        REQUIRE(row_data_start % 64 == 0); //The row is aligned to 64 byte
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
                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = unoptimized_simulator.space_of_renderer.load()->getValue(column, row);
                            float optimized_value = optimized_simulator.space_of_renderer.load()->getValue(column, row);

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
                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = unoptimized_simulator.space_of_renderer.load()->getValue(column, row);
                            float optimized_value = optimized_simulator.space_of_renderer.load()->getValue(column, row);

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
                    for (int column = 0; column < space.getNumCols(); ++column)
                    {
                        for (int row = 0; row < space.getNumRows(); ++row)
                        {
                            float unoptimized_value = unoptimized_simulator.space_of_renderer.load()->getValue(column, row);
                            float optimized_value = optimized_simulator.space_of_renderer.load()->getValue(column, row);

                            REQUIRE(unoptimized_value == optimized_value);
                        }
                    }
                }

            }
        }
    }
}