#ifndef SCALEBUFFERX_H
#define SCALEBUFFERX_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"
#include "ScaleBufferBase.h"


template <bool use_log>
class ScaleBufferX : public ScaleBufferBase
{
    Scale<use_log> scaleXtoFreq;
    BufferIOInt *m_pBins;

public:
    ~ScaleBufferX()
    {
        delete(m_pBins);
    }

    void setOutputWidth(int outputWidth, float minFreq, float maxFreq)
    {
        delete (m_pOutput);
        m_pOutput = new BufferIODouble(outputWidth);
        m_pOutput->clear();

        delete (m_pBins);
        m_pBins = new BufferIOInt(outputWidth);
        m_pBins->clear();

        scaleXtoFreq.init(m_pBins->GetSize(), minFreq, maxFreq);
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
            int bin = (int)ceil(pProc->freq2Bin(freq));
            pBins[x] = bin;
        }
    }

    void Build(BufferIODouble *inputIO)
    {
        //set bins to min value
        m_pOutput->clear();

        float *input = inputIO->GetData();
        float *output = m_pOutput->GetData();
        int *pBins = m_pBins->GetData();

        // set X axis
        int x=0;
        int bin = pBins[x];
        output[x]= input[bin];
        for(x=1; x < m_pOutput->GetSize(); x++)
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
    }
};

typedef ScaleBufferX<true> ScaleBufferXLog;
typedef ScaleBufferX<false> ScaleBufferXLin;

#endif