cmake_minimum_required(VERSION 3.0)
project(ae_smoothlife)    

# enable unit testing
enable_testing()

OPTION(BUILD_INTEL "Build with intel compiler" ON)
OPTION(BUILD_MIC "Also build MIC version" OFF)
OPTION(GCC_VERBOSE_VECTOR "Show complete GCC vector report" OFF)
OPTION(GUI_SDL "Use SDL GUI instead of OpenGL" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ae_smoothlife_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "-std=c++1y -O3 -pedantic -Wall -DENABLE_PERF_MEASUREMENT")

IF (BUILD_INTEL)
  #intel compiler version, additional flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopenmp -vec-report=5")
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
simulator.cpp)

set(SOURCES_TESTS
test_main.cpp)

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

set(SOURCES main.cpp ${SOURCES_SIM} ${SOURCES_GUI})

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})

find_package(Boost 1.59.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Add support for GUI

if(GUI_SDL)
	set(GUI_TYPE "sdl_gui")
	message("Compile GUI with SDL")
else()
	set(GUI_TYPE "ogl_gui")
	message("Compile GUI with OpenGL 2.0 and fancy stuff")
endif()

message("GUI_TYPE is ${GUI_TYPE}")

# Three executables:
# smoothlife_mpi_host: Communicates via MPI. Contains code for graphics
# smoothlife_mpi_mic: Communicates via MPI. Only contains simulator code.
# smoothlife_tests: tests
add_executable(smoothlife main.cpp ${SOURCES_SIM} ${SOURCES_GUI})
add_executable(smoothlife_perftest main.cpp ${SOURCES_SIM})
add_executable(smoothlife_tests ${SOURCES_TESTS} ${SOURCES_SIM})

if(BUILD_INTEL AND BUILD_MIC)
  add_executable(smoothlife.mic main.cpp ${SOURCES_SIM})
  add_executable(smoothlife_perftest.mic main.cpp ${SOURCES_SIM})
endif()

# smoothlife_ui is determined by #ifdef GUI
# smoothlife_local gets #ifdef GUI and #ifdef SIM
# smoothlife_sim gets #ifdef SIM
set_target_properties(smoothlife PROPERTIES COMPILE_FLAGS "-xhost -DAPP_GUI=true -DAPP_SIM=true -DGUI_TYPE=${GUI_TYPE}  -DAPP_PERFTEST=false -DAPP_UNIT_TEST=false")
target_link_libraries(smoothlife ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES} ${Boost_LIBRARIES})

set_target_properties(smoothlife_perftest PROPERTIES COMPILE_FLAGS "-xhost -DAPP_GUI=false -DAPP_SIM=true -DAPP_PERFTEST=true -DNDEBUG -DAPP_UNIT_TEST=false")
target_link_libraries(smoothlife_perftest ${Boost_LIBRARIES})

set_target_properties(smoothlife_tests PROPERTIES COMPILE_FLAGS "-xhost -DAPP_SIM=true -DAPP_GUI=false -DAPP_PERFTEST=false -DAPP_UNIT_TEST=true")
target_link_libraries(smoothlife_tests ${Boost_LIBRARIES})

if(BUILD_INTEL AND BUILD_MIC)
  set_target_properties(smoothlife.mic PROPERTIES LINK_FLAGS "-mmic" COMPILE_FLAGS "-DAPP_SIM=true -DAPP_GUI=false -DAPP_PERFTEST=false -mmic -DAPP_UNIT_TEST=false")
  target_link_libraries(smoothlife.mic ${Boost_LIBRARIES})
  set_target_properties(smoothlife_perftest.mic PROPERTIES LINK_FLAGS "-mmic" COMPILE_FLAGS "-DAPP_SIM=true -DAPP_GUI=false -DAPP_PERFTEST=true -mmic -DAPP_UNIT_TEST=false")
  target_link_libraries(smoothlife_perftest.mic ${Boost_LIBRARIES})
endif()

#Adding test to cmake
# run 'make test' to test
add_test(SMOOTHLIFE_TEST smoothlife_tests)
