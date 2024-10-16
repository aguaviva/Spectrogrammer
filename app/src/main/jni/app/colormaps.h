#ifndef COLORMAPS_H
#define COLORMAPS_H

#define TO16BITS(r,g,b) ((uint16_t)( ((r << 8) & 0xf800) | ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f) ))
uint16_t GetMagma(int i);

#endif
