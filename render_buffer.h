#pragma once

#include <vector>
#include "matrix.h"

template <typename T>
/**
 * @brief Contains a n size buffer for matrices.
 * @author ruman
 */
class render_buffer
{
  public: 
  
    render_buffer(int size, int columns, int rows)
    {
      for(int i = 0; i < size; ++i)
      {
	buffer.push_back( vectorized_matrix<T>(columns, rows) );
      }
    }
    
    ~render_buffer()
    {
    }
    
    /**
     * @brief Store matrix into buffer
     * @param force If enabled, overwrite a buffer even if the accessor did not caught up yet
     * @return true if storing was successful
     */
    bool store(vectorized_matrix<T> & matrix, bool force)
    {
      int next_write_index = next_index(current_write);
      
      if(force || is_empty() || !read_write_collision(current_read, next_write_index))
      {
	current_write = next_write_index;
	
	return true;
      }
      
      return false;
    }
    
    /**
     * @brief returns current element without switching to the next element
     */
    vectorized_matrix<T> current()
    {
    }
    
    bool is_empty()
    {
      return buffer_size == 0;
    }
    
    /**
     * @brief checks if there is a next element
     */
    bool has_next()
    {
    }
    
    /**
     * @brief returns current element and switches to the next element if possible
     */
    vectorized_matrix<T> pop()
    {
    }
    
private:
  
  vector<vectorized_matrix<T>> buffer;
  int current_read = 0;
  int current_write = 0;
  int buffer_size = 0;
  
  int next_index(int index)
  {
    if(is_empty())
      return -1;
    else
      return (index + 1) % buffer_size;
  }
  
  bool read_write_collision(int read_index, int write_index)
  {
    return read_index == write_index;
  }
}