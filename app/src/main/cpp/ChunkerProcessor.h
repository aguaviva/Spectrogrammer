#include "Processor.h"
#include "audio_common.h"
#include <mutex>

#define QUEUE_SIZE 10

class ChunkerProcessor
{
    bool m_started = false;
    int offset = 0;
    AudioQueue audioFftQueue{QUEUE_SIZE};
    int audioFttQueueTotalSize = 0;

    AudioQueue recQueue{QUEUE_SIZE};
    AudioQueue freeQueue{QUEUE_SIZE};

    std::mutex lock_pRecQueue;
    std::mutex lock_pFreeQueue;

    void PrepareBuffer(Processor *pSpectrum);
    AU_FORMAT *GetSampleData(sample_buf *b0)
    {
        return (AU_FORMAT *)b0->buf_;
    }

public:
    void begin();
    void end();
    bool pushAudioChunk(sample_buf *buf);
    bool getAudioChunk();
    bool getFreeBufferFrontAndPop(sample_buf **buf);
    void releaseUsedAudioChunks();
    bool Process(Processor *pSpectrum, double decay, double timeOverlap);
};