#include "Processor.h"
#include "audio_common.h"
#include <mutex>

class ChunkerProcessor
{
    int offset = 0;
    AudioQueue audioFftQueue{5};
    int audioFttQueueTotalSize = 0;

    AudioQueue *pRecQueue, *freeQueue;

    std::mutex lock_;

    void PrepareBuffer(Processor *pSpectrum);
    AU_FORMAT *GetSampleData(sample_buf *b0)
    {
        return (AU_FORMAT *)b0->buf_;
    }

public:
    void setBuffers(AudioQueue *pRecQueue, AudioQueue *freeQueue);
    bool getAudioChunk();
    void releaseUsedAudioChunks();
    bool Process(Processor *pSpectrum, double decay, double timeOverlap);

    std::mutex &getMutex() { return lock_; }
};