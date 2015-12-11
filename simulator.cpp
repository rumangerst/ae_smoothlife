#include "simulator.h"

simulator::simulator(const ruleset & r) : rules(r)
{    

}

simulator::~simulator()
{
    if(initialized)
    {
        delete space_current;
        delete space_next;
    }
}

void simulator::initialize()
{
    cout << "Initializing ..." << endl;

    space_current = new matrix<double>(field_size_x, field_size_y, field_ld);
    space_next = new matrix<double>(field_size_x, field_size_y, field_ld);
    space_current_atomic.store(space_current);

    m_mask = matrix<double>(rules.ri * 2, rules.ri * 2);
    n_mask = matrix<double>(rules.ra * 2, rules.ra * 2);

    m_mask.set_circle(rules.ri, 1);
    n_mask.set_circle(rules.ra, 1);
    n_mask.set_circle(rules.ri, 0);

    m_mask_sum = m_mask.sum();
    n_mask_sum = n_mask.sum();

    //initialize_field_propagate();
    //initialize_field_random();
    //initialize_field_1();
    if(INITIALIZE_FIELD)
     initialize_field_splat();

    initialized = true;
}

void simulator::simulate()
{
    cout << "Running ..." << endl;

    running = ENABLE_SIMULATION;

    while(running)
    {
        for(int x = 0; x < field_size_x; ++x)
        {
            for(int y = 0; y < field_size_y; ++y)
            {
                double n = filling(x, y, n_mask, n_mask_sum);
                double m = filling(x,y,m_mask,m_mask_sum);

                //Calculate the new state based on fillings n and m
                //Smooth state function must be clamped to [0,1] (this is also done by author's implementation!)
                space_next->M[matrix_index(x,y,field_ld)] = rules.discrete ? s(n,m) : fmax(0,fmin(1,f(x,y,n,m)));

            }
        }

        //Swap fields
        std::swap(space_current, space_next);

        //Tell outside
        space_current_atomic.store(space_current);

        ++time;
    }

    //Print the two masks
    /*for(int x = 0; x < m_mask.columns; ++x)
    {
        for(int y = 0; y < m_mask.rows; ++y)
        {
            space_current->M[matrix_index(x,y,space_current->ld)] = m_mask.M[matrix_index(x,y,m_mask.ld)];
        }
    }
    for(int x = 0; x < n_mask.columns; ++x)
    {
        for(int y = 0; y < n_mask.rows; ++y)
        {
            space_current->M[matrix_index(x + m_mask.columns + 10,y + m_mask.rows + 10,space_current->ld)] = n_mask.M[matrix_index(x,y,n_mask.ld)];
        }
    }
     space_current_atomic.store(space_current);*/


    //Test the colors
    /*for(int x = 0; x < field_size_x; ++x)
    {
        for(int y = 0; y < field_size_y; ++y)
        {
            double m = (double)y / field_size_y;
            double n = (double)x / field_size_x;

            double &  value = space_current->M[matrix_index(x,y,field_ld)];
            value = (n + m) / 2.0;
        }
    }*/

    //Test to print s(n,m)
    /*for(int x = 0; x < field_size_x; ++x)
    {
        for(int y = 0; y < field_size_y; ++y)
        {
            double m = (double)y / field_size_y;
            double n = (double)x / field_size_x;

            double &  value = space_current->M[matrix_index(x,y,field_ld)];
            value = s(n,m);
        }
    }*/
}

double simulator::filling(cint x, cint y, const matrix<double> &m, cdouble m_sum)
{
    // The theorectically considered bondaries
    cint x_begin = x - m.rows / 2;
    cint x_end = x + m.rows / 2;
    cint y_begin = y - m.rows / 2;
    cint y_end = y + m.rows / 2;

    double f = 0;

    if(x_begin >= 0 && y_begin >= 0 && x_end < field_size_x && y_end < field_size_y)
    {
        for(int y = y_begin; y < y_end; ++y)
        {
            #pragma omp simd
            for(int x = x_begin; x < x_end; ++x)
            {
                f += space_current->M[matrix_index(x,y,field_ld)] * m.M[matrix_index(x - x_begin, y - y_begin, m.ld)];
            }
        }
    }
    else
    {
        for(int y = y_begin; y < y_end; ++y)
        {
            for(int x = x_begin; x < x_end; ++x)
            {
                f += space_current->M[matrix_index_wrapped(x,y,field_size_x,field_size_y,field_ld)] * m.M[matrix_index(x - x_begin, y - y_begin, m.ld)];
            }
        }
    }


    return f / m_sum;
}
