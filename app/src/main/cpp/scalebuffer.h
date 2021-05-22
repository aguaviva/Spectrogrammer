#ifndef SCALEBUFFER_H
#define SCALEBUFFER_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"
#include "ScaleBufferBase.h"


template <bool useLogX, bool useLogY>
class ScaleBuffer : public ScaleBufferBase
{
    Scale<useLogX> scaleXtoFreq;
    BufferIOInt *m_pBins;

public:
    ~ScaleBuffer()
    {
        delete(m_pOutput);
        delete(m_pBins);
    }

    void setOutputWidth(int outputWidth, float minFreq, float maxFreq)
    {
        delete (m_pOutput);
        m_pOutput = new BufferIODouble(outputWidth);

        delete(m_pBins);
        m_pBins = new BufferIOInt(outputWidth);

        scaleXtoFreq.init(outputWidth, minFreq, maxFreq);
    }

    float XtoFreq(float x) const
    {
        return scaleXtoFreq.forward(x);
    }

    float FreqToX(float freq) const
    {
        return scaleXtoFreq.backward(freq);
    }

    void PreBuild(Processor *pProc)
    {
        int *pBins = m_pBins->GetData();
        for(int x=0; x < m_pBins->GetSize(); x++)
        {
            float freq = XtoFreq(x);
            int bin = (int)floor(pProc->freq2Bin(freq));
            pBins[x] = bin;
        }
    }

    void Build(Processor *pProc)
    {
        //set bins to min value
        m_pOutput->clear();

        float *input = pProc->getBufferIO()->GetData();
        float *output = m_pOutput->GetData();
        int *pBins = m_pBins->GetData();

        // set X axis
        int x=0;
        int bin = pBins[x];
        output[x++]= input[bin];
        for(; x < m_pOutput->GetSize(); x++)
        {
            int binNext = pBins[x];

            if (binNext==bin)
            {
                output[x]=output[x-1];
            }
            else
            {
                float v = 0;
                for (int i = bin; i < binNext; i++)
                {
                    v = std::max(input[i], v);
                }
                output[x]=v;
            }

            bin = binNext;
        }

        // set Y axis
        if (useLogY)
        {
            // log Y axis
            for (int i = 0; i < m_pOutput->GetSize(); i++)
            {
                const float ref = 32768;
                output[i] = convertToDecibels(output[i], ref);
                output[i] = clamp(unlerp( -120, -20, output[i]), 0, 1);
            }
        }
        else
        {
            for (int i = 0; i < m_pOutput->GetSize(); i++)
            {
                output[i] = clamp(unlerp( 0, 32768/20, output[i]), 0, 1);
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

typedef ScaleBuffer<true, true> ScaleBufferLogLog;
typedef ScaleBuffer<false, true> ScaleBufferLinLog;
typedef ScaleBuffer<true, false> ScaleBufferLogLin;
typedef ScaleBuffer<false,false> ScaleBufferLinLin;

#endif