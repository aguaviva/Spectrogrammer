#include "Processor.h"
#define LOGW(...)
#include "buf_manager.h"

#define QUEUE_SIZE 32

class ChunkerProcessor
{
    bool m_started = false;
    int m_offset = 0;
    int m_srcOffset = 0;
    int m_destOffset = 0;
    int m_bufferIndex = 0;

    AudioQueue *m_pRecQueue = NULL;
    AudioQueue *m_pFreeQueue = NULL;

    bool PrepareBuffer(Processor *pSpectrum);
    AU_FORMAT *GetSampleData(sample_buf *b0)
    {
        return (AU_FORMAT *)b0->buf_;
    }

public:

    void begin();
    void end();
    void SetQueues(AudioQueue *pRecQueue, AudioQueue *pFreeQueue);
    bool releaseUsedAudioChunks();
    bool Process(Processor *pSpectrum, double decay, double fractionOverlap);
};