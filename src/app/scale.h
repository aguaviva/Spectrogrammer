#ifndef SCALE_H
#define SCALE_H
#include "math.h"

float lerp( float t, float min, float max);
float unlerp( float min, float max, float x);
float clamp(float v, float min, float max);

template <bool bLogarithmic>
class Scale {
    //log stuff
    float a,b, inva, invb;

    // lin stuff
    float minFreq, maxFreq;

public:
    void init(float minFreq_, float maxFreq_)
    {
        // Logarithmic
        if (bLogarithmic) 
        {
            a = minFreq_;
            b = log10(maxFreq_) - log10(a);
            inva = 1.0 / a;
            invb = 1.0 / b;
        }
        else 
        {
            // Linear
            minFreq = minFreq_;
            maxFreq = maxFreq_;
        }
    }

    float forward(float x) const
    {
        if (bLogarithmic)
        {
            return a * pow(10, b * x);
        }
        else
        {
            return x * (maxFreq - minFreq) + minFreq;
        }
    }

    float backward(float x)  const
    {
        if (bLogarithmic)
        {
            return log10(x * inva) * invb;
        }
        else
        {
            return (x - minFreq) / (maxFreq - minFreq);
        }
    }
};

#endif