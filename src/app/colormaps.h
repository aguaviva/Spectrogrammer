#ifndef COLORMAPS_H
#define COLORMAPS_H
#include <inttypes.h>

#define TO16BITS(r,g,b) ((uint16_t)( ((r << 8) & 0xf800) | ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f) ))

void SetColorMap(int i);
uint16_t GetColorMap(int i);

#endif
