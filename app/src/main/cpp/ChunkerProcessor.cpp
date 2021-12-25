#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "ChunkerProcessor.h"
#include "auformat.h"

void ChunkerProcessor::begin()
{
    assert(m_started == false);
    offset = 0;
    audioFttQueueTotalSize = 0;
    m_started = true;
}

void ChunkerProcessor::end()
{
    assert(m_started == true);
    sample_buf *buf = nullptr;
    while (audioFftQueue.front(&buf))
    {
        audioFftQueue.pop();
        assert(buf);

        //queue audio chunks
        {
            freeQueue.push(buf);
            std::lock_guard<std::mutex> lock(lock_pFreeQueue);
        }

        audioFttQueueTotalSize -= AU_LEN(buf->cap_);
    }
    assert(audioFttQueueTotalSize==0);
    m_started = false;
}

bool ChunkerProcessor::pushAudioChunk(sample_buf *buf)
{
    std::lock_guard<std::mutex> lock(lock_pRecQueue);

    bool res = recQueue.push(buf);
    assert(res);
    return true;
}

bool ChunkerProcessor::getAudioChunk()
{
    std::lock_guard<std::mutex> lock(lock_pRecQueue);

    sample_buf *buf = nullptr;
    if (recQueue.front(&buf))
    {
        recQueue.pop();
        assert(buf);

        //queue audio chunks
        bool res = audioFftQueue.push(buf);
        assert(res);
        audioFttQueueTotalSize += AU_LEN(buf->cap_);
        return true;
    }
    return false;
}

void ChunkerProcessor::releaseUsedAudioChunks()
{
    for (;;)
    {
        sample_buf *front = nullptr;
        audioFftQueue.front(&front);
        assert(front);
        int frontSize = AU_LEN(front->cap_);

        if (offset < frontSize)
            break;

        audioFftQueue.pop();
        {
            std::lock_guard<std::mutex> lock(lock_pFreeQueue);
            freeQueue.push(front);
        }
        offset -= frontSize;
        audioFttQueueTotalSize -= frontSize;
    }
}

bool ChunkerProcessor::getFreeBufferFrontAndPop(sample_buf **buf)
{
    std::lock_guard<std::mutex> lock(lock_pFreeQueue);
    if (freeQueue.front(buf))
    {
        freeQueue.pop();
        return true;
    }
    return false;
}

void ChunkerProcessor::PrepareBuffer(Processor *pSpectrum)
{
    int dataToWrite = pSpectrum->getProcessedLength();

    int i = 0;
    int destOffset = 0;
    sample_buf *buf = nullptr;
    bool res = audioFftQueue.peek(&buf, i++);
    assert(res);
    assert(buf);
    assert(buf->buf_);
    AU_FORMAT *ptrB0 = GetSampleData(buf) + offset;
    int toWrite = std::min(dataToWrite, (int) (AU_LEN(buf->cap_) - offset));
    assert(toWrite > 0);

    pSpectrum->convertShortToFFT(ptrB0, destOffset, toWrite);
    destOffset += toWrite;
    dataToWrite -= toWrite;

    while (dataToWrite > 0)
    {
        buf = nullptr;
        audioFftQueue.peek(&buf, i++);
        assert(buf);
        ptrB0 = GetSampleData(buf);
        toWrite = std::min(dataToWrite, (int) (AU_LEN(buf->cap_)));

        pSpectrum->convertShortToFFT(ptrB0, destOffset, toWrite);
        destOffset += toWrite;
        dataToWrite -= toWrite;
    }
}

bool ChunkerProcessor::Process(Processor *pSpectrum, double decay, double timeOverlap)
{
    if ((audioFttQueueTotalSize - offset) >= pSpectrum->getProcessedLength())
    {
        PrepareBuffer(pSpectrum);
        pSpectrum->computePower(decay);

        offset += pSpectrum->getProcessedLength() * timeOverlap;

        releaseUsedAudioChunks();

        return true;
    }

    return false;
}
