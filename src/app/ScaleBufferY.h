#ifndef SCALEBUFFERY_H
#define SCALEBUFFERY_H

#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"
#include "ScaleBufferBase.h"

class ScaleBufferY : public BufferIODouble
{
public:
    void apply(BufferIODouble *inputIO, float min, float max, bool use_log)
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
            const float ref = 32768;

            //min = linearToDecibels(min, ref);
            //max = linearToDecibels(max, ref);

            for (int i = 0; i < GetSize(); i++)
            {
                float dec = linearToDecibels(pInput[i], ref);
                pOutput[i] = clamp(unlerp( min, max, dec), 0, 1);
            }
        }
        else
        {
            for (int i = 0; i < GetSize(); i++)
            {
                pOutput[i] = clamp(unlerp( min, max , pInput[i]), 0, 1);
            }
        }
    }

    float linearToDecibels(float v, float ref)
    {
        if (v<=0.001)
            return -120;

        return 20 * log10(v / ref);
    }

    float decibelsToLinear(float v, float ref)
    {
        return ref * pow(10, v *  20.0f);
    }
};

#endif