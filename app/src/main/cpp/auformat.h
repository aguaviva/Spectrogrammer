#include <cstdio>
#include <cstdlib>

#define AU_FORMAT int16_t
#define AU_LEN(l) (l/sizeof(int16_t))
#define AU_CONVERT2DOUBLE(v) ((double)v)


double Uint16ToDouble(const AU_FORMAT *v);
AU_FORMAT DoubleToUint16(double v);
