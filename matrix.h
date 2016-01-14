// 26.12 by B: change to aligned vector
#pragma once
#include <vector>
#include <iostream>
#include <math.h>
#include "aligned_vector.h"
#include <assert.h>
#include "mpi_manager.h"

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
 * @author Ruman
 */
inline int matrix_index_wrapped(cint x, cint y, cint w, cint h, cint ld)
{
    return matrix_index(x >= 0 ? x % w : (w + x) % w, y >= 0 ? y % h : (h + y) % h, ld);
}

/**
 * @brief Calculates the ideal ld to 64 byte alignment
 * @param size
 * @author Bastian
 * @return
 */
inline int matrix_calc_ld_with_padding(cint typesize, cint size, cint cacheline_size)
{
    return cacheline_size * ceil(float(size * typesize)/cacheline_size);
}

template <typename T>
/**
 * @brief A matrix with row first approach
 * - this is basically a data container to keep all important information 
 * - concept: together
 * - vectorization updates: Bastian
 */
class vectorized_matrix
{
private:
    aligned_vector<T> M __attribute__((aligned(ALIGNMENT)));
    int rows;
    int columns;
    int ld;
    int offset;
    int leftOffset; // number of elements left from (a circles) center
    int rightOffset; // number of elements right from (a circles) center

public:

    /**
     * @brief creates an empty matrix
     */
    vectorized_matrix()
    {
        rows = 0;
        columns = 0;
        ld = 0;
        offset = 0;
        leftOffset = 0;
        rightOffset = 0;
    }

    /**
     * @brief standard constructor
     * @param columns number of elements per column
     * @param rows number of rows
     */
    vectorized_matrix(cint columns, cint rows)
    {
        cint ld = matrix_calc_ld_with_padding(sizeof (T), columns, CACHELINE_SIZE);
        assert((ld * sizeof (T)) % CACHELINE_SIZE == 0);

        this->M = aligned_vector<T>(ld * rows);
        this->rows = rows;
        this->columns = columns;
        this->ld = ld;
        this->offset = 0;
        this->leftOffset = columns / 2;
        this->rightOffset = ld - leftOffset;
    }

    /**
     * @brief offset constructor. Is used to make circle masks with additional
     * offset to re-enable vectorization in hard cases
     * @param columns
     * @param rows
     * @param offset
     * @author Bastian
     */
    vectorized_matrix(cint columns, cint rows, cint offset)
    {
        assert(rows > 0 && columns > 0 && offset >= 0 && offset <= CACHELINE_FLOATS);
        this->ld = CACHELINE_FLOATS * ceil(float(offset + columns) / CACHELINE_FLOATS);
        assert(ld * sizeof (T) % CACHELINE_SIZE == 0);
        this->M = aligned_vector<T>(ld * rows);
        assert(long(this->M.data()) % ALIGNMENT == 0);
        this->columns = columns;
        this->rows = rows; // the actual number of rows potentially containing information
        this->offset = offset;
        this->leftOffset = ceil(columns / 2) + offset; // will be +1 of the last, accessible index!
        this->rightOffset = ld - leftOffset;
        assert(leftOffset + rightOffset == ld);
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
        offset = copy.offset;
        leftOffset = copy.leftOffset;
        rightOffset = copy.rightOffset;
    }

    // Getter and Setter methods

    inline T getValue(cint x, cint y) const
    {
        return M[matrix_index(x, y, ld)];
    }

    inline T getValueWrapped(cint x, cint y) const
    {
        return M[matrix_index_wrapped(x, y, columns, rows, ld)];
    }
    
    inline T* getRow_ptr(int y)
    {
        return &M.data()[matrix_index(0, y, ld)];
    }

    inline const T* getRow_ptr(int y) const
    {
        return &M.data()[matrix_index(0, y, ld)];
    }

    inline const T* getValue_ptr(cint x, cint y) const
    {
        return &M.data()[matrix_index(x, y, ld)];
    }

    inline const T* getValueWrapped_ptr(cint x, cint y) const
    {
        return &M.data()[matrix_index_wrapped(x, y, columns, rows, ld)];
    }

    inline void setValue(T val, cint x, cint y)
    {
        M[matrix_index(x, y, ld)] = val;
    }

    inline void setValueWrapped(T val, cint x, cint y)
    {
        M[matrix_index_wrapped(x, y, columns, rows, ld)] = val;
    }

    const T * getValues() const
    {
        return M.data();
    }

    int getLd() const
    {
        return this->ld;
    }

    int getNumRows() const
    {
        return this->rows;
    }

    int getNumCols() const
    {
        return this->columns;
    }

    int getLeftOffset() const
    {
        return this->leftOffset;
    }

    int getRightOffset() const
    {
        return this->rightOffset;
    }

    // Advanced Access Methods

