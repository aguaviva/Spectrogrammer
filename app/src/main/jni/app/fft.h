#ifndef MYFFT_H
#define MYFFT_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include "Processor.h"
#include "kiss_fftr.h"
#include "auformat.h"

#define REAL 0
#define IMAG 1

class myFFT : public Processor
{
    kiss_fft_scalar *m_in = nullptr;
    kiss_fft_cpx *m_out = nullptr;

    kiss_fftr_cfg m_cfg;

    int m_length;
    float m_fftScaling;
    float m_sampleRate;

    void init(int length)
    {
        m_length = length;

        m_fftScaling = 0;
        for(int i=0;i<m_length;i++)
            m_fftScaling+=hamming(i);
 
        m_in = (kiss_fft_scalar*) malloc(sizeof(kiss_fft_scalar) * m_length);
        m_out = (kiss_fft_cpx*) malloc(sizeof(kiss_fft_cpx) * m_length);
        m_cfg = kiss_fftr_alloc( m_length ,false ,0,0 );

        m_pOutput = new BufferIODouble(getBins());
        m_pOutput->clear();
    }

    void deinit()
    {
        delete(m_pOutput);
        m_pOutput = nullptr;
 
        kiss_fftr_free(m_cfg);
    }

public:
    ~myFFT()
    {
        deinit();
    }

    virtual const char *GetName() const {  return "FFT"; };

    void init(int length, float sampleRate)
    {
        m_sampleRate = sampleRate;
        init(length);
    }

    int getProcessedLength() const { return m_length; }

    int getBins() const { return m_length/2; }

    float hanning(int i) const
    {
        return .50 - .50 * cos((2 * M_PI * i) / (m_length - 1.0));
    }

    float hamming(int i) const
    {
        return .54 - .46 * cos((2 * M_PI * i) / (m_length - 1.0));
    }

    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length)
    {
        assert(m_length>=offsetDest+length);

        for (int i = 0; i < length; i++)
        {
            float val = Uint16ToFloat(&input[i]);

            int ii= i + offsetDest;
            val *= hamming(ii);

            assert(ii<m_length);
            m_in[ii] = val;
        }
    }

    void computePower(float decay)
    {
        kiss_fftr( m_cfg , m_in , m_out );

        float totalPower = 0;

        float *m_rout = m_pOutput->GetData();
        for (int i = 0; i < getBins(); i++)
        {
            float power = sqrt(m_out[i].r * m_out[i].r + m_out[i].i * m_out[i].i);
            power *= (2 / m_fftScaling);
            m_rout[i] = m_rout[i] *decay + power*(1.0f-decay);

            totalPower += power;
        }
    }


    float bin2Freq(int bin) const
    {
        return ((m_sampleRate) * (float)bin) / (float) getProcessedLength();
    }

    float freq2Bin(float freq) const
    {
        return (freq * (float) getProcessedLength()) / (m_sampleRate);
    }
};

#endif