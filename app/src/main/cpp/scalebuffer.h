#ifndef SCALEBUFFER_H
#define SCALEBUFFER_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include "fft.h"
#include "Processor.h"
#include "scale.h"
#include "ScaleBuffer.h"
#include "BufferIO.h"

class ScaleBuffer
{
protected:
    BufferIODouble *m_pOutput = nullptr;
    bool m_useLogY = true;
    Scale scaleXtoFreq;

    BufferIOInt *m_pBins;

public:
    ~ScaleBuffer()
    {
        delete(m_pOutput);
        delete(m_pBins);
    }

    void setOutputWidth(int outputWidth, double minfreq, double maxfreq)
    {
        delete (m_pOutput);
        m_pOutput = new BufferIODouble(outputWidth);

        delete(m_pBins);
        m_pBins = new BufferIOInt(outputWidth);

        scaleXtoFreq.init(outputWidth, minfreq, maxfreq);
    }

    void SetFrequencyLogarithmicAxis(bool bLogarithmic)
    {
        scaleXtoFreq.setLogarithmic(bLogarithmic);
    }

    bool GetFrequencyLogarithmicAxis() const
    {
        return scaleXtoFreq.getLogarithmic();
    }

    double XtoFreq(double x) const
    {
        return scaleXtoFreq.forward(x);
    }

    double FreqToX(double freq) const
    {
        return scaleXtoFreq.backward(freq);
    }

    void PreBuild(Processor *pProc)
    {
        int *pBins = m_pBins->GetData();
        for(int x=0; x < m_pBins->GetSize(); x++)
        {
            double freq = XtoFreq(x);
            int bin = (int)floor(pProc->freq2Bin(freq));
            pBins[x] = bin;
        }
    }

    void Build(Processor *pProc)
    {
        //set bins to min value
        m_pOutput->clear();

        double *input = pProc->getBufferIO()->GetData();
        double *output = m_pOutput->GetData();
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
                double v = 0;
                for (int i = bin; i < binNext; i++)
                {
                    v = std::max(input[i], v);
                }
                output[x]=v;
            }

            bin = binNext;
        }

        // set Y axis
        if (m_useLogY)
        {
            // log Y axis
            for (int i = 0; i < m_pOutput->GetSize(); i++)
            {
                const double ref = 32768;
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

    double *GetData() const
    {
        return m_pOutput->GetData();
    }

    double convertToDecibels(double v, double ref)
    {
        if (v<=0.001)
            return -120;

        return 20 * log10(v / ref);
    }
};

#endif