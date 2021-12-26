
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

float sampleRate = 48000.0f;
float timeOverlap = .5f; //in seconds
float decay = .5f; //in seconds

void GetBufferQueues(float *pSampleRate, AudioQueue **pFreeQ, AudioQueue **pRecQ);

int barsHeight = 500;
int waterFallRaw = barsHeight;

static double now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000. + tv.tv_usec/1000.;
}

#include <pthread.h>
#include <semaphore.h>

struct Context
{
    pthread_attr_t attr;
    pthread_t worker;
    sem_t headwriteprotect;

    AndroidBitmapInfo  info;
    void*              pixels = nullptr;
    pthread_mutex_t    renderingLock;
    bool exit = false;

    int recordedChunks = 0;
    int processedChunks = 0;
    int droppedBuffers = 0;
    int iterationsPerChunk = 0;
    double millisecondsWaitingInLoopSemaphore = 0;
    double millisecondsProcessingChunk        = 0;

    AudioQueue* pFreeQueue = nullptr;       // user
    AudioQueue* pRecQueue = nullptr;        // user

    bool redoScale = false;
} context;

void ProcessChunk()
{
    int iterationsPerChunk = 0;

    // pass available buffers to processor
    {
        sample_buf *buf = nullptr;
        while (context.pRecQueue->front(&buf)) {
            context.pRecQueue->pop();
            if (chunker.pushAudioChunk(buf) == false) {
                context.droppedBuffers++;
            }
        }
    }

    //if we have enough data queued process the fft
    while (chunker.Process(pProcessor, decay, timeOverlap))
    {
        context.processedChunks++;

        if (pScale)
        {
            pthread_mutex_lock(&context.renderingLock);

            pScale->Build(pProcessor);

            // advance waterfall
            waterFallRaw -= 1;
            if (waterFallRaw < barsHeight)
            {
                waterFallRaw = context.info.height;
            }

            // draw line
            if (context.pixels != nullptr)
            {
                drawWaterFallLine(&context.info, waterFallRaw, context.pixels, pScale->GetBuffer());
            }

            pthread_mutex_unlock(&context.renderingLock);
        }

        iterationsPerChunk++;
    }

    context.iterationsPerChunk = iterationsPerChunk;


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
        delete (pProcessor);
        pProcessor = pProcessorDeferred;
        pProcessorDeferred = nullptr;

        pScale->PreBuild(pProcessor);
    }
}

