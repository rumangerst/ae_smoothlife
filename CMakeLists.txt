cmake_minimum_required(VERSION 3.0)
project(ae_smoothlife)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ae_smoothlife_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "-std=c++1y -pedantic -march=native -Wall -Werror -fopenmp -ftree-vectorize -ftree-vectorizer-verbose=5")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -DNDEBUG -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

set(SOURCES
main.cpp
simulator.cpp
gui.cpp
matrix.h
ruleset.h
gui_color.h
ogl_gui.cpp
ogl_shader_variable.h
ogl_shader.cpp
ogl_texture.cpp
shaders/grayscale.vsh
shaders/grayscale.fsh
shaders/microscope.vsh
shaders/microscope.fsh)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})

add_executable(smoothlife ${SOURCES})
target_link_libraries(smoothlife ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES})
