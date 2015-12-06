cmake_minimum_required(VERSION 3.0)
project(smoothlife)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${smoothlife_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "-std=c++1y -pedantic -march=native -Wall -Werror -fopenmp -ftree-vectorize -ftree-vectorizer-verbose=5")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -DNDEBUG -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

set(SOURCES main.cpp simulator.cpp gui.cpp matrix.cpp ruleset.cpp)

find_package(SDL2 REQUIRED)
find_package(SDL2_gfx REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2GFX_INCLUDE_DIR})

add_executable(smoothlife ${SOURCES})
target_link_libraries(smoothlife ${SDL2_LIBRARY} ${SDL2GFX_LIBRARY})