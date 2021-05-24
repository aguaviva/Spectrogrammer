#ifndef MYFFT_H
#define MYFFT_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <complex>
#include "Processor.h"
#include "fftw3.h"
#include "auformat.h"

#define REAL 0
#define IMAG 1

class CrossCorrelation : public Processor
{
    fftw_complex *m_in = nullptr;
    fftw_complex *m_out = nullptr;

    fftw_complex *m_complexPower = nullptr;

    int m_length;
    float m_fftScaling;
    fftw_plan m_plan;
    float m_sampleRate;

    void init(int length)
    {
        m_length = length;

        m_fftScaling = 0;
        for(int i=0;i<m_length;i++)
            m_fftScaling+=hamming(i);

        m_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_length);
        m_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_length);
        m_complexPower = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_length/2);
        m_plan = fftw_plan_dft_1d(m_length, m_in, m_out, FFTW_FORWARD, FFTW_ESTIMATE);

        m_pOutput = new BufferIODouble(getBins() );
        m_pOutput->clear();
    }

    void deinit()
    {
        delete(m_pOutput);

        fftw_destroy_plan(m_plan);
        fftw_free(m_complexPower);
        fftw_free(m_in);
        fftw_free(m_out);
    }

public:
    ~CrossCorrelation()
    {
        deinit();
    }

    virtual const char *GetName() const {  return "FFT"; };

    void init(int length, float sampleRate)
    {
        m_sampleRate = sampleRate;
        deinit();
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
        for (int i = 0; i < length; i++)
        {
            float val = Uint16ToFloat(&input[i]);

            int ii= i + offsetDest;
            val *= hamming(ii);

            m_in[ii][REAL] = val;
            m_in[ii][IMAG] = 0;
        }
    }

    void computePower(float decay)
    {
/*
        fftw_execute(m_plan);

        float totalPower = 0;

        float *m_rout = m_pOutput->GetData();


        for (int i = 0; i < getBins(); i++)
        {
            m_complexPower[i].re = m_out[i].re * m_out[i].re - m_out[i].im * m_out[i].im;
            m_complexPower[i].im = 0;
        }
*/
    }


    float bin2Freq(int bin) const
    {
        if (bin==0)
            return 0;

        return (m_sampleRate * (float)bin) / (float) getProcessedLength();
    }

    float freq2Bin(float freq) const
    {
        if (freq==0)
            return 0;

        return (freq * (float) getProcessedLength()) / m_sampleRate;
    }
};

#endif