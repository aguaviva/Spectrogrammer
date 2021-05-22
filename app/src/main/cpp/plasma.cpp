
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

//WARNING: Single threaded path not working
#define MULTITHREADING
myFFT fft;
Goertzel goertzel;
Processor *pProcessor = &fft;
//Processor *pProcessor = &goertzel;

ScaleBufferBase *pScale = nullptr;

ChunkerProcessor chunker;

float sampleRate = 48000.0f;
float timeOverlap = .5f; //in seconds
float decay = .5f; //in seconds
int fftLength = -1;
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

    AudioQueue freeQueue{16};
    AudioQueue recQueue{16};

    int droppedFrames = 0;

    AndroidBitmapInfo  info;
    void*              pixels;
    pthread_mutex_t    lock;
    bool exit = false;

    int iterationsPerChunk = 0;
    double millisecondsPerChunk = 0;
    bool redoScale = false;
} context;

void ProcessChunk()
{
    while(chunker.getAudioChunk()) {

        double oldTime = now_ms();
        int iterationsPerChunk = 0;

        //if we have enough data queued process the fft
        while (chunker.Process(pProcessor, decay, timeOverlap)) {
            pthread_mutex_lock(&context.lock);

            pScale->Build(pProcessor);

            // advance waterfall
            waterFallRaw -= 1;
            if (waterFallRaw < barsHeight)
                waterFallRaw = context.info.height;

            // draw line
            drawWaterFallLine(&context.info, waterFallRaw, context.pixels, pScale);

            if (context.redoScale) {
                fft.init(fftLength, sampleRate);
                pScale->PreBuild(pProcessor);
                context.redoScale = false;
            }

            pthread_mutex_unlock(&context.lock);

            iterationsPerChunk++;
        }

        double currTime = now_ms();

        context.iterationsPerChunk = iterationsPerChunk;
        context.millisecondsPerChunk = currTime - oldTime;
    }
}

#ifdef MULTITHREADING
void * loop( void *init)
{
    double oldTime = now_ms();

    for(;;)
    {
        sem_wait(&context.headwriteprotect);
        if (context.exit)
            break;
        double waitTime = now_ms();

        ProcessChunk();

        double currTime = now_ms();
        //LOGI("waiting: %f  process: %f    fifo:(%i / %i)", (waitTime - oldTime), (currTime - waitTime), context.recQueue.size(), context.freeQueue.size());
        oldTime = currTime;
    }
    return nullptr;
}
#endif

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

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetFftLength(JNIEnv * env, jclass obj, jint  fftLength_)
{
    pthread_mutex_lock(&context.lock);
    fftLength = fftLength_;
    context.redoScale = true;
    pthread_mutex_unlock(&context.lock);
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

extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_FreqToX(JNIEnv * env, jclass obj, double freq)
{
    return pScale->FreqToX(freq);
}

extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_XToFreq(JNIEnv * env, jclass obj, double x)
{
    return pScale->XtoFreq(x);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetScaler(JNIEnv * env, jclass obj, int width, double min, double max, jboolean bLogX, jboolean bLogY)
{
    pthread_mutex_lock(&context.lock);
    delete(pScale);
    pScale = GetScale(bLogX, bLogY);
    pScale->setOutputWidth(width, min, max);
    pScale->PreBuild(pProcessor);
    pthread_mutex_unlock(&context.lock);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetProcessor(JNIEnv * env, jclass obj, int type)
{
    pthread_mutex_lock(&context.lock);
    if (type==0)
    {
        pProcessor = &fft;
    }
    else if (type==1)
    {
        pProcessor = &goertzel;
    }
    pthread_mutex_unlock(&context.lock);
}

extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_GetIterationsPerChunk(JNIEnv * env, jclass obj)
{
    return context.iterationsPerChunk;
}
extern "C" JNIEXPORT double JNICALL Java_com_example_plasma_Spectrogram_GetMillisecondsPerChunk(JNIEnv * env, jclass obj)
{
    return context.millisecondsPerChunk;
}

extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_GetDroppedFrames(JNIEnv * env, jclass obj)
{
    static int lastDroppedFrames = 0;
    int dp = context.droppedFrames;
    int res = dp - lastDroppedFrames;
    lastDroppedFrames = dp;
    return res;
}


extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ConnectWithAudioMT(JNIEnv * env, jclass obj, jobject bitmap)
{

#ifdef MULTITHREADING
    // if there is a thread running destroy it because we will create a new one
    static bool gotThread = false;
    if (gotThread)
    {
        context.exit = true;
        sem_post(&context.headwriteprotect);
        pthread_join(context.worker, NULL);
        pthread_attr_destroy(&context.attr);
    }
#endif

    //----------------------------------

    int ret;
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &context.info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &context.pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    //----------------------------------

    chunker.setBuffers(&context.recQueue, &context.freeQueue);
    fft.init(fftLength, sampleRate);
    goertzel.init(4096, sampleRate);
    pScale->PreBuild(pProcessor);

#ifdef MULTITHREADING
    SetRecorderCallback([](void* pCTX, uint32_t msg, void* pData) ->bool {

        {
            std::lock_guard<std::mutex> lock(chunker.getMutex());

            AudioQueue* pFreeQueue = nullptr;       // user
            AudioQueue* pRecQueue = nullptr;        // user
            GetBufferQueues(&sampleRate, &pFreeQueue, &pRecQueue);

            sample_buf *buf = nullptr;

            //drop frames
            while(context.recQueue.size()>2)
            {
                context.recQueue.front(&buf);
                context.recQueue.pop();
                context.freeQueue.push(buf);
                context.droppedFrames++;
            }

            while (pRecQueue->front(&buf)) {
                pRecQueue->pop();
                context.recQueue.push(buf);
            }

            while (context.freeQueue.front(&buf)) {
                context.freeQueue.pop();
                pFreeQueue->push(buf);
            }
        }

        assert(msg==ENGINE_SERVICE_MSG_RECORDED_AUDIO_AVAILABLE);
        sem_post(&context.headwriteprotect);
        return true;
    });

    context.exit = false;
    sem_init(&context.headwriteprotect, 0, 0);
    pthread_attr_init(&context.attr);
    pthread_create(&context.worker, &context.attr, loop , nullptr);
    gotThread = true;
#endif
}

extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_Lock(JNIEnv * env, jclass  obj, jobject bitmap)
{
#ifndef MULTITHREADING
    ProcessChunk();
#endif
    pthread_mutex_lock(&context.lock);

    drawSpectrumBars(&context.info, context.pixels, barsHeight, pScale);

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

    pthread_mutex_unlock(&context.lock);
}

#endif

