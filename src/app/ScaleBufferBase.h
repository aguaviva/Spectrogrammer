#ifndef SCALEBUFFERBASE_H
#define SCALEBUFFERBASE_H

#include <algorithm>
#include "fft.h"
#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"

class ScaleBufferBase
{
public:
    BufferIODouble *m_pOutput = nullptr;

public:
    virtual ~ScaleBufferBase()
    {
        delete(m_pOutput);
    };
    virtual void setOutputWidth(int outputWidth, float minFreq, float maxFreq) = 0;
    virtual float XtoFreq(float x) const = 0;
    virtual float FreqToX(float freq) const = 0;
    virtual void PreBuild(Processor *pProc) = 0;
    virtual void Build(BufferIODouble *bufferIO) = 0;

    BufferIODouble *GetBuffer() const
    {
        return m_pOutput;
    }

    float *GetData() const
    {
        return m_pOutput->GetData();
    }
};

#endif