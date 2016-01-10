#pragma once

#include <iostream>
#include <vector>
#include <atomic>
#include "matrix.h"

using namespace std;

template <typename T>

/**
 * @brief contains N matrices and offers FIFO like properties.
 */
class matrix_buffer
{
public:

    matrix_buffer(int size, vectorized_matrix<T> initial)
    {
        if(size <= 1)
        {
            cerr << "matrix_buffer should at least have size 2" << endl;
            exit(EXIT_FAILURE);
        }
        
        // Reserve buffer and put initial value (read value) into the buffer
        buffer.reserve(size);        
        buffer.push_back(initial);
        for(int i = 0; i < size - 1; ++size)
        {
            buffer.push_back(vectorized_matrix<T>(initial.getNumCols(), initial.getNumRows()));
        }
        
        queue_start = 0;
        queue_size = 1;
        buffer_write = 1;
    }
    
    /**
     * @brief Returns copy of the current queue front and removes it. Program will terminate if pop() is used with an empty queue!
     * @return 
     */
    vectorized_matrix<T> queue_pop()
    {
        if(queue_size == 0)
        {
            cerr << "queue_pop() called on empty queue!" << endl;
            exit(EXIT_FAILURE);
        }
        
        vectorized_matrix<T> M = vectorized_matrix<T>(buffer[queue_start]); //aquire data
        
        //shrink the queue by 1
        queue_start = queue_start + 1;
        queue_size = queue_size - 1;
        
        return M;
    }
    
    /**
     * @brief Returns the current queue size
     * @return 
     */
    int get_queue_size()
    {
        return queue_size;
    }
    
    /**
     * @brief Tests if the buffer is writable.
     * @return true if buffer is writable
     */
    bool buffer_can_write_next()
    {
        return wrap_index(buffer_write + 1) != queue_start;
    }
    
    /**
     * @brief Moves to next writing position if possible
     * @return nullptr if writing is not possible
     */
    vectorized_matrix<T> * buffer_next()
    {
        //TODO: Make thread safe
        if(buffer_can_write_next())
        {
            buffer_write = wrap_index(buffer_write + 1); //Move to next write index
            queue_size = queue_size + 1; //The queue now has one item more
            
            return buffer[buffer_write];
        }
        else
        {
            return nullptr;
        }
    }
    
    /**
     * @brief returns current writing position
     * @return 
     */
    vectorized_matrix<T> * buffer_write_ptr()
    {
        return buffer[buffer_write];
    }
    
    /**
     * @brief returns current read position
     * @return 
     */
    vectorized_matrix<T> * buffer_read_ptr()
    {
        return buffer[wrap_index(buffer_write - 1)];
    }

private:
    
    vector<vectorized_matrix<T>> buffer;
    atomic<int> queue_start; //Start of the queue
    atomic<int> queue_size; // Size of the queue
    
    atomic<int> buffer_write; //Position where the buffer is writing
    
    int wrap_index(int i)
    {
        return i % buffer.size();
    }

};