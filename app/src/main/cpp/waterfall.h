#include "scale.h"

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include "ScaleBufferBase.h"


void drawWaterFallLine(AndroidBitmapInfo*  info, int yy, void*  pixels, ScaleBufferBase *pScaleLog);
void drawSpectrumBars(AndroidBitmapInfo*  info, void*  pixels, int height, ScaleBufferBase *pScaleLog);
