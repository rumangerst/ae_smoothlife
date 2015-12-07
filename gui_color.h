#pragma once

#include <vector>

using namespace std;

struct color
{
    double r = 0;
    double g = 0;
    double b = 0;
    double a = 1;

    color()
    {

    }

    color(int r, int g, int b, double a)
    {
        this->r = r / 255.0;
        this->g = g / 255.0;
        this->b = b / 255.0;
        this->a = a;
    }

    color(double r, double g, double b, double a)
    {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }

    void set(color clr)
    {
        r = clr.r;
        g = clr.g;
        b = clr.b;
        a = clr.a;
    }

};

inline double lerp(double y1,double y2,double mu)
{
    return (y1*(1-mu)+y2*mu);
}

inline color lerp_color_2 (color y1, color y2, double mu)
{
    return color(lerp(y1.r,y2.r,mu), lerp(y1.g,y2.g,mu), lerp(y1.b,y2.b,mu), lerp(y1.a,y2.a,mu));
}

inline color lerp_color_n(vector<color> colors, double mu)
{
    int n = colors.size() - 1;
    int color_start = floor(mu * n);

    return lerp_color_2(colors[color_start], colors[color_start + 1], mu * n - color_start);
}

inline double clamp(double mu)
{
    return fmin(1,fmax(0,mu));
}

inline Uint8 bytecolor(double floatcolor)
{
    return (int)ceil(floatcolor * 255);
}