    friend ostream& operator<<(ostream& out, const vectorized_matrix<T> & m)
    {
        for (int y = 0; y < m.columns; ++y)
        {
            for (int x = 0; x < m.rows; ++x)
            {
                out << m.M[matrix_index(x, y, m.ld)] << ",";
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
     * @param offset the number of units added to the left boundary before drawing the circle begins
     * @author Ruman
     */
    void set_circle(cdouble center_x, cdouble center_y, cdouble r, const T v, cdouble smooth_factor, cint offset)
    {
        assert(offset >= 0);
        cdouble c_x = center_x + offset; // push it to the right
        cdouble c_y = center_y;

        for (int i = 0; i < ld; ++i)
        {
            for (int j = 0; j < rows; ++j)
            {
                cdouble local_x = i + 0.5;
                cdouble local_y = j + 0.5;

                cdouble d = sqrt((local_x - c_x)*(local_x - c_x) + (local_y - c_y)*(local_y - c_y));

                /*
                 * Calculate the interpolation weight with 
                 * 
                 * w(d) = max(0, min(1, (-d + r + bb) / bb))
                 * 
                 * We calcuate this with d (distance from center point). 
                 * The author of reference SmoothLife implementation does exactly the same, but in an other way
                 */

                cdouble w = smooth_factor <= 0 ? (d <= r ? 1.0 : 0.0) : 1.0 / smooth_factor * (-d + r + smooth_factor);
                cdouble weight = fmax(0, fmin(1, w));
                T src_value = getValue(i, j);
                T dst_value = src_value * (1.0 - weight) + v * weight;

                setValue(dst_value, i, j);
            }
        }
    }

    inline int getNumBytesPerRow()
    {
        return sizeof (T) * ld;
    }

    /**
     * @brief Sets the matrix values in a circle around matrix center
     * @param r
     * @param v
     * @param smooth Will increase radius of the circle by this value
     */
    void set_circle(cdouble r, const T v, cdouble smooth_factor, cint offset)
    {
        set_circle(columns / 2.0, rows / 2.0, r, v, smooth_factor, offset);
    }

    /**
     * @brief sum Calculates the sum of values
     * @return
     */
    float sum() const {
        float s = 0;

        for (int i = 0; i < columns; ++i)
        {
            cint I = i*ld;
#pragma omp simd
#pragma vector aligned
            for (int j = 0; j < rows; ++j)
            {
                s += M[j + I];
            }
        }

        return s;
    }
    
    void print_to_console() const {
        cout << "matrix print:\n";
        for (int y = 0; y < this->rows; ++y)
        {
            for (int x = 0; x < this->ld; ++x)
                printf((x < offset) ? "-" : ((this->getValue(x, y) == 0) ? "0" : "x"), "  ");
            printf("\n");
        }
        cout.flush();
    }
    
    void print_info() const {
        cout << "rows: " << rows << "  cols: " << columns << "  ld: " << ld << endl;
        cout << "left: " << leftOffset << "  right: " << rightOffset << "  off: " << offset << endl;
        cout.flush();
    }

    /**
     * @brief Overwrites the data in this matrix with data from src.
     * @param src must have the same size as this matrix, otherwise the program will terminate!
     */
    void overwrite(vectorized_matrix<T> & src)
    {
        if (src.getNumCols() != getNumCols() || src.getNumRows() != getNumRows())
        {
            cerr << "Cannot overwrite matrix from matrix with different size!"<<endl;
            cerr << "From rank" << mpi_rank() << endl;
            cerr << src.getNumCols() << "<>" << getNumCols() << " or " << src.getNumRows() << "<>" << getNumRows() << endl;
            exit(EXIT_FAILURE);
        }

        for (int y = 0; y < getNumRows(); ++y)
        {
            for (int x = 0; x < getNumCols(); ++x)
            {
                setValue(src.getValue(x,y),x,y);
            }
        }

    }
    
    /**
     * @brief Copies a horizontal slice of this matrix into ptr.
     * @param data
     * @author Ruman
     */
    void raw_copy_to(T * ptr, int x_start, int w)
    {
        for(int y = 0; y < getNumRows();++y)
        {
            for(int x = x_start; x < x_start + w;++x)
            {
                ptr[matrix_index(x - x_start,y,w)] = getValue(x,y);
            }
        }
    }
    
    /**
     * @brief Overwrites a horizontal slice of this matrix with a horizontal slice of a raw matrix with same row count.
     */
    void raw_overwrite(T * ptr, int src_x_start, int dst_x_start, int w, int src_columns)
    {
        for(int y = 0; y < getNumRows(); ++y)
        {
            T * src_row = &ptr[y * src_columns];
            T * dst_row = getRow_ptr(y);
            
            for(int x = 0; x < w; ++x)
            {
                dst_row[x + dst_x_start] = src_row[x + src_x_start];
            }
        }
    }
    
    /**
     * @brief Overwrites horizontal slice of this matrix with a raw matrix.
     * @param ptr
     * @author Ruman
     */
    void raw_overwrite(T * ptr, int x_start, int w)
    {
        raw_overwrite(ptr, x_start, 0, w, w);
        
        /*for(int y = 0; y < getNumRows();++y)
        {
            for(int x = x_start; x < x_start + w;++x)
            {
                setValue(ptr[matrix_index(x - x_start,y,w)],x,y);
            }
        }*/
    }
    
    /**
     * @brief Writes all data from this matrix into a raw matrix
     * @param ptr
     */
    void raw_copy_to(T * ptr)
    {
        raw_copy_to(ptr, 0, getNumCols());
    }
    
    /**
     * @brief Overwrites all data in this matrix with a raw matrix.
     * @param ptr
     */
    void raw_overwrite(T * ptr)
    {
        raw_overwrite(ptr, 0, getNumCols());
    }
};

template <typename T>
/**
 * @brief return TRUE, if the given matrix is optimized for vectorization
 */
bool is_vectorized_matrix(vectorized_matrix<T>* mat)
{

    if (mat == NULL)
    {
        printf("mat is NULL!");
        return false;
    }

    if (mat % ALIGNMENT != 0)
    {
        printf("mat is not aligned!");
        return false;
    }

    if (mat->getNumBytesPerRow() % CACHELINE_SIZE != 0)
    {
        printf("leading dimension of mat does not fit to cacheline size!");
        return false;
    }

    return true;
}
