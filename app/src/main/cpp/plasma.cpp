
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "fft.h"
#include "Goertzel.h"
#include "colormaps.h"
#include "waterfall.h"
#include "scale.h"
#include "auformat.h"
#include "ScaleBufferBase.h"
#include "ScaleBuffer.h"
#include "ChunkerProcessor.h"
#include "BufferAverage.h"

#undef ANDROID
#define ANDROID

#ifdef ANDROID
#include "audio_common.h"
#include <jni.h>
#include <ctime>
#include <algorithm>
#include <android/log.h>
#include <android/bitmap.h>
#else
#define LOGI(a,...) printf(a "\n", __VA_ARGS__);
#endif

////////////////////////////////////////////////////////////////////

#ifdef ANDROID
Processor *pProcessorDeferred = nullptr;
Processor *pProcessor = nullptr;
ScaleBufferBase *pScale = nullptr;
BufferIODouble *m_pHoldedData = nullptr;

ChunkerProcessor chunker;

BufferAverage bufferAverage;

float sampleRate = 48000.0f;

void GetBufferQueues(float *pSampleRate, AudioQueue **pFreeQ, AudioQueue **pRecQ);

static double now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000. + tv.tv_usec/1000.;
}

#include <pthread.h>
#include <semaphore.h>

struct PerfCounters
{
    // perf counters
    uint32_t recordedChunks = 0;
    uint32_t processedChunks = 0;
    uint32_t droppedBuffers = 0;
    uint32_t iterationsPerChunk = 0;
};

struct Context
{
    pthread_attr_t attr;
    pthread_t worker;
    sem_t headwriteprotect;

    AndroidBitmapInfo  info;
    void*              pixels = nullptr;
    pthread_mutex_t    scaleLock;
    bool exit = false;

    float volume = 1;

    PerfCounters perfCounters;

    double millisecondsWaitingInLoopSemaphore = 0;
    double millisecondsProcessingChunk        = 0;

    AudioQueue* pFreeQueue = nullptr;       // user
    AudioQueue* pRecQueue = nullptr;        // user

    float fractionOverlap = .5f; // 0 to 1
    float decay = .5f;
    int barsHeight = 500;
    int waterFallRaw = barsHeight;


    bool redoScale = false;
};

Context context;

void ProcessChunk()
{
    int iterationsPerChunk = 0;

    // pass available buffers to processor
    {
        sample_buf *buf = nullptr;
        while (context.pRecQueue->front(&buf))
        {
            context.pRecQueue->pop();
            if (chunker.pushAudioChunk(buf) == false)
            {
                context.perfCounters.droppedBuffers++;
            }
        }
    }

    //if we have enough data queued process the fft
    while (chunker.Process(pProcessor, context.decay, context.fractionOverlap))
    {
        BufferIODouble *bufferIO = pProcessor->getBufferIO();

        bufferIO = bufferAverage.Do(bufferIO);
        if (bufferIO!=nullptr)
        {
            context.perfCounters.processedChunks++;

            if (pScale) {
                pthread_mutex_lock(&context.scaleLock);
                //LOGE("Begin DrawLine");

                pScale->Build(bufferIO, context.volume);

                // advance waterfall
                context.waterFallRaw -= 1;
                if (context.waterFallRaw < context.barsHeight) {
                    context.waterFallRaw = context.info.height-1;
                }

                // draw line
                if (context.pixels != nullptr) {
                    drawWaterFallLine(&context.info, context.waterFallRaw, context.pixels, pScale->GetBuffer());
                }

                //LOGE("End   DrawLine");
                pthread_mutex_unlock(&context.scaleLock);
            }
        }

        iterationsPerChunk++;
    }

    context.perfCounters.iterationsPerChunk = iterationsPerChunk;

    // return processed buffers
    {
        sample_buf *buf = nullptr;
        while (chunker.getFreeBufferFrontAndPop(&buf)) {
            context.pFreeQueue->push(buf);
        }
    }

    // if processor changed swap it now
    //
    if (pProcessorDeferred != nullptr)
    {
        pthread_mutex_lock(&context.scaleLock);
        delete pProcessor;
        pProcessor = pProcessorDeferred;
        pProcessorDeferred = nullptr;

        chunker.Reset();

        pScale->PreBuild(pProcessor);
        pthread_mutex_unlock(&context.scaleLock);
    }
}

void *loop( void *init)
{
    LOGE("loop()");

    for(;;)
    {
        // wait for buffer
        double sem_start = now_ms();
        sem_wait(&context.headwriteprotect);
        double sem_stop = now_ms();
        if (context.exit)
            break;

        double chunk_start = now_ms();
        ProcessChunk();
        double chunk_stop = now_ms();

        context.millisecondsWaitingInLoopSemaphore = sem_stop - sem_start;
        context.millisecondsProcessingChunk        = chunk_stop - chunk_start;
    }

    return nullptr;
}

