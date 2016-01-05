cmake_minimum_required(VERSION 3.0)
project(ae_smoothlife)    

OPTION(BUILD_INTEL "Build with intel compiler" OFF)
OPTION(BUILD_MIC "Also build MIC version" OFF)
OPTION(GCC_VERBOSE_VECTOR "Show complete GCC vector report" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ae_smoothlife_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "-std=c++1y -O3 -pedantic -Wall -DENABLE_PERF_MEASUREMENT")

IF (BUILD_INTEL)
  #intel compiler version, additional flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopenmp -xhost -vec-report=5")
ELSE(BUILD_INTEL)
  #gcc compiler version, additional flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp  -march=native -ftree-vectorize")
  
  IF(GCC_VERBOSE_VECTOR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-vec -fopt-info-missed -fdump-tree-vect")#fdrump drops more information in *.vect files
  ENDIF()
  
ENDIF(BUILD_INTEL)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -DNDEBUG -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

set(SOURCES_SIM
main.cpp
simulator.cpp
matrix.h
aligned_vector.h
ruleset.h
mpi_manager.h)

set(SOURCES_GUI
sdl_gui.cpp
ogl_gui.cpp
ogl_shader_variable.h
ogl_shader.cpp
ogl_texture.cpp
shaders/grayscale.vsh
shaders/grayscale.fsh
shaders/microscope.vsh
shaders/microscope.fsh
shaders/microscope2.vsh
shaders/microscope2.fsh
shaders/blue.vsh
shaders/blue.fsh)

set(SOURCES ${SOURCES_SIM} ${SOURCES_GUI})

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})

find_package(Boost 1.59.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Three executables:
# smoothlife_sim: Simulator only, no graphics
# smoothlife_sim.mic: Simulator only, no graphics, compiled for MIC (intel only)
# smoothlife_ui: GUI only, obtains data from smoothlife and displays it
# smoothlife_local: 1 thread for GUI and other threads for simulation
add_executable(smoothlife_sim ${SOURCES_SIM})
add_executable(smoothlife_ui ${SOURCES_SIM} ${SOURCES_GUI})
add_executable(smoothlife_local ${SOURCES_SIM} ${SOURCES_GUI})

if(BUILD_INTEL AND BUILD_MIC)
  add_executable(smoothlife_sim.mic ${SOURCES_SIM})
endif()

# smoothlife_ui is determined by #ifdef GUI
# smoothlife_local gets #ifdef GUI and #ifdef SIM
# smoothlife_sim gets #ifdef SIM
set_target_properties(smoothlife_sim PROPERTIES COMPILE_FLAGS "-DSIM")
target_link_libraries(smoothlife_sim ${Boost_LIBRARIES})
set_target_properties(smoothlife_ui PROPERTIES COMPILE_FLAGS "-DGUI")
target_link_libraries(smoothlife_ui ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES})
set_target_properties(smoothlife_local PROPERTIES COMPILE_FLAGS "-DGUI -DSIM")
target_link_libraries(smoothlife_local ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES} ${Boost_LIBRARIES})

if(BUILD_INTEL AND BUILD_MIC)
  set_target_properties(smoothlife_sim.mic PROPERTIES COMPILE_FLAGS "-DSIM -mmic")
  target_link_libraries(smoothlife_sim.mic ${Boost_LIBRARIES})
endif()
