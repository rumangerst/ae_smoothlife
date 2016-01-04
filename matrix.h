// 26.12 by B: change to aligned vector
#pragma once
#include <vector>
#include <iostream>
#include <math.h>
#include "aligned_vector.h"

/*
 * DONE:
 * - building of (circular) masks
 * - capsulation
 * - alignment to cache line
 * - padded to fit cacheline size
 * - vectorized sum()
 */

/*
 * TODO: Vectorization
 * 1. apply alignment tests (function already available)
 *
 * TODO: circle
 * 1. smoothing is half done (outer ring needs "trapez" smoothing)
 */

using namespace std;

using cfloat = const float;
using cdouble = const double;
using cint = const int;
using clong = const long;

/**
 * @brief matrix_index, 0 <= x < ld , 0 <= y
 * row-major
 * @param x column index
 * @param y row index
 * @param ld number of elements in a row
 * @return The array index of matrix element x,y
 */
inline int matrix_index(cint x, cint y, cint ld)
{
    return x + ld * y;
}

/**
 * @brief matrix_index_wrapped
 * folds x and y back into the matrix, when these are out of bounds
 * x and y can be negative!
 * @Note may be useful for testing
 * @Note may not be used for full optimization? TODO
 * @param x column index
 * @param y row index
 * @param ld number of elements in a row
 * @param w width of the matrix, i.e. number of columns
 * @param h height of the matrix, i.e. number of rows
 * @return Wrapped matrix index
 */
inline int matrix_index_wrapped(cint x, cint y, cint w, cint h, cint ld)
{
    return matrix_index(x >= 0 ? x % w : (w + x) % w, y >= 0 ? y % h : (h + y) % h, ld);
}

/**
 * @brief Calculates the ideal ld to 64 byte alignment
 * @param size
 * @return
 */
inline int matrix_calc_ld_with_padding(const int size, const int cachline_size)
{
    return cachline_size * ceil(float(size) / cachline_size);
}

template <typename T>
/**
 * @brief A matrix with row first approach
 */
class vectorized_matrix
{
private:
    aligned_vector<T> M __attribute__((aligned(64)));
    int rows;
    int columns;
    int ld;

public:
    /**
     * @brief creates an empty matrix
     */
    vectorized_matrix()
    {
        rows = 0;
        columns = 0;
        ld = 0;
    }

    /**
     * @brief standard constructor
     * @param columns number of elements per column
     * @param rows number of rows
     */
    vectorized_matrix(int columns, int rows)
    {
        int ld = matrix_calc_ld_with_padding(columns, CACHELINE_SIZE);

        this->M = aligned_vector<T>(ld * rows);
        this->rows = rows;
        this->columns = columns;
        this->ld = ld;
    }

    /**
     * @brief creates a deep copy of the given matrix
     * @param copy
     */
    vectorized_matrix(const vectorized_matrix<T> & copy)
    {
        M = aligned_vector<T>(copy.M);
        rows = copy.rows;
        columns = copy.columns;
        ld = copy.ld;
    }

    // Getter and Setter methods

    T getValue(cint x, cint y) const {
      #pragma vector aligned	// needed for vectorization
      return M[matrix_index(x,y,ld)]; 
    }

    T getValueWrapped(cint x, cint y) const {
      #pragma vector aligned	// needed for vectorization
      return M[matrix_index_wrapped(x,y,columns,rows,ld)];
    }

    void setValue(T val, cint x, cint y) { 
      #pragma vector aligned
      M[matrix_index(x,y,ld)] = val; 
    }
    
    void setValueWrapped(T val, cint x, cint y) { 
      #pragma vector aligned
      M[matrix_index_wrapped(x,y,columns,rows,ld)] = val; 
    }
    //TODO: setValueWrapped with dim? Should be avoided, i guess

    int getLd() const { return this->ld; }
    int getNumRows() const { return this->rows; }
    int getNumCols() const { return this->columns; }

    // Avanced Access Methods

    friend ostream& operator<<(ostream& out, const vectorized_matrix<T> & m )
    {
        for(int y = 0; y < m.columns; ++y)
        {
            for(int x = 0; x < m.rows; ++x)
            {
                out << m.M[matrix_index(x,y,m.ld)] << ",";
            }
            out << endl;
        }

        return out;
    }

    /**
     * @brief Sets the matrix values in a circle around given center
     * @param x x coordinate (double). The center x of the matrix is columns / 2.0
     * @param y y coordinate (double). The center y of the matrix is rows / 2.0
     * @param r radius of the circle
     * @param v the value the elements in the circle should be set to.
     * @param smooth_factor smooth the circle. Set it to 0 to disable smoothing. Will increase radius of the circle by this value
     */
    void set_circle(cdouble center_x, cdouble center_y, cdouble r, const T v, cdouble smooth_factor)
    {
        for(int i = 0; i < columns; ++i)
        {
            for(int j = 0; j < rows; ++j)
            {
                cdouble local_x = i + 0.5;
                cdouble local_y = j + 0.5;
		
		cdouble d = sqrt((local_x-center_x)*(local_x-center_x) + (local_y-center_y)*(local_y-center_y));
		
		/*
		 * Calculate the interpolation weight with 
		 * 
		 * w(d) = max(0, min(1, (-d + r + bb) / bb))
		 * 
		 * We calcuate this with d (distance from center point). 
		 * The author of reference SmoothLife implementation does exactly the same, but in an other way
		 */
		
		cdouble w = smooth_factor <= 0 ? ( d <= r ? 1.0 : 0.0 ) : 1.0 / smooth_factor * (-d + r + smooth_factor);
		cdouble weight = fmax(0, fmin(1, w));
		T src_value = getValue(i,j);
		T dst_value = src_value * (1.0 - weight) + v * weight;
		
		setValue(dst_value, i, j);
            }
        }
    }


    int getNumBytesPerRow() {
        return sizeof(T)*ld;
    }

    /**
     * @brief Sets the matrix values in a circle around matrix center
     * @param r
     * @param v
     * @param smooth Will increase radius of the circle by this value
     */
    void set_circle(cdouble r, const T v, cdouble smooth_factor)
    {
        set_circle(columns / 2.0, rows / 2.0, r, v, smooth_factor);
    }

    /**
     * @brief sum Calculates the sum of values
     * @return
     */
    float sum() {
        float s = 0;

        for(int i = 0; i < columns; ++i) {
            cint I = i*ld;
            #pragma omp simd
            #pragma vector aligned
            for(int j = 0; j < rows; ++j) {
                s += M[j + I];
            }
        }

        return s;
    }
};


template <typename T>
/**
 * @brief return TRUE, if the given matrix is optimized for vectorization
 */
bool is_vectorized_matrix(vectorized_matrix<T>* mat) {

    if (mat == NULL) {
        printf("mat is NULL!");
        return false;
    }

    if (mat % ALIGNMENT != 0) {
        printf("mat is not aligned!");
        return false;
    }

    if (mat->getNumBytesPerRow() % CACHELINE_SIZE != 0) {
        printf("leading dimension of mat does not fit to cacheline size!");
        return false;
    }

    return true;
}
