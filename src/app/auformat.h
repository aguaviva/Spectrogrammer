#include <cstdio>
#include <cstdlib>

#define AU_FORMAT int16_t
#define AU_LEN(l) (l/sizeof(int16_t))
//#define AU_CONVERT2DOUBLE(v) ((double)v)
#define AU_CONVERT2FLOAT(v) ((float)v)


double Uint16ToFloat(const AU_FORMAT *v);
AU_FORMAT FloatToUint16(float v);
