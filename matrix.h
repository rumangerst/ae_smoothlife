#pragma once
#include <vector>
#include <iostream>
#include <math.h>

using namespace std;

using cdouble = const double;
using cint = const int;

/**
 * @brief matrix_index
 * @param x
 * @param y
 * @param ld
 * @return The array index of matrix element x,y
 */
inline int matrix_index(cint x, cint y, cint ld)
{
    return x + ld * y;
}

/**
 * @brief matrix_index_wrapped
 * @param x
 * @param y
 * @param w
 * @param h
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
inline int calculate_ld(int size)
{
    return 64 * ceil(size / 64.0);
}

/**
 * @brief A matrix with row first approach
 */
struct matrix
{
    vector<double> M;
    int rows;
    int columns;
    int ld;

    matrix()
    {
        rows = 0;
        columns = 0;
        ld = 0;
    }

    matrix(double rows, double columns, int ld)
    {
        this->M = vector<double>(ld * rows);
        this->rows = rows;
        this->columns = columns;
    }

    matrix(const matrix & copy)
    {
        M = vector<double>(copy.M);
        rows = copy.rows;
        columns = copy.columns;
        ld = copy.ld;
    }

    friend ostream& operator<<(ostream& out, const matrix & m )
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
};

struct mask : public matrix
{
    double normalizer;
    int size;

    mask()
    {

    }

    mask(cint s) : matrix(s,s,calculate_ld(s))
    {
        normalizer = 0;
        size = s;

        cint r_squared = (s / 2) * (s / 2);

        for(int x = 0; x < s; ++x)
        {
            cint x2 = x - s / 2;

            for(int y = 0; y < s; ++y)
            {
                cint y2 = y - s / 2;


                if(x2*x2 + y2*y2 <= r_squared)
                {
                    M[matrix_index(x,y,s)] = 1.0;
                    ++normalizer;
                }
            }
        }
    }

    mask(const mask & copy) : matrix(copy)
    {
        size = copy.size;
        normalizer = copy.normalizer;
    }

    mask operator - (const mask & m) const
    {
        if(size < m.size)
        {
            cout << "Invalid arguments for mask operator -" << endl;
            exit(-1);
        }

        mask o(*this);
        o.normalizer = normalizer - m.normalizer;

        const int pos_begin = size / 2 - m.size / 2;
        const int pos_end = size / 2 + m.size / 2 + 1;

        for(int x =  pos_begin; x < pos_end ; ++x)
        {
            for(int y = pos_begin; y < pos_end; ++y)
            {
                o.M[matrix_index(x,y,size)] -= m.M[matrix_index(x - pos_begin, y - pos_begin, m.size)];
            }
        }

        return o;
    }


};
