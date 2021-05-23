#ifndef SCALEBUFFERBASE_H
#define SCALEBUFFERBASE_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include "fft.h"
#include "Processor.h"
#include "scale.h"
#include "BufferIO.h"

class ScaleBufferBase
{
public:
    BufferIODouble *m_pOutput = nullptr;
    float mVolume = 1;
public:
    virtual ~ScaleBufferBase() {};
    virtual void setOutputWidth(int outputWidth, float minFreq, float maxFreq) = 0;
    virtual void setVolume(float volume) { mVolume = volume; }
    virtual float getVolume() { return mVolume; }
    virtual float XtoFreq(float x) const = 0;
    virtual float FreqToX(float freq) const = 0;
    virtual void PreBuild(Processor *pProc) = 0;
    virtual void Build(Processor *pProc) = 0;

    float *GetData() const
    {
        return m_pOutput->GetData();
    }
};

#endif