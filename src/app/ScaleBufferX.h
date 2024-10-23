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
    BufferIOInt *m_pBinsOffsets;

public:
    ~ScaleBufferX()
    {
        delete(m_pBinsOffsets);
    }

    void setOutputWidth(int outputWidth, float minFreq, float maxFreq)
    {
        delete (m_pOutput);
        m_pOutput = new BufferIODouble(outputWidth);
        m_pOutput->clear();

        delete (m_pBinsOffsets);
        m_pBinsOffsets = new BufferIOInt(outputWidth);
        m_pBinsOffsets->clear();

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
        int *pBins = m_pBinsOffsets->GetData();
        for(int i=0; i < m_pBinsOffsets->GetSize(); i++)
        {
            float t = (float)i/(float)(m_pBinsOffsets->GetSize()-1.0f);
            float freq = XtoFreq(t);
            int bin2 = (int)floor(pProc->freq2Bin(freq));
            pBins[i] = bin2;
        }
    }

    void Build(BufferIODouble *inputIO)
    {
        //set bins to min value
        m_pOutput->clear();

        float *input = inputIO->GetData();
        float *output = m_pOutput->GetData();
        int *pBins = m_pBinsOffsets->GetData();

        for(int x=0; x < m_pOutput->GetSize(); x++)
            output[x] = 0;

        // set X axis
        for(int x=0; x < m_pOutput->GetSize(); x++)
        {
            int bin = pBins[x];
            output[x]=std::max(input[bin], output[x]);
        }
    }
};

typedef ScaleBufferX<true> ScaleBufferXLog;
typedef ScaleBufferX<false> ScaleBufferXLin;

#endif