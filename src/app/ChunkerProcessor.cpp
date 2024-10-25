#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "ChunkerProcessor.h"
#include "auformat.h"

void ChunkerProcessor::begin()
{
    assert(m_started == false);
    m_offset = 0;
    m_started = true;
    m_destOffset = 0;
    m_bufferIndex = 0;
}

void ChunkerProcessor::end()
{
    assert(m_started == true);
    m_started = false;
}

bool ChunkerProcessor::releaseUsedAudioChunks()
{
    assert(m_pRecQueue!=NULL);

    sample_buf *front = nullptr;
    while(m_pRecQueue->front(&front))
    {
        assert(front!=NULL);

        int frontSize = AU_LEN(front->cap_);

        if (m_offset < frontSize)
            return true;

        m_pRecQueue->pop();
        m_pFreeQueue->push(front);

        m_offset -= frontSize;
    }

    return false;
}

void ChunkerProcessor::SetQueues(AudioQueue *pRecQueue, AudioQueue *pFreeQueue)
{
    assert(pRecQueue!=NULL);
    assert(pFreeQueue!=NULL);

    m_pRecQueue = pRecQueue;
    m_pFreeQueue = pFreeQueue;
}

bool ChunkerProcessor::PrepareBuffer(Processor *pSpectrum)
{
    assert(pSpectrum!=NULL);

    int dataToWrite = pSpectrum->getProcessedLength();

    if (m_bufferIndex==0)
    {
        if (releaseUsedAudioChunks()==false)
            return false;

        m_srcOffset = m_offset;
    }
    
    sample_buf *buf = nullptr;
    while(m_pRecQueue->peek(&buf, m_bufferIndex))
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

bool ChunkerProcessor::Process(Processor *pSpectrum, double decay, double fractionOverlap)
{    
    if (PrepareBuffer(pSpectrum))
    {
        pSpectrum->computePower(decay);
        m_offset += pSpectrum->getProcessedLength() * (1.0f - fractionOverlap);

        return true;
    }

    return false;
}
