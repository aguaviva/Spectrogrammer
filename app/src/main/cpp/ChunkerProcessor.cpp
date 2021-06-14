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
        freeQueue->push(buf);
        audioFttQueueTotalSize -= AU_LEN(buf->cap_);
    }
    assert(audioFttQueueTotalSize==0);
    m_started = false;
}


void ChunkerProcessor::setBuffers(AudioQueue *pRecQueue_, AudioQueue *freeQueue_)
{
    pRecQueue = pRecQueue_;
    freeQueue = freeQueue_;
}

bool ChunkerProcessor::getAudioChunk() {

    std::lock_guard<std::mutex> lock(getMutex());

    sample_buf *buf = nullptr;
    if (pRecQueue->front(&buf)) {
        pRecQueue->pop();
        assert(buf);

        //queue audio chunks
        audioFftQueue.push(buf);
        audioFttQueueTotalSize += AU_LEN(buf->cap_);
        return true;
    }
    return false;
}

void ChunkerProcessor::releaseUsedAudioChunks() {

    std::lock_guard<std::mutex> lock(getMutex());

    for (;;) {
        sample_buf *front = nullptr;
        audioFftQueue.front(&front);
        assert(front);
        int frontSize = AU_LEN(front->cap_);

        if (offset < frontSize)
            break;

        audioFftQueue.pop();
        freeQueue->push(front);
        offset -= frontSize;
        audioFttQueueTotalSize -= frontSize;
    }
}

void ChunkerProcessor::PrepareBuffer(Processor *pSpectrum)
{
    int dataToWrite = pSpectrum->getProcessedLength();

    int i = 0;
    int destOffset = 0;
    sample_buf *buf = nullptr;

    bool res = audioFftQueue.peek(&buf, i++);
    assert(res==true);
    assert(buf);
    assert(buf->buf_);
    AU_FORMAT *ptrB0 = GetSampleData(buf) + offset;
    int toWrite = std::min(dataToWrite, (int) (AU_LEN(buf->cap_) - offset));
    assert(toWrite > 0);

    pSpectrum->convertShortToFFT(ptrB0, destOffset, toWrite);
    destOffset += toWrite;
    dataToWrite -= toWrite;

    while (dataToWrite > 0) {
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
