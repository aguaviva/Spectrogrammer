#include "Processor.h"
#include "audio_common.h"
#include <mutex>

#define QUEUE_SIZE 32

class ChunkerProcessor
{
    bool m_started = false;
    int m_offset = 0;
    int m_srcOffset = 0;
    int m_destOffset = 0;
    int m_bufferIndex = 0;

    AudioQueue recQueue{QUEUE_SIZE};
    AudioQueue freeQueue{QUEUE_SIZE};

    bool PrepareBuffer(Processor *pSpectrum);
    AU_FORMAT *GetSampleData(sample_buf *b0)
    {
        return (AU_FORMAT *)b0->buf_;
    }

public:
    void begin();
    void end();
    void Reset();
    bool pushAudioChunk(sample_buf *buf);
    bool getFreeBufferFrontAndPop(sample_buf **buf);
    bool releaseUsedAudioChunks();
    bool Process(Processor *pSpectrum, double decay, double fractionOverlap);
};