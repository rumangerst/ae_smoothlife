#pragma once
#include <iostream>
#include <math.h>
#include <vector>
#include <random>
#include "matrix.h"
#include "ruleset.h"

using namespace std;

#define FIELD_SIZE 256

class simulator
{
public:
    simulator(const ruleset & rules);

    const int field_size_x = FIELD_SIZE;
    const int field_size_y = FIELD_SIZE;
    const int field_ld = FIELD_SIZE;

    const ruleset rules;

    matrix field;
    mask m_mask;
    mask n_mask;

    bool running = false;

    void initialize();
    void simulate();

private:

    void initialize_field_1()
    {

        for(int x = 0; x < field_size_x; ++x)
        {
            for(int y = 0; y < field_size_y; ++y)
            {
                field.M[matrix_index(x,y,field_ld)] = 1;
            }
        }
    }

    void initialize_field_random()
    {
        random_device rd;
        default_random_engine re(rd());
        uniform_real_distribution<double> random_state(0,1);

        for(int x = 0; x < field_size_x; ++x)
        {
            for(int y = 0; y < field_size_y; ++y)
            {
                field.M[matrix_index(x,y,field_ld)] = random_state(re);
            }
        }
    }

    /**
     * @brief initialize_field_splat Taken from reference implementation to generate "splats"
     */
    void initialize_field_splat()
    {
        random_device rd;
        default_random_engine re(rd());

        uniform_real_distribution<double> random_idk(0,0.5);
        uniform_real_distribution<double> random_point_x(0,field_size_x);
        uniform_real_distribution<double> random_point_y(0,field_size_y);

        double mx, my;

        mx = 2*rules.ra; if (mx>field_size_x) mx=field_size_x;
        my = 2*rules.ra; if (my>field_size_y) my=field_size_y;

        for(int t=0; t<=(int)(field_size_x*field_size_y/(mx*my)); ++t)
        {
            double mx, my, dx, dy, u, l;
            int ix, iy;

            mx = random_point_x(re);
            my = random_point_y(re);
            u = rules.ra*(random_idk(re) + 0.5);

            for (iy=(int)(my-u-1); iy<=(int)(my+u+1); ++iy)
                for (ix=(int)(mx-u-1); ix<=(int)(mx+u+1); ++ix)
                {
                    dx = mx-ix;
                    dy = my-iy;
                    l = sqrt(dx*dx+dy*dy);
                    if (l<u)
                    {
                        int px = ix;
                        int py = iy;
                        while (px<  0) px+=field_size_x;
                        while (px>=field_size_x) px-=field_size_x;
                        while (py<  0) py+=field_size_y;
                        while (py>=field_size_y) py-=field_size_y;
                        if (px>=0 && px<field_size_x && py>=0 && py<field_size_y)
                        {
                            field.M[matrix_index(px,py,field_ld)] = 1.0;
                        }
                    }
                }
        }
    }

    void initialize_field_propagate()
    {
        random_device rd;
        default_random_engine re(rd());

        uniform_real_distribution<double> random_state(0,1);

        const double p_seed = 0.01;
        const double p_proagate = 0.3;

        //Seed
        for(int x = 0; x < field_size_x; ++x)
        {
            for(int y = 0; y < field_size_y; ++y)
            {
                field.M[matrix_index(x,y,field_ld)] = random_state(re) <= p_seed ? 1 : 0;
            }
        }

        //Propagate
        for(int i = 0; i < 5; ++i)
        {
            for(int x = 0; x < field_size_x; ++x)
            {
                for(int y = 0; y < field_size_y; ++y)
                {
                    double f = field.M[matrix_index(x,y,field_ld)];

                    if(f > 0.5)
                    {
                        if(random_state(re) <= p_proagate)
                            field.M[matrix_index_wrapped(x - 1,y,field_size_x, field_size_y,field_ld)] = 1;
                        if(random_state(re) <= p_proagate)
                            field.M[matrix_index_wrapped(x + 1,y,field_size_x, field_size_y,field_ld)] = 1;
                        if(random_state(re) <= p_proagate)
                            field.M[matrix_index_wrapped(x,y - 1,field_size_x, field_size_y,field_ld)] = 1;
                        if(random_state(re) <= p_proagate)
                            field.M[matrix_index_wrapped(x,y + 1,field_size_x, field_size_y,field_ld)] = 1;
                    }
                }
            }
        }
    }

    inline double sigma1(cdouble x,cdouble a, cdouble alpha)
    {
        return 1.0 / ( 1 + exp(-(x-a)*4.0/alpha));
    }

    inline double sigma2(cdouble x,cdouble a,cdouble b, cdouble alpha)
    {
        return sigma1(x,a,alpha) * ( 1.0 - sigma1(x,b,alpha));
    }

    inline double sigmam(cdouble x,cdouble y,cdouble m, cdouble alpha)
    {
        return x * ( 1.0 - sigma1(m,0.5, alpha)) + y*sigma1(m, 0.5, alpha);
    }

    /**
     * @brief s Calculates the new state
     * @param n Outer filling
     * @param m Inner Filling
     * @return New state
     */
    inline double s(cdouble n, cdouble m)
    {
        return sigma2(n, sigmam(rules.b1,rules.d1,m, rules.alpha_m),sigmam(rules.b2,rules.d2,m, rules.alpha_m), rules.alpha_n);
    }

    inline double ss(cdouble n, cdouble m)
    {
        return 2.0 * s(n,m) - 1.0;
    }

    inline double f(cint x, cint y, cdouble n, cdouble m)
    {
        cdouble v = field.M[matrix_index(x,y, field_ld)];

        return v + rules.dt * ss(n,m);
    }

    double filling(const matrix & field, cint x, cint y, const mask & m);
};
