#pragma once
#include <vector>
#include <iostream>
#include <math.h>

using namespace std;

using cdouble = const double;
using cint = const int;

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
inline int matrix_calculate_ld(int size)
{
    return 64 * ceil(size / 64.0);
}

template <typename T>
/**
 * @brief A matrix with row first approach
 */
class matrix
{
private:
    vector<T> M;
    int rows;
    int columns;
    int ld;

public:
    /**
     * @brief creates an empty matrix
     */
    matrix()
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
    matrix(int columns, int rows)
    {
        int ld = matrix_calculate_ld(columns);

        this->M = vector<T>(ld * rows);
        this->rows = rows;
        this->columns = columns;
        this->ld = ld;
    }

    /**
     * @brief creates a deep copy of the given matrix
     * @param copy
     */
    matrix(const matrix<T> & copy)
    {
        M = vector<T>(copy.M);
        rows = copy.rows;
        columns = copy.columns;
        ld = copy.ld;
    }

    // Getter and Setter methods

    T getValue(cint x, cint y) const { return M[matrix_index(x,y,ld)]; }
    T getValueWrapped(cint x, cint y) const { return M[matrix_index_wrapped(x,y,columns,rows,ld)]; }

    void setValue(T val, cint x, cint y) { M[matrix_index(x,y,ld)] = val; }
    void setValueWrapped(T val, cint x, cint y) { M[matrix_index_wrapped(x,y,columns,rows,ld)] = val; }
    //TODO: setValueWrapped with dim? Should be avoided, i guess

    int getLd() const { return this->ld; }
    int getNumRows() const { return this->rows; }
    int getNumCols() const { return this->columns; }

    // Avanced Access Methods


    friend ostream& operator<<(ostream& out, const matrix<T> & m )
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

    void set_circle(cint x, cint y, cint r, const T v)
    {
        for(int i = 0; i < columns; ++i)
        {
            for(int j = 0; j < rows; ++j)
            {
                cdouble s_sq = (i-x)*(i-x) + (j-y)*(j-y);

                if(s_sq <= r*r)
                {
                    M[matrix_index(i,j,ld)] = v;
                }

                /*cdouble d = sqrt((i-x)*(i-x) + (j-y)*(j-y));

                if(d <= r)
                {
                    M[matrix_index(i,j,ld)] = v;
                }*/

                /*if(d <= r)
                {
                    // The point is in radius. No further actions needed
                    M[matrix_index(i,j,ld)] = v;
                }
                else if( floor(d) <= r)
                {
                    //Distance between next point in circle and the point outside circle
                    cdouble d_tf = d - floor(d);

                    M[matrix_index(i,j,ld)] = v * (1.0 - d_tf); //Works.
                }*/
            }
        }
    }

    void set_circle(cint r, const T v)
    {
        set_circle(columns / 2, rows / 2, r, v);
    }

    /**
     * @brief sum Calculates the sum of values
     * @return
     */
    double sum()
    {
        double s = 0;

        for(int i = 0; i < columns; ++i)
        {
            for(int j = 0; j < rows; ++j)
            {
                s += M[matrix_index(i,j,ld)];
            }
        }

        return s;
    }
};


template <typename T>
bool is_vectorized_matrix(matrix<T>& mat) {
    //TODO: implement this function here
    return false;
}
