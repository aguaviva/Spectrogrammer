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




