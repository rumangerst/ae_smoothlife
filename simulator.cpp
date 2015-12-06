#include "simulator.h"

simulator::simulator(const ruleset & r) : rules(r)
{    

}

void simulator::initialize()
{
    cout << "Initializing ..." << endl;

    field = matrix(field_size_x, field_size_y, field_ld);
    m_mask = mask(rules.ri);
    n_mask = mask(rules.ra) - m_mask;

    //initialize_field_propagate();
    //initialize_field_random();
    //initialize_field_1();
    initialize_field_splat();
}

void simulator::simulate()
{
    cout << "Running ..." << endl;

    running = true;

    while(running)
    {
        for(int x = 0; x < field_size_x; ++x)
        {
            for(int y = 0; y < field_size_y; ++y)
            {
                double n = filling(field, x, y, n_mask);
                double m = filling(field,x,y,m_mask);

                //Calculate the new state based on fillings n and m
                //Smooth state function must be clamped to [0,1] (this is also done by author's implementation!)
                field.M[matrix_index(x,y,field_ld)] = rules.discrete ? s(n,m) : fmax(0,fmin(1,f(x,y,n,m)));

            }
        }
    }

    //Test to print s(n,m)
    /*for(int x = 0; x < field_size_x; ++x)
    {
        for(int y = 0; y < field_size_y; ++y)
        {
            double m = (double)y / field_size_y;
            double n = (double)x / field_size_x;

            double &  value = field.M[matrix_index(x,y,field_ld)];
            value = s(n,m);
        }
    }*/
}

double simulator::filling(const matrix &field, cint x, cint y, const mask &m)
{
    // The theorectically considered bondaries
    cint x_begin = x - m.size / 2;
    cint x_end = x + m.size / 2;
    cint y_begin = y - m.size / 2;
    cint y_end = y + m.size / 2;

    double f = 0;

    if(x_begin >= 0 && y_begin >= 0 && x_end < field_size_x && y_end < field_size_y)
    {
        for(int y = y_begin; y < y_end; ++y)
        {
            #pragma omp simd
            for(int x = x_begin; x < x_end; ++x)
            {
                f += field.M[matrix_index(x,y,field_ld)] * m.M[matrix_index(x - x_begin, y - y_begin, m.size)];
            }
        }
    }
    else
    {
        for(int y = y_begin; y < y_end; ++y)
        {
            for(int x = x_begin; x < x_end; ++x)
            {
                f += field.M[matrix_index_wrapped(x,y,field_size_x,field_size_y,field_ld)] * m.M[matrix_index(x - x_begin, y - y_begin, m.size)];
            }
        }
    }


    return f / m.normalizer;
}
