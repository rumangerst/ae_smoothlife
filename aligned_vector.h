//date: 26.12.2015, begin of alignment
#pragma once
#include <vector>
#include <boost/align/aligned_allocator.hpp>

#define ALIGNMENT 64 // memory border to align a memory address to
#define CACHELINE_SIZE 64

template <typename T>
using aligned_vector = std::vector<T, boost::alignment::aligned_allocator<T, 64>>;
