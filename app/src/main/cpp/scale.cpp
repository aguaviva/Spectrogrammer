#include "scale.h"
#include <cmath>

float lerp( float t, float min, float max)
{
    return min*(1.0-t) + max*t;
}

float unlerp( float min, float max, float x)
{
    return (x - min)/(max - min);
}

float clamp(float v, float min, float max)
{
    if (v<min)
        return min;
    if  (v>max)
        return max;
    return v;
}

//--------------------------

void Scale::init(float maxIdx_,float minFreq_, float maxFreq_)
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

float Scale::forward(float x) const
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

float Scale::backward(float x)  const
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


