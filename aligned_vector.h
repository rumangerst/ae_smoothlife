//date: 26.12.2015, begin of alignment
//@author Bastian
#pragma once
#include <vector>
#include <boost/align/aligned_allocator.hpp>

#define ALIGNMENT 64 // memory border to align a memory address to
#define CACHELINE_SIZE 64
const int CACHELINE_FLOATS = CACHELINE_SIZE/sizeof(float);

template <typename T>
using aligned_vector = std::vector<T, boost::alignment::aligned_allocator<T, ALIGNMENT>>;