void *loop( void *init)
{
    LOGE("loop()");

    double oldTime = now_ms();

    for(;;)
    {
        // wait for buffer
        sem_wait(&context.headwriteprotect);
        if (context.exit)
            break;

        double waitTime = now_ms();
        ProcessChunk();
        double currTime = now_ms();

        context.millisecondsWaitingInLoopSemaphore = waitTime - oldTime;
        context.millisecondsProcessingChunk        = currTime - waitTime;

        oldTime = currTime;
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

extern "C" JNIEXPORT jint JNICALL Java_com_example_plasma_Spectrogram_GetFftLength(JNIEnv * env, jclass obj)
{
    return pProcessor->getProcessedLength();
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetBarsHeight(JNIEnv * env, jclass obj, jint  barsHeight_)
{
    barsHeight = barsHeight_;
    waterFallRaw = barsHeight_;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetOverlap(JNIEnv * env, jclass obj, jfloat timeOverlap_)
{
    if (timeOverlap_<0.02f)
        timeOverlap_ = 0.02f;
    timeOverlap = timeOverlap_;
}

extern "C" JNIEXPORT jfloat JNICALL Java_com_example_plasma_Spectrogram_GetOverlap(JNIEnv * env, jclass obj)
{
    return timeOverlap;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetDecay(JNIEnv * env, jclass obj, jfloat decay_)
{
    decay = decay_;
}

extern "C" JNIEXPORT jfloat JNICALL Java_com_example_plasma_Spectrogram_GetDecay(JNIEnv * env, jclass obj)
{
    return decay;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetVolume(JNIEnv * env, jclass obj, jfloat volume_)
{
    pScale->setVolume(volume_);
}
extern "C" JNIEXPORT jfloat JNICALL Java_com_example_plasma_Spectrogram_GetVolume(JNIEnv * env, jclass obj)
{
    return pScale->getVolume();
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
    pthread_mutex_lock(&context.renderingLock);
    if (m_pHoldedData==nullptr)
        m_pHoldedData = new BufferIODouble(pScale->GetBuffer()->GetSize());

    m_pHoldedData->copy(pScale->GetBuffer());
    pthread_mutex_unlock(&context.renderingLock);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ClearHeldData(JNIEnv * env, jclass obj)
{
    pthread_mutex_lock(&context.renderingLock);
    free(m_pHoldedData);
    m_pHoldedData = nullptr;
    pthread_mutex_unlock(&context.renderingLock);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetScaler(JNIEnv * env, jclass obj, int screenWidth, double minFreq, double maxFreq, jboolean bLogX, jboolean bLogY)
{
    pthread_mutex_lock(&context.renderingLock);
    delete(pScale);
    pScale = GetScale(bLogX, bLogY);
    pScale->setOutputWidth(screenWidth, minFreq, maxFreq);
    pScale->PreBuild(pProcessor);
    pthread_mutex_unlock(&context.renderingLock);
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

extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_GetIterationsPerChunk(JNIEnv * env, jclass obj)
{
    return context.iterationsPerChunk;
}

extern "C" JNIEXPORT double JNICALL Java_com_example_plasma_Spectrogram_GetMillisecondsPerChunk(JNIEnv * env, jclass obj)
{
    return context.millisecondsProcessingChunk;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_example_plasma_Spectrogram_GetDebugInfo(JNIEnv * env, jclass obj)
{
    char sout[1024];

    sprintf(sout, "recordedChunks %i\nprocessedChunks %i\ndroppedBuffers %i\nrecordedBufSize %i\nfreeBufSize %i\niterations Per Chunk %i\ndroppedBuffers %i\nTime WaitingInLoopSemaphore %.1f ms\nTime ProcessingChunk %.2fms\n",
    context.recordedChunks,
    context.processedChunks,
    context.droppedBuffers,
    (context.pRecQueue!=nullptr)?context.pRecQueue->size():0,
    (context.pFreeQueue!=nullptr)?context.pFreeQueue->size():0,
    context.iterationsPerChunk,
    context.droppedBuffers,
    context.millisecondsWaitingInLoopSemaphore,
    context.millisecondsProcessingChunk);

    return env->NewStringUTF(sout);
}


extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_GetDroppedFrames(JNIEnv * env, jclass obj)
{
    static int lastDroppedFrames = 0;
    int dp = context.droppedBuffers;
    int res = dp - lastDroppedFrames;
    lastDroppedFrames = dp;
    return res;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ConnectWithAudioMT(JNIEnv * env, jclass obj)
{
    LOGE("chunker.begin();");
    chunker.begin();

    SetRecorderCallback([](void* pCTX, uint32_t msg, void* pData) ->bool
    {

        context.recordedChunks++;
        GetBufferQueues(&sampleRate, &context.pFreeQueue, &context.pRecQueue);
        sem_post(&context.headwriteprotect);
        assert(msg==ENGINE_SERVICE_MSG_RECORDED_AUDIO_AVAILABLE);

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
    pthread_mutex_lock(&context.renderingLock);

    int ret;
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &context.info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &context.pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    pthread_mutex_unlock(&context.renderingLock);
}

extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_Lock(JNIEnv * env, jclass  obj, jobject bitmap)
{
    pthread_mutex_lock(&context.renderingLock);

    if (pScale!=nullptr) {
        drawSpectrumBars(&context.info, context.pixels, barsHeight, pScale->GetBuffer());

        if (m_pHoldedData != nullptr) {
            drawHeldData(&context.info, context.pixels, barsHeight, m_pHoldedData);
        }
    }

    AndroidBitmap_unlockPixels(env, bitmap);

    return waterFallRaw;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_Unlock(JNIEnv * env, jclass  obj, jobject bitmap)
{
    int ret;
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &context.info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &context.pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    pthread_mutex_unlock(&context.renderingLock);
}

#endif

