#include "scale.h"

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include "ScaleBuffer.h"


void drawWaterFallLine(AndroidBitmapInfo*  info, int yy, void*  pixels, ScaleBuffer *pScaleLog);
void drawSpectrumBars(AndroidBitmapInfo*  info, void*  pixels, int height, ScaleBuffer *pScaleLog);
