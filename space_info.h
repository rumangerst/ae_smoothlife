#pragma once
#include <matrix.h>
#include <atomic>
#include <memory>

/**
 *  a simple container used to synchronize information between rendering and calculation threads
 */
template <typedef T>
struct space_info {
    std::shared_ptr<matrix<T>> space_ref = nullptr;
    std::atomic<bool> IsDrawn = false;
};
