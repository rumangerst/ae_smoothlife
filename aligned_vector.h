#pragma once
#include <vector>
#include <boost/align/aligned_allocator.hpp>

template <typename T>
using align_alloc = boost::alignment::aligned_allocator<T, 64>;

template <typename T>
using aligned_vector = std::vector<T, aligned_alloc>;
