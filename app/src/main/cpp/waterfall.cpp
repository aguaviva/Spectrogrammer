#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "colormaps.h"
#include "scale.h"
#include "waterfall.h"

#include <jni.h>
#include <ctime>
#include <algorithm>
#include <android/log.h>
#include <android/bitmap.h>
//--------------------------------Draw spectrum--------------------------------
inline uint16_t GetColor(float x)
{
    return GetMagma(x * 255);
}

void drawWaterFallLine(AndroidBitmapInfo*  info, int yy, void*  pixels, ScaleBufferBase *pScaleLog)
{
    uint16_t* line = (uint16_t*)((char*)pixels + info->stride*yy);

    /* optimize memory writes by generating one aligned 32-bit store
     * for every pair of pixels.
     */
    uint16_t*  line_end = line + info->width;
    uint32_t x = 0;

    float *pData = pScaleLog->GetData();

    if (line < line_end)
    {
        if (((uint32_t)(uintptr_t)line & 3) != 0)
        {
            line[0] = GetColor(pData[x++]);
            line++;
        }

        while (line + 2 <= line_end)
        {
            uint32_t  pixel;
            pixel = (uint32_t)GetColor(pData[x++]);
            pixel <<=16;
            pixel |= (uint32_t)GetColor(pData[x++]);
            ((uint32_t*)line)[0] = pixel;
            line += 2;
        }

        if (line < line_end)
        {
            line[0] = GetColor(pData[x++]);
            line++;
        }
    }
}

void drawSpectrumBar(AndroidBitmapInfo*  info, uint16_t *line, float val, int height)
{
    int y = 0;

    //background
    for (int yy=0;yy<val;yy++)
    {
        line[y] = 0x00ff;
        y += info->stride/2;
    }

    // bar
    for (int yy=val;yy<height;yy++)
    {
        line[y] = 0xffff;
        y += info->stride/2;
    }
}

void drawSpectrumBars(AndroidBitmapInfo*  info, void*  pixels, int height, ScaleBufferBase *pScaleLog)
{
    uint16_t *line = (uint16_t *)pixels;
    float *pPower = pScaleLog->GetData();
    for (int x=0;x<info->width;x++)
    {
        drawSpectrumBar(info, line, (1.0-pPower[x]) * height, height);
        line+=1;
    }
}