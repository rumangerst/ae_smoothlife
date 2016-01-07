#define CATCH_CONFIG_MAIN
#include "catch.hpp"

//Headers for test
#include "matrix.h"
#include "simulator.h"

TEST_CASE("Test correct alignment of matrix class", "[matrix_aligned]")
{
    //Setup
    vectorized_matrix<float> matrix = vectorized_matrix<float>(10, 12);
    
    //Test
    for(int row = 0; row < matrix.getNumRows(); ++row)
    {
        ulong row_data_start = (ulong)matrix.getRow_ptr(row);
        
        REQUIRE(row_data_start % 64 == 0); //The row is aligned to 64 byte
    }    
}

TEST_CASE("Test correct padding of matrix class", "[matrix_padded]")
{
    //Setup
    vectorized_matrix<float> matrix = vectorized_matrix<float>(10, 12);
    
    // Test by checking if leading dimentsion is correct
    REQUIRE( matrix.getLd() * sizeof(float) % 64 == 0 );
}

TEST_CASE("Test optimized simulation: Initialize at center", "[sim_optimized_center]")
{
    FAIL("To be implemented: No support from simulation class, yet!");
}

TEST_CASE("Test optimized simulation: Initialize at left/right border", "[sim_optimized_left_right]")
{
    FAIL("To be implemented: No support from simulation class, yet!");
}

TEST_CASE("Test optimized simulation: Initialize at top/bottom border", "[sim_optimized_top_bottom]")
{
    FAIL("To be implemented: No support from simulation class, yet!");
}