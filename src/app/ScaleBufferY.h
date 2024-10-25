#ifndef SCALEBUFFERY_H
#define SCALEBUFFERY_H

#include "BufferIO.h"

float linearToDecibels(float v, float ref);
float decibelsToLinear(float v, float ref);

void applyScaleY(BufferIODouble *pInput, float min, float max, bool use_log, BufferIODouble *pOutput);

#endif