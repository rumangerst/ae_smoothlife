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
        if (width <= 0 || height <= 0)
        {
            cerr << "Provided invalid space size" << endl;
            exit(-1);
        }

        m_space_width = width;
        m_space_height = height;

        set_radius_outer(ra);
        set_radius_ratio(rr);
        set_birth_min(b1);
        set_birth_max(b2);
        set_death_min(d1);
        set_death_max(d2);
        set_alpha_m(alpha_m);
        set_alpha_n(alpha_n);
        set_delta_time(dt);
        set_is_discrete(discrete);
    }

    int get_space_width() const
    {
        return m_space_width;
    }

    int get_space_height() const
    {
        return m_space_height;
    }

    float get_radius_outer() const
    {
        return m_radius_outer;
    }

    void set_radius_outer(float ra)
    {
        if (ra <= 0)
        {
            cerr << "Ruleset: Invalid ra" << endl;
            exit(-1);
        }

        this->m_radius_outer = ra;
    }

    float get_radius_ratio() const
    {
        return m_radius_ratio;
    }

    float get_radius_inner() const
    {
        return m_radius_outer / m_radius_ratio;
    }

    void set_radius_ratio(float rr)
    {
        if (rr <= 0)
        {
            cerr << "Ruleset: Invalid rr" << endl;
            exit(-1);
        }

        this->m_radius_ratio = rr;
    }

    float get_birth_min() const
    {
        return m_birth_min;
    }

    void set_birth_min(float b1)
    {
        if (b1 <= 0)
        {
            cerr << "Ruleset: Invalid b1" << endl;
            exit(-1);
        }

        this->m_birth_min = b1;
    }

    float get_birth_max() const
    {
        return m_birth_max;
    }

    void set_birth_max(float b2)
    {
        if (b2 <= 0)
        {
            cerr << "Ruleset: Invalid b2" << endl;
            exit(-1);
        }

        this->m_birth_max = b2;
    }

    float get_death_min() const
    {
        return n_death_min;
    }

    void set_death_min(float d1)
    {
        if (d1 <= 0)
        {
            cerr << "Ruleset: Invalid d1" << endl;
            exit(-1);
        }

        this->n_death_min = d1;
    }

    float get_death_max() const
    {
        return m_death_max;
    }

    void set_death_max(float d2)
    {
        if (d2 <= 0)
        {
            cerr << "Ruleset: Invalid d2" << endl;
            exit(-1);
        }

        this->m_death_max = d2;
    }

    float get_alpha_m() const
    {
        return alpha_m;
    }

    void set_alpha_m(float alpha_m)
    {
        if (alpha_m <= 0)
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
        if (alpha_n <= 0)
        {
            cerr << "Ruleset: Invalid alpha_n" << endl;
            exit(-1);
        }

        this->alpha_n = alpha_n;
    }

    bool get_is_discrete() const
    {
        return m_is_discrete;
    }

    void set_is_discrete(bool discrete)
    {
        this->m_is_discrete = discrete;
    }

    float get_delta_time() const
    {
        return m_delta_time;
    }

    void set_delta_time(float dt)
    {
        if (dt <= 0)
        {
            cerr << "Ruleset: Invalid dt" << endl;
            exit(-1);
        }

        this->m_delta_time = dt;
    }

private:

    int m_space_width;
    int m_space_height;
    // radius' of the masks (cell size)
    float m_radius_outer;
    float m_radius_ratio;
    // interval in which a cell stays alive/comes to live
    float m_birth_min;
    float m_birth_max;
    // interval in which a cell shall de
    float n_death_min;
    float m_death_max;
    float alpha_m; // inner sigmoid width
    float alpha_n; // outer sigmoid width
    bool m_is_discrete;
    float m_delta_time;

protected:

    ruleset(int width, int height)
    {
        this->m_space_width = width;
        this->m_space_height = height;
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
        set_radius_outer(10);
        set_radius_ratio(3.0);
        set_birth_min(0.257);
        set_birth_max(0.336);
        set_death_min(0.365);
        set_death_max(0.549);
        set_alpha_m(0.147);
        set_alpha_n(0.028);
        set_is_discrete(false);
        set_delta_time(0.1);
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
        set_radius_outer(21);
        set_radius_ratio(3);
        set_birth_min(0.278);
        set_birth_max(0.365);
        set_death_min(0.267);
        set_death_max(0.445);
        set_is_discrete(false);
        set_delta_time(0.05);
    }
};

/**
 * @brief Returns ruleset from name. Returns smooth_life_l if name is invalid
 */
inline ruleset ruleset_from_name(std::string name, int width, int height)
{
    if (name == "rafler_paper")
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

    if (v != "=")
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

    if (v != "=")
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

    if (params == 0)
    {
        return ruleset_smooth_life_l(RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);
    }
    else
    {
        if (string(argv[1]) == "help" || params > 13)
        {
            if (params > 13)
                cerr << "Too many parameters!" << endl;

            cli_print_help();
            exit(0);
        }
        else
        {
            bool new_ruleset = string(argv[1]) == "new";

            if (new_ruleset && params < 13)
            {
                cerr << "You must set all values for a new ruleset!" << endl;
                exit(-1);
            }

            int w = RULESET_DEFAULT_SPACE_W;
            int h = RULESET_DEFAULT_SPACE_H;

            if (params >= 2 && string(argv[2]) != "=")
                w = stoi(argv[2]);
            if (params >= 3 && string(argv[3]) != "=")
                h = stoi(argv[3]);

            ruleset base = new_ruleset ? RULESET_DEFAULT_t(w, h) : ruleset_from_name(string(argv[1]), w, h);

            //Set ra
            if (params >= 4)
                ruleset_cli_set_float_value(argv, 4, base, new_ruleset, &ruleset::set_radius_outer);
            //Set rr
            if (params >= 5)
                ruleset_cli_set_float_value(argv, 5, base, new_ruleset, &ruleset::set_radius_ratio);
            //Set b1
            if (params >= 6)
                ruleset_cli_set_float_value(argv, 6, base, new_ruleset, &ruleset::set_birth_min);
            //Set b2
            if (params >= 7)
                ruleset_cli_set_float_value(argv, 7, base, new_ruleset, &ruleset::set_birth_max);
            //Set d1
            if (params >= 8)
                ruleset_cli_set_float_value(argv, 8, base, new_ruleset, &ruleset::set_death_min);
            //Set d2
            if (params >= 9)
                ruleset_cli_set_float_value(argv, 9, base, new_ruleset, &ruleset::set_death_max);
            //Set alpha_m
            if (params >= 10)
                ruleset_cli_set_float_value(argv, 10, base, new_ruleset, &ruleset::set_alpha_m);
            //Set alpha_n
            if (params >= 11)
                ruleset_cli_set_float_value(argv, 11, base, new_ruleset, &ruleset::set_alpha_n);
            //Set dt
            if (params >= 12)
                ruleset_cli_set_float_value(argv, 12, base, new_ruleset, &ruleset::set_delta_time);
            //Set discrete
            if (params >= 13)
                ruleset_cli_set_bool_value(argv, 13, base, new_ruleset, &ruleset::set_is_discrete);

            return base;
        }
    }


    return ruleset_smooth_life_l(RULESET_DEFAULT_SPACE_W, RULESET_DEFAULT_SPACE_H);
}
