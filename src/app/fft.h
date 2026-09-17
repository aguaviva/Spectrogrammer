#ifndef MYFFT_H
#define MYFFT_H

#include <cassert>
#include "Processor.h"
#include <cmath>
#include "kiss_fftr.h"
#include "auformat.h"

float hanning(int i, int window_size);

class myFFT : public Processor
{
    kiss_fft_cpx *m_out_cpx = nullptr;

    kiss_fftr_cfg m_cfg = nullptr;

    BufferIODouble m_pInput_samples;
    BufferIODouble m_pOutput_bins;
    BufferIODouble m_window;

    float m_fftScaling;
    float m_invFftScaling;
    float m_sampleRate;
    float m_freq_resolution;

    void init(int nfft)
    { 
        m_cfg = kiss_fftr_alloc(nfft, false, 0, 0);
        m_pInput_samples.Resize(nfft);
        m_pOutput_bins.Resize(nfft/2+1);
        m_window.Resize(nfft);
        m_pOutput_bins.clear();

        m_out_cpx = (kiss_fft_cpx*) malloc(sizeof(kiss_fft_cpx) * (nfft/2+1));

        float *w = m_window.GetData();
        m_fftScaling = 0;
        for(int i=0;i<nfft;i++)
        {
            w[i] = hanning(i, nfft);
            m_fftScaling += w[i];
        }
        m_invFftScaling = 2.0f / m_fftScaling;
    }

    void deinit()
    {
        if (m_cfg != nullptr)
            kiss_fftr_free(m_cfg);
        free(m_out_cpx);
        m_cfg = nullptr;
        m_out_cpx = nullptr;
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
        const float *w = m_window.GetData();
        for (int i = 0; i < length; i++)
        {
            int ii = i + offsetDest;
            assert(ii < m_pInput_samples.GetSize());
            m_in_samples[ii] = (float)Uint16ToFloat(&input[i]) * w[ii];
        }
    }

    void computePower(float decay)
    {
        kiss_fftr( m_cfg , m_pInput_samples.GetData() , m_out_cpx );

        float *m_rout = m_pOutput_bins.GetData();
        const float keep = decay;
        const float mix = 1.0f - decay;
        for (int i = 0; i < getBinCount(); i++)
        {
            float power = hypotf(m_out_cpx[i].r, m_out_cpx[i].i) * m_invFftScaling;
            m_rout[i] = m_rout[i] * keep + power * mix;
        }
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