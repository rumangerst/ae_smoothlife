#pragma once

#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include "matrix.h"

using namespace std;

template <typename T>

/**
 * Combination between FIFO and state buffers. Allows in-place calculation and queueing of calculated states. 
 * Thread safe for two threads that call either push or pop.
 * 
 * @author Ruman
 */
class matrix_buffer_queue
{
public:

    matrix_buffer_queue(int size, vectorized_matrix<T> initial) : queue_max_size(size)
    {
        if (size < 0)
        {
            cerr << "Invalid matrix_buffer_queue size" << endl;
            exit(EXIT_FAILURE);
        }
        
        if (initial.getNumCols() <= 0 || initial.getNumRows() <= 0)
        {
            cerr << "Invalid initial matrix" <<endl;
            exit(EXIT_FAILURE);
        }

        // Reserve size queue buffer + 2 read/write buffer
        buffer.reserve(size + 2);

        buffer.push_back(initial); //Insert the initial read matrix
        buffer.push_back(vectorized_matrix<T>(initial.getNumCols(), initial.getNumRows())); //Insert the initial write matrix

        // Insert queue elements
        for (int i = 0; i < size; ++i)
        {
            buffer.push_back(vectorized_matrix<T>(initial.getNumCols(), initial.getNumRows()));
        }

        queue_start = 0; //The queue is currently at position 0
        queue_size = 0; //The queue has 0 elements
        buffer_read = 0; //Reading happens at element 0

        update_buffer_pointers();
    }
    
    /**
     * Pop first item of queue into nothing.
     * @return 
     */
    bool pop()
    {
        if (queue_size == 0)
        {
            return false;
        }
        
        //shrink the queue by 1
        queue_start = wrap_index(queue_start + 1);
        queue_size = queue_size - 1;

        return true;
    }

    /**
     * @brief Pop first item of queue into raw float array. Data is row major with ld = columns
     * @param dst
     * @return 
     */
    bool pop(float * dst)
    {
        if (queue_size == 0)
        {
            return false;
        }
        
        buffer[queue_start].raw_copy_to(dst);

        /*int w = buffer[0].getNumCols();
        int h = buffer[0].getNumRows();

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                dst[x + y * w] = buffer[queue_start].getValue(x, y);
            }
        }*/

        //shrink the queue by 1
        queue_start = wrap_index(queue_start + 1);
        queue_size = queue_size - 1;

        return true;
    }

    /**
     * @brief Pop first item in queue to destination matrix.
     * @param dst
     * @return 
     */
    bool pop(vectorized_matrix<T> & dst)
    {
        if (queue_size == 0)
        {
            return false;
        }

        dst.overwrite(buffer[queue_start]);

        //shrink the queue by 1
        queue_start = wrap_index(queue_start + 1);
        queue_size = queue_size - 1;

        return true;
    }

    /**
     * @brief pushes the current read matrix into the queue, sets the write matrix as new read matrix.
     */
    bool push()
    {
        int expected = queue_size;

        if (expected < queue_max_size)
        {
            if (queue_size.compare_exchange_strong(expected, expected + 1)) //Increase queue size if it did not change yet
            {
                buffer_read = wrap_index(buffer_read + 1); //Move to next read index
                update_buffer_pointers();

                return true;
            }
        }
        return false;
    }

    /**
     * @brief returns current writing position
     * @return 
     */
    vectorized_matrix<T> * buffer_write_ptr() const
    {
        return write_buffer;
    }

    /**
     * @brief returns current read position
     * @return 
     */
    vectorized_matrix<T> * buffer_read_ptr() const
    {
        return read_buffer;
    }

    /**
     * @brief If queue is empty. use with caution in parallel environment.
     * @return 
     */
    bool empty()
    {
        return queue_size == 0;
    }

    /**
     * @brief Returns size of queue. use with caution in parallel environment.
     */
    int size()
    {
        return queue_size;
    }
    
    /**
     * @brief Returns max. size of this queue.
     * @return 
     */
    int max_size()
    {
        return queue_max_size;
    }
    
    /**
     * @brief returns how many more items can be put into this queue.
     * @return 
     */
    int capacity_left()
    {
        return queue_max_size - queue_size;
    }

private:

    const int queue_max_size;
    vector<vectorized_matrix<T>> buffer;

    atomic<int> queue_start; //Start of the queue. Atomic as this can be changed by another thread
    atomic<int> queue_size; // Size of the queue. Atomic as this can be changed by another thread

    int buffer_read; //Position where the buffer is reading


    vectorized_matrix<T> * write_buffer = nullptr;
    vectorized_matrix<T> * read_buffer = nullptr;

    inline int wrap_index(int i)
    {
        return i % buffer.size();
    }

    void update_buffer_pointers()
    {
        write_buffer = &buffer[wrap_index(buffer_read + 1)];
        read_buffer = &buffer[buffer_read];
    }

};
