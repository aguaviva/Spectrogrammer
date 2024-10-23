#ifndef MYFFT_H
#define MYFFT_H

#include <cassert>
#include "Processor.h"
#include "kiss_fftr.h"
#include "auformat.h"

float hanning(int i, int window_size)
{
    return .50f - .50f * cos((2.0f * M_PI * (float)i) / (float)(window_size - 1));
}

float hamming(int i, int window_size)
{
    return .54f - .46f * cos((2.0f * M_PI * (float)i) / (float)(window_size - 1));
}

class myFFT : public Processor
{
    kiss_fft_cpx *m_out_cpx = nullptr;

    kiss_fftr_cfg m_cfg;

    BufferIODouble m_pInput_samples;
    BufferIODouble m_pOutput_bins;

    float m_fftScaling;
    float m_sampleRate;
    float m_freq_resolution;

    void init(int nfft)
    { 
        m_cfg = kiss_fftr_alloc(nfft, false, 0, 0);
        m_pInput_samples.Resize(nfft);
        m_pOutput_bins.Resize(nfft/2+1);
        
        for (int i = 0; i < m_pOutput_bins.GetSize(); i++)
            m_pOutput_bins.GetData()[i]=0;

        m_out_cpx = (kiss_fft_cpx*) malloc(sizeof(kiss_fft_cpx) * (nfft/2+1));

        m_fftScaling = 0;
        for(int i=0;i<m_pInput_samples.GetSize();i++)
            m_fftScaling+=hanning(i, m_pInput_samples.GetSize());
    }

    void deinit()
    {
        kiss_fftr_free(m_cfg);
    }

public:
    ~myFFT()
    {
        deinit();
    }

    virtual const char *GetName() const {  return "FFT"; };

    void init(int nfft, float sampleRate)
    {
        init(nfft);
        m_sampleRate = sampleRate;
        m_freq_resolution = sampleRate / nfft;
    }

    int getBinCount() const { return m_pOutput_bins.GetSize(); }

    int getProcessedLength() const { return m_pInput_samples.GetSize(); }

    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length)
    {
        assert(m_pInput_samples.GetSize()>=offsetDest+length);

        float *m_in_samples = m_pInput_samples.GetData();
        for (int i = 0; i < length; i++)
        {
            float val = Uint16ToFloat(&input[i]);

            int ii= i + offsetDest;
            val *= hanning(ii, m_pInput_samples.GetSize());

            assert(ii < m_pInput_samples.GetSize());
            m_in_samples[ii] = val;
        }
    }

    void computePower(float decay)
    {
        kiss_fftr( m_cfg , m_pInput_samples.GetData() , m_out_cpx );

        float totalPower = 0;

        float *m_rout = m_pOutput_bins.GetData();
        for (int i = 0; i < getBinCount(); i++)
        {
            float power = sqrt(m_out_cpx[i].r * m_out_cpx[i].r + m_out_cpx[i].i * m_out_cpx[i].i);
            power *= (2 / m_fftScaling);
            m_rout[i] = m_rout[i] *decay + power*(1.0f-decay);


            totalPower += power;
        }

        /*
        for (int i = 0; i < getBinCount(); i++)
        {
            m_rout[i] = hamming(i, getBinCount());
        }
        m_rout[0] = 1.0f;
        */
    }

    float bin2Freq(int bin) const
    {
        return (float)(bin) * m_freq_resolution;
    }

    float freq2Bin(float freq) const
    {
        return ((float)freq / m_freq_resolution);
    }

    BufferIODouble *getBufferIO()
    {
        return &m_pOutput_bins;
    }
};

#endif