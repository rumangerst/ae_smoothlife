// 26.12 by B: change to aligned vector
#pragma once
#include <vector>
#include <iostream>
#include <math.h>
#include "aligned_vector.h"
#include <assert.h>
#include "communication.h"

/*
 * DONE:
 * - building of (circular) masks
 * - capsulation
 * - alignment to cache line
 * - padded to fit cacheline size
 * - vectorized sum()
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
class aligned_matrix
{
private:
    aligned_vector<T> m_Mat __attribute__((aligned(ALIGNMENT))); // matrix stored as vector
    int m_rows;
    int m_columns;  // actual number of used columns
    int m_ld;       // leading dimension. Can be greater than columns
    int m_offset;
    int m_leftOffset; // number of elements left from (a circles) center
    int m_rightOffset; // number of elements right from (a circles) center

public:

    /**
     * @brief creates an empty matrix
     */
    aligned_matrix() :
        m_rows(0),
        m_columns(0),
        m_ld(0),
        m_offset(0),
        m_leftOffset(0),
        m_rightOffset(0)
    { }

    /**
     * @brief standard constructor
     * @param columns number of elements per column
     * @param rows number of rows
     */
    aligned_matrix(cint columns, cint rows) :
        m_ld(matrix_calc_ld_with_padding(sizeof (T), columns, CACHELINE_SIZE)),
        m_Mat(aligned_vector<T>(matrix_calc_ld_with_padding(sizeof (T), columns, CACHELINE_SIZE) * rows)),
        m_rows(rows),
        m_columns(columns),
        m_offset(0),
        m_leftOffset(ceil(columns / 2)),
        m_rightOffset(matrix_calc_ld_with_padding(sizeof (T), columns, CACHELINE_SIZE) - m_leftOffset)
    {
        assert((m_ld * sizeof (T)) % CACHELINE_SIZE == 0);
    }

    /**
     * @brief offset constructor. Is used to make circle masks with additional
     * offset to re-enable vectorization in hard cases
     * @param columns
     * @param rows
     * @param offset
     * @author Bastian
     */
    aligned_matrix(cint columns, cint rows, cint offset) :
        m_ld(CACHELINE_FLOATS * ceil(float(offset + columns) / CACHELINE_FLOATS)),
        m_Mat(aligned_vector<T>(CACHELINE_FLOATS * ceil(float(offset + columns) / CACHELINE_FLOATS) * rows)),
        m_columns(columns),
        m_rows(rows),
        m_offset(offset),
        m_leftOffset(ceil(columns / 2) + offset),
        m_rightOffset(CACHELINE_FLOATS * ceil(float(offset + columns) / CACHELINE_FLOATS) - m_leftOffset)
    {
        assert(rows > 0 && columns > 0 && offset >= 0 && offset <= CACHELINE_FLOATS);
        assert(m_ld * sizeof (T) % CACHELINE_SIZE == 0);
        assert(long(this->m_Mat.data()) % ALIGNMENT == 0);
        assert(m_leftOffset + m_rightOffset == m_ld);
    }

    /**
     * @brief creates a deep copy of the given matrix
     * @param copy
     */
    aligned_matrix(const aligned_matrix<T> & copy) :
        m_Mat(aligned_vector<T>(copy.m_Mat)),
        m_rows(copy.m_rows),
        m_columns(copy.m_columns),
        m_ld(copy.m_ld),
        m_offset(copy.m_offset),
        m_leftOffset(copy.m_leftOffset),
        m_rightOffset(copy.m_rightOffset)
    {}

    // Getter and Setter methods

    inline T getValue(cint x, cint y) const
    {
        return m_Mat[matrix_index(x, y, m_ld)];
    }

    inline T getValueWrapped(cint x, cint y) const
    {
        return m_Mat[matrix_index_wrapped(x, y, m_columns, m_rows, m_ld)];
    }
    
    /**
     * - uses ld als width instead of columns
     * - this is/was needed to test correctness of values inside vectorized code
     */
    inline T getValueWrappedLd(cint col, cint row) const
    {
        return m_Mat[matrix_index_wrapped(col, row, m_ld, m_rows, m_ld)];
    }
    
    inline T* getRow_ptr(int row)
    {
        return &m_Mat.data()[matrix_index(0, row, m_ld)];
    }

    inline const T* getRow_ptr(int row) const
    {
        return &m_Mat.data()[matrix_index(0, row, m_ld)];
    }

    inline const T* getValue_ptr(cint col, cint row) const
    {
        return &m_Mat.data()[matrix_index(col, row, m_ld)];
    }

    inline const T* getValueWrapped_ptr(cint col, cint row) const
    {
        return &m_Mat.data()[matrix_index_wrapped(col, row, m_columns, m_rows, m_ld)];
    }
    
    inline const T* getValueWrappedLd_ptr(cint col, cint row) const
    {
        return &m_Mat.data()[matrix_index_wrapped(col, row, m_ld, m_rows, m_ld)];
    }

    inline void setValue(T val, cint col, cint row)
    {
        m_Mat[matrix_index(col, row, m_ld)] = val;
    }

    inline void setValueWrapped(T val, cint col, cint row)
    {
        m_Mat[matrix_index_wrapped(col, row, m_columns, m_rows, m_ld)] = val;
    }

    const T * getValues() const
    {
        return m_Mat.data();
    }

    int getLd() const
    {
        return this->m_ld;
    }

    int getNumRows() const
    {
        return this->m_rows;
    }

    int getNumCols() const
    {
        return this->m_columns;
    }

    int getLeftOffset() const
    {
        return this->m_leftOffset;
    }

    int getRightOffset() const
    {
        return this->m_rightOffset;
    }

    // Advanced Access Methods

    friend ostream& operator<<(ostream& out, const aligned_matrix<T> & m)
    {
        for (int y = 0; y < m.m_columns; ++y)
        {
            for (int x = 0; x < m.m_rows; ++x)
            {
                out << m.m_Mat[matrix_index(x, y, m.m_ld)] << ",";
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

        for (int i = 0; i < m_ld; ++i)
        {
            for (int j = 0; j < m_rows; ++j)
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
        return sizeof (T) * m_ld;
    }

    /**
     * @brief Sets the matrix values in a circle around matrix center
     * @param r
     * @param v
     * @param smooth Will increase radius of the circle by this value
     */
    void set_circle(cdouble r, const T v, cdouble smooth_factor, cint offset)
    {
        set_circle(m_columns / 2.0, m_rows / 2.0, r, v, smooth_factor, offset);
    }

    /**
     * @brief sum accumulates all values of the matrix
     * @return
     */
    float sum() const {
        float s = 0;

        for (int i = 0; i < m_columns; ++i)
        {
            //cint I = i*m_ld;
            cint I = m_offset + i*m_ld;
            for (int j = 0; j < m_rows; ++j)
            {
                s += m_Mat[j + I];
            }
        }

        return s;
    }
    
    /**
     * @brief: prints the given matrix with '0' and 'x' letters to the console
     * - 0 will be printed, if any value is exactly zero, 'x' otherwise
     * - this is used to make basic checks for the circlular masks
     */
    void print_to_console() const {
        cout << "matrix print:\n";
        for (int y = 0; y < this->m_rows; ++y)
        {
            for (int x = 0; x < this->m_ld; ++x)
                printf((x < m_offset) ? "-" : ((this->getValue(x, y) == 0) ? "0" : "x"), "  ");
            printf("\n");
        }
        cout.flush();
    }
    
    /**
     * @brief: prints some basic informaton about this matrix to the console
     */
    void print_info() const {
        cout << "rows: " << m_rows << "  cols: " << m_columns << "  ld: " << m_ld << endl;
        cout << "left: " << m_leftOffset << "  right: " << m_rightOffset << "  off: " << m_offset << endl;
        cout.flush();
    }

    /**
     * @brief Overwrites the data in this matrix with data from src.
     * @param src must have the same size as this matrix, otherwise the program will terminate!
     */
    void overwrite(aligned_matrix<T> & src)
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
     * @param ptr the source
     * @param src_x_start slice start x coordinate (column) in source
     * @param dst_x_start slice start x coordinate (column) in destination (this matrix)
     * @param w slice width
     * @param src_column column count of source
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
        raw_overwrite(ptr, 0, x_start, w, w);
        
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
bool is_vectorized_matrix(aligned_matrix<T>* mat)
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
