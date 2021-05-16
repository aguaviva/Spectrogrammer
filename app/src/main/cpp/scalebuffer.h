#ifndef SCALEBUFFER_H
#define SCALEBUFFER_H

#include <algorithm>
#include "fft.h"
#include "auformat.h"

class ScaleBuffer
{
    myFFT *m_pFFT = nullptr;;
    int m_width;
    double *m_data = nullptr;;
    double m_sampleRate;
    int m_binCount;
    bool m_useLogY = true;

    Scale scaleXtoFreq;

public:
    ~ScaleBuffer()
    {
        free(m_data);
    }

    void SetSamplerate(double sampleRate)
    {
        m_sampleRate = sampleRate;
    }

    void SetFFT(myFFT *pFFT)
    {
        m_pFFT	= pFFT;
    }

    void SetWidth(int width)
    {
        m_width = width;
        free(m_data);
        m_data = (double *)malloc(sizeof(double) * m_width);
        scaleXtoFreq.init(width,100,  m_sampleRate/2);
    }

    void SetFrequencyLogarithmicAxis(bool bLogarithmic)
    {
        scaleXtoFreq.setLogarithmic(bLogarithmic);
    }

    bool GetFrequencyLogarithmicAxis() const
    {
        return scaleXtoFreq.getLogarithmic();
    }

    int FreqToBin(double freq) const
    {
        double t = unlerp( 1, m_sampleRate/2.0f, freq);
        return lerp( t, 1, m_pFFT->getBins());
    }

    double XtoFreq(double x) const
    {
        return scaleXtoFreq.forward(x);
    }

    double FreqToX(double freq) const
    {
        return scaleXtoFreq.backward(freq);
    }

    void Build(double min, double max)
    {
        //set bins to min value
        for(int i=0; i < m_width; i++)
            m_data[i]=0;

        // set X axis
        int x=0;
        int bin = FreqToBin(XtoFreq(x));
        m_data[x]=m_pFFT->getData(bin);
        x++;
        for(; x < m_width; x++)
        {
            int binNext = FreqToBin(XtoFreq(x+1));

            if (binNext==bin)
            {
                m_data[x]=m_data[x-1];
            }
            else
            {
                double v = 0;
                for (int i = bin; i < binNext; i++)
                {
                    v = std::max(m_pFFT->getData(i), v);
                }
                m_data[x]=v;
            }

            bin = binNext;
        }

        // set Y axis
        if (m_useLogY)
        {
            // log Y axis
            for (int i = 0; i < m_width; i++)
            {
                const double ref = 32768;
                m_data[i] = convertToDecibels(m_data[i], ref);
            }
        }
    }

    double GetNormalizedData(int i) const
    {
        if (m_useLogY)
        {
            double t = unlerp( -120, -20, m_data[i]);
            t = clamp(t, 0, 1);
            return t;
        }
        else
        {
            double t = unlerp( 0, 32768/20, m_data[i]);
            t = clamp(t, 0, 1);
            return t;
        }
    }

    int GetLength() const
    {
        return m_width;
    }

private:
    double convertToDecibels(double v, double ref)
    {
        if (v<=0.001)
            return -120;

        return 20 * log10(v / ref);
    }
};

#endif