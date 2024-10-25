#ifndef SCALEBUFFERX_H
#define SCALEBUFFERX_H

#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"
#include "ScaleBufferBase.h"


template <bool use_log>
class ScaleBufferX : public ScaleBufferBase
{
    Scale<use_log> scaleBintoFreq;
    BufferIOInt m_binOffsets;

public:
    void setOutputWidth(int outputWidth, float minFreq, float maxFreq)
    {
        m_binOffsets.Resize(outputWidth);
        m_binOffsets.clear();

        scaleBintoFreq.init(minFreq, maxFreq);
    }

    float XtoFreq(float x) const
    {
        return scaleBintoFreq.forward(x);
    }

    float FreqToX(float freq) const
    {
        return scaleBintoFreq.backward(freq);
    }

    void PreBuild(Processor *pProc)
    {
        int *pBins = m_binOffsets.GetData();
        for(int i=0; i < m_binOffsets.GetSize(); i++)
        {
            float t = (float)i/(float)(m_binOffsets.GetSize()-1.0f);
            float freq = XtoFreq(t);
            int bin2 = (int)floor(pProc->freq2Bin(freq));
            pBins[i] = bin2;
        }
    }

    void Build(BufferIODouble *pInput, BufferIODouble *pOutput)
    {
        pOutput->Resize(m_binOffsets.GetSize());

        float *pInputData = pInput->GetData();
        float *pOutputData = pOutput->GetData();
        int *pBins = m_binOffsets.GetData();

        pOutput->clear();

        // set X axis
        for(int x=0; x < m_binOffsets.GetSize(); x++)
        {
            int bin = pBins[x];
            pOutputData[x]=std::max(pInputData[bin], pOutputData[x]);
        }
    }
};

typedef ScaleBufferX<true> ScaleBufferXLog;
typedef ScaleBufferX<false> ScaleBufferXLin;

#endif