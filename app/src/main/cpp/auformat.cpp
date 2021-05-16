#include "auformat.h"

double Uint16ToDouble(const AU_FORMAT *v)
{
    return AU_CONVERT2DOUBLE(*v);
}

AU_FORMAT DoubleToUint16(double v)
{
    v = v + 32767;
    return (AU_FORMAT)v;
}


