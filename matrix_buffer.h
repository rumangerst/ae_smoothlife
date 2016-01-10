#pragma once

#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
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
        if (size <= 1)
        {
            cerr << "matrix_buffer should at least have size 2" << endl;
            exit(EXIT_FAILURE);
        }

        // Reserve buffer and put initial value (read value) into the buffer
        buffer.reserve(size);
        buffer.push_back(initial);
        for (int i = 0; i < size - 1; ++i)
        {
            //buffer.push_back(vectorized_matrix<T>(initial.getNumCols(), initial.getNumRows()));
            buffer.push_back(initial);
        }

        queue_start = 0;
        queue_size = 1;
        buffer_write = 1;
        
        update_buffer_pointers();

        spinlock_writing = false;
    }

    /**
     * @brief Returns copy of the current queue front and removes it. Program will terminate if pop() is used with an empty queue!
     * @return 
     */
    vectorized_matrix<T> queue_pop()
    {
        bool expected = false;
        while (spinlock_writing.compare_exchange_strong(expected, true))
        {
        } //Wait until we can write

        if (queue_size == 0)
        {
            cerr << "queue_pop() called on empty queue!" << endl;
            exit(EXIT_FAILURE);
        }

        //shrink the queue by 1
        queue_start = queue_start + 1;
        queue_size = queue_size - 1;
        
        //Free the lock
        spinlock_writing = false;

        return buffer[queue_start - 1];
    }

    void queue_pop_to(vectorized_matrix<T> & dst)
    {
        bool expected = false;
        while (spinlock_writing.compare_exchange_strong(expected, true))
        {
        } //Wait until we can write

        if (queue_size == 0)
        {
            cerr << "queue_pop() called on empty queue!" << endl;
            exit(EXIT_FAILURE);
        }
        
        dst.overwrite(buffer[queue_start]);

        //shrink the queue by 1
        queue_start = wrap_index(queue_start + 1);
        queue_size = queue_size - 1;
        
        //Free the lock
        spinlock_writing = false;
    }

    /**
     * @brief Returns the current queue size
     * @return 
     */
    int get_queue_size()
    {
        while (spinlock_writing.load())
        {
        } //Wait until some write operation finished

        return queue_size;
    }

    /**
     * @brief Tests if the buffer is writable.
     * @return true if buffer is writable
     */
    bool buffer_can_write_next()
    {
        while (spinlock_writing.load())
        {
        } //Wait until some write operation finished

        return wrap_index(buffer_write + 1) != queue_start;
    }

    /**
     * @brief Moves to next writing position if possible
     * @return nullptr if writing is not possible
     */
    vectorized_matrix<T> * buffer_next()
    {    
        bool expected = false;
            while (spinlock_writing.compare_exchange_strong(expected, true))
            {
            } //Wait until we can write
        
        if (wrap_index(buffer_write + 1) != queue_start)
        {            
            buffer_write = wrap_index(buffer_write + 1); //Move to next write index
            queue_size = queue_size + 1; //The queue now has one item more
            
            update_buffer_pointers();
            
            //Free the lock            
            spinlock_writing = false;

            return & buffer[buffer_write];
        }
        else
        {
            //Free the lock            
            spinlock_writing = false;
            
            return nullptr;
        }
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

private:

    vector<vectorized_matrix<T>> buffer;
    atomic<int> queue_start; //Start of the queue
    atomic<int> queue_size; // Size of the queue

    atomic<int> buffer_write; //Position where the buffer is writing

    //"Write" spinlock
    atomic<bool> spinlock_writing;
    
    vectorized_matrix<T> * write_buffer = nullptr;
    vectorized_matrix<T> * read_buffer = nullptr;

    inline int wrap_index(int i)
    {
        return i % buffer.size();
    }
    
    void update_buffer_pointers()
    {
        write_buffer = & buffer[buffer_write];
        read_buffer = & buffer[wrap_index(buffer_write - 1)];
    }

};