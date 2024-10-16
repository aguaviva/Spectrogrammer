#ifndef SCALEBUFFERY_H
#define SCALEBUFFERY_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"
#include "ScaleBufferBase.h"

class ScaleBufferY : public BufferIODouble
{
public:
    void apply(BufferIODouble *inputIO, float volume, bool use_log)
    {
        if (inputIO->GetSize()!=GetSize())
        {
            Resize(inputIO->GetSize());
            clear();
        }

        float *pInput = inputIO->GetData();
        float *pOutput = GetData();

        // set Y axis
        if (use_log)
        {
            // log Y axis
            const float ref = 32768/volume;
            for (int i = 0; i < GetSize(); i++)
            {
                float dec = convertToDecibels(pInput[i], ref);
                pOutput[i] = clamp(unlerp( -120, -20, dec), 0, 1);
            }
        }
        else
        {
            float max =  32768/volume;
            for (int i = 0; i < GetSize(); i++)
            {
                pOutput[i] = clamp(unlerp( 0, max , pInput[i]), 0, 1);
            }
        }
    }

    float convertToDecibels(float v, float ref)
    {
        if (v<=0.001)
            return -120;

        return 20 * log10(v / ref);
    }
};

#endif