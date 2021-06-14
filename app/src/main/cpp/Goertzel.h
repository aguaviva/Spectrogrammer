#ifndef GOERTZEL_H
#define GOERTZEL_H

#define _USE_MATH_DEFINES
#include "Processor.h"
#include <cmath>
#include <assert.h>
#include "auformat.h"

double goertzelFilter(float *input, int length, float frequency, float sampleRate)
{
    float omega = 2 * M_PI * frequency / sampleRate;
    float cr = cos(omega);
    float coeff = 2 * cr;

    float sprev = 0;
    float sprev2 = 0;
    for (int i = 0; i < length; i++)
    {
        float s = input[i] + coeff * sprev - sprev2;
        sprev2 = sprev;
        sprev = s;
    }

    float power = sprev2 * sprev2 + sprev * sprev - coeff * sprev * sprev2;

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
    int m_length = -1;
    BufferIODouble *m_pInput = nullptr;
    BufferIODouble *m_pInput2 = nullptr;
    BufferIODouble *m_pInput4 = nullptr;

    float m_sampleRate = 0;

    int m_minNote, m_maxNote;

    void init(int length)
    {
        m_length = length;
        m_pInput = new BufferIODouble(m_length);
        m_pInput2 = new BufferIODouble(m_length/2);
        m_pInput4 = new BufferIODouble(m_length/4);

        m_pOutput = new BufferIODouble(getBins() );
        m_pOutput->clear();
    }

    void deinit()
    {
        delete(m_pInput);
        delete(m_pInput2);
        delete(m_pInput4);
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

    virtual const char *GetName() const {  return "Goertzel"; };

    void init(int length, float sampleRate)
    {
        m_sampleRate = sampleRate;
        deinit();
        init(length);
    }

    void setMaxMinNotes(int minNote, int maxNote)
    {
        m_minNote = minNote;
        m_maxNote = maxNote+1;
    }

    int getBins() const { return (m_maxNote-m_minNote); }
    int getProcessedLength() const { return m_length; }

    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length)
    {
        float *pInputBuf = m_pInput->GetData();
        for (int i = 0; i < length; i++)
        {
            float val = Uint16ToFloat(&input[i]);

            int ii= i + offsetDest;
            val *= hamming(ii);

            pInputBuf[ii] = val;
        }
    }

    void Decimate(BufferIODouble *pOutBuf, BufferIODouble *pInBuf)
    {
        float *pIn = pInBuf->GetData();
        float *pOut = pOutBuf->GetData();

        assert((pOutBuf->GetSize()*2) == pInBuf->GetSize());

        for(int i=0;i<pOutBuf->GetSize();i++)
        {
            pOut[i] = (pIn[2*i] + pIn[2*i+1])/2;
        }
    }

    void computePower(float decay)
    {
        float *powerBuf = m_pOutput->GetData();

        float sampleRate = m_sampleRate;
        int length =m_length;

        BufferIODouble *pInput = m_pInput;
        Decimate(m_pInput2, m_pInput); sampleRate/=2; length/=2; pInput = m_pInput2;
        Decimate(m_pInput4, m_pInput2); sampleRate/=2; length/=2; pInput = m_pInput4;

        assert(getBins() == m_pOutput->GetSize() );
        for(int i=0;i<m_pOutput->GetSize();i++)
        {
            float frequency = NoteToFreq(m_minNote + i);
            float power = sqrt(goertzelFilter(pInput->GetData(), length, frequency, sampleRate));

            powerBuf[i] = powerBuf[i] *decay + power*(1.0f-decay);

        }
    }

    float bin2Freq(int bin) const
    {
        return NoteToFreq(bin +1);
    }

    float freq2Bin(float freq) const
    {
        return freq;
        //return FreqToNote(freq)-1;
    }
};

#endif