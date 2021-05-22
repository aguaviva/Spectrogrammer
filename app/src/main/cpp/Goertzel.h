#ifndef GOERTZEL_H
#define GOERTZEL_H

#define _USE_MATH_DEFINES
#include "Processor.h"
#include <cmath>
#include "auformat.h"

double goertzelFilter(double *input, int length, double frequency, double sampleRate)
{
    double omega = 2 * M_PI * frequency / sampleRate;
    double cr = cos(omega);
    double coeff = 2 * cr;

    double sprev = 0;
    double sprev2 = 0;
    for (int i = 0; i < length; i++)
    {
        double s = input[i] + coeff * sprev - sprev2;
        sprev2 = sprev;
        sprev = s;
    }

    double power = sprev2 * sprev2 + sprev * sprev - coeff * sprev * sprev2;

    return power;
}

double NoteToFreq(int note)
{
    float exponent = ((float) note - 49.0f) / 12.0f;
    return 440.0*pow(2, exponent);
}

int FreqToNote(double freq)
{
    return (int)floor((log(freq/440.0)/log(2))*12+49);
}


class Goertzel : public Processor
{
    int m_length;
    double *inputBuf;

    double m_sampleRate;

    void init(int length)
    {
        m_length = length;
        inputBuf = (double*) malloc(sizeof(double) * m_length);
        m_pOutput = new BufferIODouble(getBins() );
        m_pOutput->clear();
    }

    void deinit()
    {
        free(inputBuf);
        delete(m_pOutput);
    }

    double hanning(int i) const
    {
        return .50 - .50 * cos((2 * M_PI * i) / (m_length - 1.0));
    }

    double hamming(int i) const
    {
        return .54 - .46 * cos((2 * M_PI * i) / (m_length - 1.0));
    }
public:
    ~Goertzel()
    {
        deinit();
    }

    void init(int length, double sampleRate)
    {
        m_sampleRate = sampleRate;
        deinit();
        init(length);
    }

    int getBins() const { return 88; }
    int getProcessedLength() const { return m_length; }

    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length)
    {
        for (int i = 0; i < length; i++)
        {
            double val = Uint16ToDouble(&input[i]);

            int ii= i + offsetDest;
            //val *= hamming(ii);

            inputBuf[ii] = val;
        }
    }

    void computePower(float decay)
    {
        double *powerBuf = m_pOutput->GetData();
        for(int i=0;i<m_pOutput->GetSize();i++)
        {
            double frequency = NoteToFreq(i+1);
            double power = sqrt(goertzelFilter(inputBuf, m_length, frequency, m_sampleRate));
            powerBuf[i] = power;
        }
    }

    double bin2Freq(int bin) const
    {
        return NoteToFreq(bin +1);
    }

    double freq2Bin(double freq) const
    {
        return FreqToNote(freq)-1;
    }
};

#endif