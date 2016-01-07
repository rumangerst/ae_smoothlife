#define CATCH_CONFIG_MAIN
#include "catch.hpp"

//Headers for test
#include "matrix.h"
#include "simulator.h"

TEST_CASE( "Test system working", "[Test_alive]" )
{
    REQUIRE(true);
}

TEST_CASE("Test correct alignment of matrix class", "[matrix_aligned]")
{
    //Setup
    vectorized_matrix<float> matrix = vectorized_matrix<float>(10, 12);
    
    //Test
    for(int row = 0; row < matrix.getNumRows(); ++row)
    {
        REQUIRE((ulong)matrix.getRow(row) % 64 == 0); //The row is aligned to 64 byte
    }    
}

