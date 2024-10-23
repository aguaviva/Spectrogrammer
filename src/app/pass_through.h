#ifndef PASSTHROUGH_H
#define PASSTHROUGH_H

#include <cassert>
#include "Processor.h"
#include "kiss_fftr.h"
#include "auformat.h"

class PassThrough : public Processor
{
    float m_sampleRate;
    BufferIODouble m_pInput_samples;
    BufferIODouble m_pOutput_data;

    void init(int nfft)
    { 
        m_pInput_samples.Resize(nfft);
        m_pOutput_data.Resize(nfft+1);
    }

public:
    ~PassThrough()
    {
    }

    virtual const char *GetName() const {  return "Pass Through"; };

    void init(int nfft, float sampleRate)
    {
        init(nfft);
        m_sampleRate = sampleRate;
    }

    int getBinCount() const { return m_pOutput_data.GetSize(); }

    int getProcessedLength() const { return m_pInput_samples.GetSize(); }

    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length)
    {
        assert(m_pInput_samples.GetSize()>=offsetDest+length);

        float *m_in_samples = m_pInput_samples.GetData();
        for (int i = 0; i < length; i++)
        {
            float val = Uint16ToFloat(&input[i]);

            int ii= i + offsetDest;
            assert(ii < m_pInput_samples.GetSize());
            m_in_samples[ii] = val;
        }
    }

    void computePower(float decay)
    {
        float *out = m_pOutput_data.GetData();
        out[0]=-1; //this is the power
        for (int i = 0; i < getBinCount(); i++)
        {
            out[i+1] = m_pInput_samples.GetData()[i];
        }
    }

    float bin2Freq(int bin) const
    {
        return (float)(bin);
    }

    float freq2Bin(float freq) const
    {
        return (float)freq;
    }

    BufferIODouble *getBufferIO()
    {
        return &m_pOutput_data;
    }
};

#endif // PASSTHROUGH_H