ScaleBufferBase *GetScale(bool logX, bool logY)
{
    if (logX && logY)
        return new ScaleBufferLogLog();
    if (!logX && !logY)
        return new ScaleBufferLinLin();
    if (logX)
        return new ScaleBufferLogLin();
    if (logY)
        return new ScaleBufferLinLog();

    return nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetProcessorFFT(JNIEnv * env, jclass obj, int length)
{
    myFFT *pFFT = new myFFT();
    pFFT->init(length, sampleRate);

    if (pProcessor==nullptr)
        pProcessor = pFFT;
    else
        pProcessorDeferred = pFFT;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetProcessorGoertzel(JNIEnv * env, jclass obj, int length)
{
    Goertzel *pGoertzel = new Goertzel();
    pGoertzel->setMaxMinNotes(1, 88);
    pGoertzel->init(length, sampleRate);

    if (pProcessor==nullptr)
        pProcessor = pGoertzel;
    else
        pProcessorDeferred = pGoertzel;
}

extern "C" JNIEXPORT jint JNICALL Java_com_example_plasma_Spectrogram_GetFftLength(JNIEnv * env, jclass obj)
{
    return pProcessor->getProcessedLength();
}

/////////////////////////////////////////////////// get/sets ///////////////////////////////////////////////////////


extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetBarsHeight(JNIEnv * env, jclass obj, jint  barsHeight_)
{
    context.barsHeight = barsHeight_;
    context.waterFallRaw = barsHeight_;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetOverlap(JNIEnv * env, jclass obj, jfloat timeOverlap_)
{
    if (timeOverlap_>0.98f)
        timeOverlap_ = 0.98f;

    context.fractionOverlap = timeOverlap_;
}

extern "C" JNIEXPORT jfloat JNICALL Java_com_example_plasma_Spectrogram_GetOverlap(JNIEnv * env, jclass obj)
{
    return context.fractionOverlap;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetDecay(JNIEnv * env, jclass obj, jfloat decay_)
{
    context.decay = decay_;
}

extern "C" JNIEXPORT jfloat JNICALL Java_com_example_plasma_Spectrogram_GetDecay(JNIEnv * env, jclass obj)
{
    return context.decay;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetVolume(JNIEnv * env, jclass obj, jfloat volume_)
{
    context.volume=volume_;
}
extern "C" JNIEXPORT jfloat JNICALL Java_com_example_plasma_Spectrogram_GetVolume(JNIEnv * env, jclass obj)
{
    return context.volume;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetAverageCount(JNIEnv * env, jclass obj, jint c)
{
    bufferAverage.setAverageCount(c);
}
extern "C" JNIEXPORT jint JNICALL Java_com_example_plasma_Spectrogram_GetAverageCount(JNIEnv * env, jclass obj)
{
    return bufferAverage.getAverageCount();
}

extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_FreqToX(JNIEnv * env, jclass obj, double freq)
{
    return pScale->FreqToX(freq);
}

extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_XToFreq(JNIEnv * env, jclass obj, double x)
{
    return pScale->XtoFreq(x);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_HoldData(JNIEnv * env, jclass obj)
{
    pthread_mutex_lock(&context.scaleLock);
    if (m_pHoldedData==nullptr)
        m_pHoldedData = new BufferIODouble(pScale->GetBuffer()->GetSize());

    m_pHoldedData->copy(pScale->GetBuffer());
    pthread_mutex_unlock(&context.scaleLock);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ClearHeldData(JNIEnv * env, jclass obj)
{
    pthread_mutex_lock(&context.scaleLock);
    free(m_pHoldedData);
    m_pHoldedData = nullptr;
    pthread_mutex_unlock(&context.scaleLock);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ResetScanline(JNIEnv * env, jclass obj)
{
    pthread_mutex_lock(&context.scaleLock);
    context.waterFallRaw = context.info.height;
    pthread_mutex_unlock(&context.scaleLock);
}

/////////////////////////////////////////////////// Perf counter ///////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetScaler(JNIEnv * env, jclass obj, int screenWidth, double minFreq, double maxFreq, jboolean bLogX, jboolean bLogY)
{
    pthread_mutex_lock(&context.scaleLock);
    LOGE("Begin SetScaler");

    delete(pScale);
    pScale = GetScale(bLogX, bLogY);
    pScale->setOutputWidth(screenWidth, minFreq, maxFreq);
    pScale->PreBuild(pProcessor);

    LOGE("End   SetScaler");
    pthread_mutex_unlock(&context.scaleLock);
}

extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_GetIterationsPerChunk(JNIEnv * env, jclass obj)
{
    return context.perfCounters.iterationsPerChunk;
}

extern "C" JNIEXPORT double JNICALL Java_com_example_plasma_Spectrogram_GetMillisecondsPerChunk(JNIEnv * env, jclass obj)
{
    return context.millisecondsProcessingChunk;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_example_plasma_Spectrogram_GetDebugInfo(JNIEnv * env, jclass obj)
{
    char sout[1024];
    char *pOut = sout;
    static double last_time = now_ms();

    PerfCounters *pC = &context.perfCounters;
    pOut += sprintf(pOut,"Buffers:\n");
    pOut += sprintf(pOut," - Recorded %i\n", pC->recordedChunks);
    pOut += sprintf(pOut," - Dropped %i\n", pC->droppedBuffers);
    pOut += sprintf(pOut,"FFTs %i\n", pC->processedChunks);
    pOut += sprintf(pOut,"iterations Per Chunk %i\n", pC->iterationsPerChunk);
    pOut += sprintf(pOut,"Queue sizes:\n");
    pOut += sprintf(pOut,"- Recorded %i\n",(context.pRecQueue!=nullptr)?context.pRecQueue->size():0);
    pOut += sprintf(pOut,"- Free %i\n",(context.pFreeQueue!=nullptr)?context.pFreeQueue->size():0);
    pOut += sprintf(pOut,"Timings:\n");
    pOut += sprintf(pOut," - Waiting for audio %.1f ms\n", context.millisecondsWaitingInLoopSemaphore);
    pOut += sprintf(pOut," - Processing chunk %.2fms\n", context.millisecondsProcessingChunk);
    pOut += sprintf(pOut,"Progress: %i %%\n", bufferAverage.getProgress());

    return env->NewStringUTF(sout);
}


extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_GetDroppedFrames(JNIEnv * env, jclass obj)
{
    static int lastDroppedFrames = 0;
    int dp = context.perfCounters.droppedBuffers;
    int res = dp - lastDroppedFrames;
    lastDroppedFrames = dp;
    return res;
}

/////////////////////////////////////////////////// Connect/disconnect ///////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ConnectWithAudioMT(JNIEnv * env, jclass obj)
{
    LOGE("chunker.begin();");
    chunker.begin();

    SetRecorderCallback([](void* pCTX, uint32_t msg, void* pData) ->bool
    {
        assert(msg==ENGINE_SERVICE_MSG_RECORDED_AUDIO_AVAILABLE);

        context.perfCounters.recordedChunks++;
        GetBufferQueues(&sampleRate, &context.pFreeQueue, &context.pRecQueue);
        sem_post(&context.headwriteprotect);
        return true;
    });

    context.exit = false;
    sem_init(&context.headwriteprotect, 0, 0);
    pthread_attr_init(&context.attr);
    pthread_create(&context.worker, &context.attr, loop , nullptr);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_Disconnect(JNIEnv * env, jclass obj) {

    context.exit = true;
    sem_post(&context.headwriteprotect);
    pthread_join(context.worker, NULL);
    pthread_attr_destroy(&context.attr);
    sem_destroy(&context.headwriteprotect);

    LOGE("chunker.end();");
    chunker.end();

    AudioQueue* pFreeQueue = nullptr;
    AudioQueue* pRecQueue = nullptr;
    GetBufferQueues(&sampleRate, &pFreeQueue, &pRecQueue);

    sample_buf *buf = nullptr;
    while (chunker.getFreeBufferFrontAndPop(&buf)) {
        pFreeQueue->push(buf);
    }

    delete pProcessor;
    pProcessor = nullptr;

    delete pScale;
    pScale = nullptr;

    delete m_pHoldedData;
    m_pHoldedData = nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_Init(JNIEnv * env, jclass  obj, jobject bitmap)
{
    pthread_mutex_lock(&context.scaleLock);

    int ret;
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &context.info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &context.pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    pthread_mutex_unlock(&context.scaleLock);
}

extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_Lock(JNIEnv * env, jclass  obj, jobject bitmap)
{
    pthread_mutex_lock(&context.scaleLock);
//    LOGE("Begin Lock");

    if (pScale!=nullptr) {
        drawSpectrumBars(&context.info, context.pixels, context.barsHeight, pScale->GetBuffer());

        if (m_pHoldedData != nullptr) {
            drawHeldData(&context.info, context.pixels, context.barsHeight, m_pHoldedData);
        }
    }

    AndroidBitmap_unlockPixels(env, bitmap);
//    LOGE("End   Lock");

    return context.waterFallRaw;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_Unlock(JNIEnv * env, jclass  obj, jobject bitmap)
{
//    LOGE("Begin Unlock");
    int ret;
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &context.info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &context.pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

//    LOGE("End   Unlock");
    pthread_mutex_unlock(&context.scaleLock);
}

#endif

