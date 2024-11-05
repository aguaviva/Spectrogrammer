#ifndef SCALEBUFFERY_H
#define SCALEBUFFERY_H

#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"
#include "ScaleBufferY.h"

float linearToDecibels(float v, float ref)
{
    if (v<=0.001)
        v = 0.001;

    return 20 * log10(v / ref);
}

float decibelsToLinear(float v, float ref)
{
    return ref * pow(10, v /  20.0f);
}

void applyScaleY(BufferIODouble *pInput, float min, float max, bool use_log, BufferIODouble *pOutput)
{
    pOutput->Resize(pInput->GetSize());

    float *pInputData = pInput->GetData();
    float *pOutputData = pOutput->GetData();

    const float ref = 32768;

    // set Y axis
    if (use_log)
    {
        // log Y axis

        for (int i = 0; i < pInput->GetSize(); i++)
        {
            float dec = linearToDecibels(pInputData[i], ref);
            pOutputData[i] = clamp(unlerp( min, max, dec), 0, 1);
        }
    }
    else
    {
        min = decibelsToLinear(min, ref);
        max = decibelsToLinear(max, ref);

        for (int i = 0; i < pInput->GetSize(); i++)
        {
            float dec = pInputData[i];
            pOutputData[i] = clamp(unlerp( min, max, dec), 0, 1);
        }
    }
}

#endif