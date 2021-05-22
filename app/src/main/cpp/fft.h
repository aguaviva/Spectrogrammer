#ifndef MYFFT_H
#define MYFFT_H

#define _USE_MATH_DEFINES
#include <cmath>
#include "Processor.h"
#include "fftw3.h"
#include "auformat.h"

#define REAL 0
#define IMAG 1

class myFFT : public Processor
{
    fftw_complex *m_in = nullptr;
    fftw_complex *m_out = nullptr;

    int m_length;
    double m_fftScaling;
    fftw_plan m_plan;
    double m_sampleRate;

    void init(int length)
    {
        m_length = length;

        m_fftScaling = 0;
        for(int i=0;i<m_length;i++)
            m_fftScaling+=hamming(i);

        m_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_length);
        m_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_length);
        m_plan = fftw_plan_dft_1d(m_length, m_in, m_out, FFTW_FORWARD, FFTW_ESTIMATE);

        m_pOutput = new BufferIODouble(getBins() );
        m_pOutput->clear();
    }

    void deinit()
    {
        delete(m_pOutput);

        fftw_destroy_plan(m_plan);
        fftw_free(m_in);
        fftw_free(m_out);
    }

public:
    ~myFFT()
    {
        deinit();
    }

    void init(int length, double sampleRate)
    {
        m_sampleRate = sampleRate;
        deinit();
        init(length);
    }

    int getProcessedLength() const { return m_length; }

    int getBins() const { return m_length/2; }

    double hanning(int i) const
    {
        return .50 - .50 * cos((2 * M_PI * i) / (m_length - 1.0));
    }

    double hamming(int i) const
    {
        return .54 - .46 * cos((2 * M_PI * i) / (m_length - 1.0));
    }

    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length)
    {
        for (int i = 0; i < length; i++)
        {
            double val = Uint16ToDouble(&input[i]);

            int ii= i + offsetDest;
            val *= hamming(ii);

            m_in[ii][REAL] = val;
            m_in[ii][IMAG] = 0;
        }
    }

    void computePower(float decay)
    {
        fftw_execute(m_plan);

        double totalPower = 0;

        double *m_rout = m_pOutput->GetData();
        for (int i = 0; i < getBins(); i++)
        {
            double power = sqrt(m_out[i][REAL] * m_out[i][REAL] + m_out[i][IMAG] * m_out[i][IMAG]);
            power *= (2 / m_fftScaling);
            m_rout[i] = m_rout[i] *decay + power*(1.0f-decay);

            totalPower += power;
        }
    }


    double bin2Freq(int bin) const
    {
        if (bin==0)
            return 0;

        return (m_sampleRate * (double)bin) / (double) getProcessedLength();
    }

    double freq2Bin(double freq) const
    {
        if (freq==0)
            return 0;

        return (freq * (double) getProcessedLength()) / m_sampleRate;
    }
};

#endif