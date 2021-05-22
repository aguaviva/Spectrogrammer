#include "scale.h"
#include <cmath>

double lerp( double t, double min, double max)
{
    return min*(1.0-t) + max*t;
}

double unlerp( double min, double max, double x)
{
    return (x - min)/(max - min);
}

double clamp(double v, double min, double max)
{
    if (v<min)
        return min;
    if  (v>max)
        return max;
    return v;
}

//--------------------------

void Scale::init(double maxIdx_,double minFreq_, double maxFreq_)
{
    // Logarithmic
    a = minFreq_;
    b = log10(maxFreq_ / a) / maxIdx_;
    inva = 1.0/a;
    invb = 1.0/b;

    // Linear
    maxIdx = maxIdx_;
    minFreq = minFreq_;
    maxFreq = maxFreq_;
}

double Scale::forward(double x) const
{
    if (bLogarithmic)
    {
        return (a * pow(10, b * x));
    }
    else
    {
        return (x / maxIdx) * (maxFreq - minFreq) + minFreq;
    }
}

double Scale::backward(double x)  const
{
    if (bLogarithmic)
    {
        return log10(x * inva) * invb;
    }
    else
    {
        return ((x - minFreq) / (maxFreq - minFreq)) * maxIdx;
    }
}


