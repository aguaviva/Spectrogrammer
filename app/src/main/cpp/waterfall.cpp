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

void drawWaterFallLine(const AndroidBitmapInfo*  info, int yy, void*  pixels, BufferIODouble *pBuffer)
{
    uint16_t* line = (uint16_t*)((char*)pixels + info->stride*yy);

    /* optimize memory writes by generating one aligned 32-bit store
     * for every pair of pixels.
     */
    uint16_t*  line_end = line + info->width;
    uint32_t x = 0;

    float *pData = pBuffer->GetData();

    if ((pData!=nullptr) && (line < line_end))
    {
        if (((uint32_t)(uintptr_t)line & 3) != 0)
        {
            assert(x<info->width);
            line[0] = GetColor(pData[x++]);
            line++;
        }

        while (line + 2 <= line_end)
        {
            uint32_t  pixel;

            assert(x<info->width);
            pixel = (uint32_t)GetColor(pData[x++]);
            pixel <<=16;

            assert(x<info->width);
            pixel |= (uint32_t)GetColor(pData[x++]);

            ((uint32_t*)line)[0] = pixel;
            line += 2;
        }

        if (line < line_end)
        {
            assert(x<info->width);
            line[0] = GetColor(pData[x++]);
            line++;
        }
    }
}

void drawSpectrumBar(const AndroidBitmapInfo*  info, uint16_t *line, float val, int height)
{
    int y = 0;

    //background
    for (int yy=0;yy<val;yy++)
    {
        line[y] = TO16BITS(0,0,255);
        y += info->stride/2;
    }

    // bar
    for (int yy=val;yy<height;yy++)
    {
        line[y] = TO16BITS(192,192,192);
        y += info->stride/2;
    }
}

void drawSpectrumBars(const AndroidBitmapInfo*  info, void*  pixels, int height, BufferIODouble *pBuffer) {
    assert(pBuffer->GetSize()==info->width);

    uint16_t *line = (uint16_t *)pixels;
    float *pPower = pBuffer->GetData();
    for (int x=0;x<pBuffer->GetSize();x++)
    {
        drawSpectrumBar(info, line, (1.0-pPower[x]) * height, height);
        line+=1;
    }
}


void drawHeldData(const AndroidBitmapInfo*  info, void*  pixels, int height, BufferIODouble *pBuffer)
{
    assert(pBuffer);
    assert(pixels);

    uint16_t color = TO16BITS(255,0,0);

    uint16_t *line = (uint16_t *)pixels;
    float *pPower = pBuffer->GetData();
    uint32_t stride = (info->stride/2);
    for (int x=0;x<pBuffer->GetSize();x++)
    {
        uint32_t yy = (1.0 - pPower[x]) * height;
        uint32_t y = yy*stride;

        line[x+y] = color; y+= stride;
        line[x+y] = color; y+= stride;
        line[x+y] = color; y+= stride;
        line[x+y] = color; y+= stride;
        line[x+y] = color; y+= stride;
    }
}
