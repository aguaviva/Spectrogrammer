#ifndef PROCESSOR_H
#define PROCESSOR_H

#define _USE_MATH_DEFINES
#include <cmath>
#include "auformat.h"
#include "BufferIO.h"


class Processor
{
protected:
    BufferIODouble *m_pOutput = nullptr;

public:
    virtual ~Processor() { };
    virtual int getProcessedLength() const = 0;
    virtual int getBins() const = 0;
    virtual void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length) = 0;
    virtual void computePower(float decay) = 0;
    virtual double bin2Freq(int bin) const = 0;
    virtual double freq2Bin(double freq) const = 0;

    BufferIODouble *getBufferIO() const
    {
        return m_pOutput;
    }
};

#endif