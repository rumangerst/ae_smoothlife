#pragma once

/**
 * @brief Contains the rules used for simulation
 */
class ruleset
{
public:
    double ra;
    double ri;
    double b1;
    double b2;
    double d1;
    double d2;
    double alpha_m;
    double alpha_n;
    bool discrete;
    double dt;

    ruleset(double ra, double ri, double b1, double b2, double d1, double d2, double alpha_m, double alpha_n, double dt, bool discrete)
    {
        this->ra = ra;
        this->ri = ri;
        this->b1 = b1;
        this->b2 = b2;
        this->d1 = d1;
        this->d2 = d2;
        this->alpha_m = alpha_m;
        this->alpha_n = alpha_n;
        this->dt = dt;
        this->discrete = discrete;
    }

protected:
    ruleset()
    {

    }

};

/**
 * @brief Smooth Life L ruleset
 */
class ruleset_smooth_life_l : public ruleset
{
public:
    ruleset_smooth_life_l()
    {
         ra = 10;
         ri = 3;
         b1 = 0.257;
         b2 = 0.336;
         d1 = 0.365;
         d2 = 0.549;
         alpha_m = 0.147;
         alpha_n = 0.028;
         discrete = false;
         dt = 0.1;
    }
};

/**
 * @brief Ruleset suggested in paper by S. Rafler
 */
class ruleset_rafler_paper : public ruleset
{
public:
    ruleset_rafler_paper()
    {
        alpha_n = 0.028;
        alpha_m = 0.147;
        ra = 21;
        ri = 7;
        b1 = 0.278;
        b2 = 0.365;
        d1 = 0.267;
        d2 = 0.445;
        discrete = false;
        dt =0.1;
    }
};
