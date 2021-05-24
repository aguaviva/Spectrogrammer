#include "scale.h"

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include "ScaleBufferBase.h"


void drawWaterFallLine(const AndroidBitmapInfo*  info, int yy, void*  pixels, BufferIODouble *pBuffer);
void drawSpectrumBars(const AndroidBitmapInfo*  info, void*  pixels, int height, BufferIODouble *pBuffer);
void drawHeldData(const AndroidBitmapInfo*  info, void*  pixels, int height, BufferIODouble *pScaleLog);
