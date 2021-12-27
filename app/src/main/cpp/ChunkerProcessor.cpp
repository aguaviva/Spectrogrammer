#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "ChunkerProcessor.h"
#include "auformat.h"

void ChunkerProcessor::begin()
{
    assert(m_started == false);
    mOffset = 0;
    m_started = true;
}

void ChunkerProcessor::end()
{
    assert(m_started == true);

    sample_buf *buf = nullptr;
    while (recQueue.front(&buf))
    {
        recQueue.pop();
        assert(buf);

        freeQueue.push(buf);
    }

    m_started = false;
}

bool ChunkerProcessor::pushAudioChunk(sample_buf *buf)
{
    bool res = recQueue.push(buf);
    return res;
}

bool ChunkerProcessor::releaseUsedAudioChunks()
{
    sample_buf *front = nullptr;
    while(recQueue.front(&front))
    {
        int frontSize = AU_LEN(front->cap_);

        if (mOffset < frontSize)
            return true;

        recQueue.pop();
        freeQueue.push(front);

        mOffset -= frontSize;
    }

    return false;
}

bool ChunkerProcessor::getFreeBufferFrontAndPop(sample_buf **buf)
{
    if (freeQueue.front(buf))
    {
        freeQueue.pop();
        return true;
    }
    return false;
}

void ChunkerProcessor::Reset()
{
    m_destOffset = 0;
    m_bufferIndex = 0;
}

bool ChunkerProcessor::PrepareBuffer(Processor *pSpectrum)
{
    int dataToWrite = pSpectrum->getProcessedLength();

    if (m_bufferIndex==0)
    {
        if (releaseUsedAudioChunks()==false)
            return false;

        m_srcOffset = mOffset;
    }

    sample_buf *buf = nullptr;
    while(recQueue.peek(&buf, m_bufferIndex))
    {
        int bufSize = AU_LEN(buf->cap_);
        int srcBufLeft = bufSize - m_srcOffset;
        int destLeft = dataToWrite - m_destOffset;

        int toWrite = std::min(destLeft, srcBufLeft);

        AU_FORMAT *ptrB0 = GetSampleData(buf) + m_srcOffset;
        pSpectrum->convertShortToFFT(ptrB0, m_destOffset, toWrite);

        m_destOffset += toWrite;
        m_srcOffset += toWrite;

        if (m_srcOffset==bufSize)
        {
            m_srcOffset = 0;
            m_bufferIndex++;
        }

        if (m_destOffset == dataToWrite)
        {
            m_destOffset = 0;
            m_bufferIndex = 0;
            return true;
        }
    }

    return false;
}

bool ChunkerProcessor::Process(Processor *pSpectrum, double decay, double timeOverlap)
{
    if (PrepareBuffer(pSpectrum))
    {
        pSpectrum->computePower(decay);
        mOffset += pSpectrum->getProcessedLength() * timeOverlap;
        return true;
    }

    return false;
}
