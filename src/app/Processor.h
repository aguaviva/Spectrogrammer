#ifndef PROCESSOR_H
#define PROCESSOR_H

#define _USE_MATH_DEFINES
#include "auformat.h"
#include "BufferIO.h"

class Processor
{
public:
    virtual ~Processor() { };
    virtual void init(int length, float sampleRate) = 0;
    virtual const char *GetName() const = 0;

    virtual int getProcessedLength() const = 0;
    virtual int getBinCount() const = 0;
    virtual void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length) = 0;
    virtual void computePower(float decay) = 0;
    virtual float bin2Freq(int bin) const = 0;
    virtual float freq2Bin(float freq) const = 0;

    virtual BufferIODouble *getBufferIO() = 0;
};

#endif