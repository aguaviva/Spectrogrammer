#include "auformat.h"

double Uint16ToFloat(const AU_FORMAT *v)
{
    return AU_CONVERT2FLOAT(*v);
}

AU_FORMAT FloatToUint16(float v)
{
    v = v + 32767;
    return (AU_FORMAT)v;
}

