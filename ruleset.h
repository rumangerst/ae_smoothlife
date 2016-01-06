#pragma once
#include <string>
#include <iostream>

#define RULESET_DEFAULT_SPACE_W 256
#define RULESET_DEFAULT_SPACE_H 256

/**
 * @brief Contains the rules used for simulation
 */
class ruleset
{
public: 
    ruleset(int width, int height, float ra, float rr, float b1, float b2, float d1, float d2, float alpha_m, float alpha_n, float dt, bool discrete)
    {
        if(width <= 0 || height <= 0)
        {
            cerr << "Provided invalid space size" << endl;
            exit(-1);
        }
    
        space_width = width;
        space_height = height;
        
        set_ra(ra);
        set_rr(rr);
        set_b1(b1);
        set_b2(b2);
        set_d1(d1);
        set_d2(d2);
        set_alpha_m(alpha_m);
        set_alpha_n(alpha_n);
        set_dt(dt);
        set_is_discrete(discrete);
    }
    
    int get_space_width() const
    {
        return space_width;
    }
    
    int get_space_height() const
    {
        return space_height;
    }
    
    float get_ra() const
    {
        return ra;
    }
    
    void set_ra(float ra)
    {
        if(ra <= 0)
        {
            cerr << "Ruleset: Invalid ra" << endl;
            exit(-1);
        }
    
        this->ra = ra;
    }
    
    float get_rr() const
    {
        return rr;
    }
    
    float get_ri() const
    {
        return ra / rr;        
    }
    
    void set_rr(float rr)
    {
        if(rr <= 0)
        {
            cerr << "Ruleset: Invalid rr" << endl;
            exit(-1);
        }
        
        this->rr = rr;
    }
    
    float get_b1() const
    {
        return b1;
    }
    
    void set_b1(float b1)
    {
        if(b1 <= 0)
        {
            cerr << "Ruleset: Invalid b1" << endl;
            exit(-1);
        }
        
        this->b1 = b1;
    }
    
    float get_b2() const
    {
        return b2;
    }
    
    void set_b2(float b2)
    {
        if(b2 <= 0)
        {
            cerr << "Ruleset: Invalid b2" << endl;
            exit(-1);
        }
        
        this->b2 = b2;
    }
    
    float get_d1() const
    {
        return d1;
    }
    
    void set_d1(float d1)
    {
        if(d1 <= 0)
        {
            cerr << "Ruleset: Invalid d1" << endl;
            exit(-1);
        }
        
        this->d1 = d1;
    }
    
    float get_d2() const
    {
        return d2;
    }
    
    void set_d2(float d2)
    {
        if(d2 <= 0)
        {
            cerr << "Ruleset: Invalid d2" << endl;
            exit(-1);
        }
        
        this->d2 = d2;
    }
    
    float get_alpha_m() const
    {
        return alpha_m;
    }
    
    void set_alpha_m(float alpha_m)
    {
        if(alpha_m <= 0)
        {
            cerr << "Ruleset: Invalid alpha_m" << endl;
            exit(-1);
        }
        
        this->alpha_m = alpha_m;
    }
    
    float get_alpha_n() const
    {
        return alpha_n;
    }
    
    void set_alpha_n(float alpha_n)
    {
        if(alpha_n <= 0)
        {
            cerr << "Ruleset: Invalid alpha_n" << endl;
            exit(-1);
        }
        
        this->alpha_n = alpha_n;
    }
    
    bool get_is_discrete() const
    {
        return discrete;
    }
    
    void set_is_discrete(bool discrete)
    {
        this->discrete = discrete;
    }
    
    float get_dt() const
    {
        return dt;
    }
    
    void set_dt(float dt)
    {
        if(dt <= 0)
        {
            cerr << "Ruleset: Invalid dt" << endl;
            exit(-1);
        }
        
        this->dt = dt;
    }
    
private:

    int space_width;
    int space_height;   
    float ra;
    float rr;
    float b1;
    float b2;
    float d1;
    float d2;
    float alpha_m;
    float alpha_n;
    bool discrete;
    float dt; 

protected:    

    ruleset(int width, int height)
    {
        this->space_width = width;
        this->space_height = height;
    }   

};

/**
 * @brief Smooth Life L ruleset
 */
class ruleset_smooth_life_l : public ruleset
{
public:
    ruleset_smooth_life_l(int width, int height) : ruleset(width, height)
    {         
         set_ra(10);
         set_rr(3.0);
         set_b1(0.257);
         set_b2(0.336);
         set_d1(0.365);
         set_d2(0.549);
         set_alpha_m(0.147);
         set_alpha_n(0.028);
         set_is_discrete(false);
         set_dt(0.1);
    }
};

/**
 * @brief Ruleset suggested in paper by S. Rafler
 */
class ruleset_rafler_paper : public ruleset
{
public:

    ruleset_rafler_paper(int width, int height) : ruleset(width, height)
    {
        set_alpha_n(0.028);
                set_alpha_m(0.147);
                set_ra(21);
                set_rr(3);
                set_b1(0.278);
                set_b2(0.365);
                set_d1(0.267);
                set_d2(0.445);
                set_is_discrete(false);
                set_dt(0.1);
    }
};

/**
* @brief Returns ruleset from name. Returns smooth_life_l if name is invalid
*/
inline ruleset ruleset_from_name(std::string name, int width, int height)
{
    if(name == "rafler_paper")
        return ruleset_rafler_paper(width, height);
    
    return ruleset_smooth_life_l(width, height);    
}

inline ruleset ruleset_from_cli(int argc, char ** argv)
{
    switch(argc - 1)
    {
        case 0:
            //No parameters provided: Run with default settings
            return ruleset_smooth_life_l(RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);
        case 1:
            //Provided ruleset name
            return ruleset_from_name(std::string(argv[1]), RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);
        case 2:
            //Provided size - use default ruleset
            return ruleset_smooth_life_l(RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);
    }
    
    cerr << "Invalid arguments!" << endl;
    cerr << "smooth_life" << endl;
    cerr << "smooth_life <ruleset> <w> <h>" << endl;
    cerr << "smooth_life <w> <h>" << endl;
    
    return ruleset_smooth_life_l(RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);
}