#pragma once
#include <string>
#include <iostream>

#define RULESET_DEFAULT_SPACE_W 256
#define RULESET_DEFAULT_SPACE_H 256
#define RULESET_DEFAULT_t ruleset_smooth_life_l

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
	 printf("Using ruleset: smooth life l\n"); 
         set_ra(20);
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
	printf("Using ruleset: rafler paper\n");
        set_alpha_n(0.028);
                set_alpha_m(0.147);
                set_ra(21);
                set_rr(3);
                set_b1(0.278);
                set_b2(0.365);
                set_d1(0.267);
                set_d2(0.445);
                set_is_discrete(false);
                set_dt(0.05);
    }
};

/**
* @brief Returns ruleset from name. Returns smooth_life_l if name is invalid
*/
inline ruleset ruleset_from_name(std::string name, int width, int height)
{
    if(name == "rafler_paper")
        return ruleset_rafler_paper(width, height);
    else if (name == "L")
        return ruleset_smooth_life_l(width, height);
    
    return RULESET_DEFAULT_t(width, height);    
}

inline void cli_print_help()
{
    cout << "smooth_life" << endl;
    cout << "smooth_life help" << endl;
    cout << "smooth_life <ruleset> (w) (h) (ra) (rr) (b1) (b2) (d1) (d2) (alpha_m) (alpha_n) (dt) (discrete 0/1); set to = for no change" << endl;
    cout << "smooth_life new (w) (h) (ra) (rr) (b1) (b2) (d1) (d2) (alpha_m) (alpha_n) (dt) (discrete 0/1)" << endl;
}

inline void ruleset_cli_set_float_value(char ** argv, int index, ruleset & base, bool new_ruleset, void (ruleset::*set)(float))
{
    string v = string(argv[index]);
    
    if(v != "=")
    {
        // Call function ptr "set" on base with argument converted to float
        (base.*set)(stof(argv[index]));
    }
    else if (new_ruleset)
    {
        cerr << "You must set a value for new rulesets!" << endl;
        exit(-1);
    }
}

inline void ruleset_cli_set_bool_value(char ** argv, int index, ruleset & base, bool new_ruleset, void (ruleset::*set)(bool))
{
    string v = string(argv[index]);
    
    if(v != "=")
    {
        // Call function ptr "set" on base with argument converted to bool
        (base.*set)(stoi(argv[index]) == 1);
    }
    else if (new_ruleset)
    {
        cerr << "You must set a value for new rulesets!" << endl;
        exit(-1);
    }
}

inline ruleset ruleset_from_cli(int argc, char ** argv)
{
    int params = argc - 1;
    
    if(params == 0)
    {
        return ruleset_smooth_life_l(RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);
    }
    else
    {
        if(string(argv[1]) == "help" || params > 13)
        {
            if(params > 13)
                cerr << "Too many parameters!" << endl;
            
            cli_print_help();
            exit(0);
        }
        else
        {
            bool new_ruleset = string(argv[1]) == "new";
            
            if(new_ruleset && params < 13)
            {
                cerr << "You must set all values for a new ruleset!" << endl;
                exit(-1);
            }
            
            int w = RULESET_DEFAULT_SPACE_W;
            int h = RULESET_DEFAULT_SPACE_H;
            
            if(params >= 2 && string(argv[2]) != "=")
                w = stoi(argv[2]);
            if(params >= 3 && string(argv[3]) != "=")
                h = stoi(argv[3]);
            
            ruleset base = new_ruleset ? RULESET_DEFAULT_t(w,h) : ruleset_from_name(string(argv[1]), w, h);
            
            //Set ra
            if(params >= 4)
                ruleset_cli_set_float_value(argv, 4, base, new_ruleset, &ruleset::set_ra);
            //Set rr
            if(params >= 5)
                ruleset_cli_set_float_value(argv, 5, base, new_ruleset, &ruleset::set_rr);
            //Set b1
            if(params >= 6)
                ruleset_cli_set_float_value(argv, 6, base, new_ruleset, &ruleset::set_b1);
            //Set b2
            if(params >= 7)
                ruleset_cli_set_float_value(argv, 7, base, new_ruleset, &ruleset::set_b2);
            //Set d1
            if(params >= 8)
                ruleset_cli_set_float_value(argv, 8, base, new_ruleset, &ruleset::set_d1);
            //Set d2
            if(params >= 9)
                ruleset_cli_set_float_value(argv, 9, base, new_ruleset, &ruleset::set_d2);
            //Set alpha_m
            if(params >= 10)
                ruleset_cli_set_float_value(argv, 10, base, new_ruleset, &ruleset::set_alpha_m);
            //Set alpha_n
            if(params >= 11)
                ruleset_cli_set_float_value(argv, 11, base, new_ruleset, &ruleset::set_alpha_n);
            //Set dt
            if(params >= 12)
                ruleset_cli_set_float_value(argv, 12, base, new_ruleset, &ruleset::set_dt);
            //Set discrete
            if(params >= 13)
                ruleset_cli_set_bool_value(argv, 13, base, new_ruleset, &ruleset::set_is_discrete);
            
            return base;
        }
    }
    

    return ruleset_smooth_life_l(RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);    
}
