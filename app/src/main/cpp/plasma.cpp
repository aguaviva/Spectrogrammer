
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "fft.h"
#include "colormaps.h"
#include "waterfall.h"
#include "scale.h"
#include "auformat.h"
#include "scalebuffer.h"
#include "ChunkerProcessor.h"

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

myFFT spectrum;
ScaleBuffer scaleLog;
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


static void DrawWaterfallLine( AndroidBitmapInfo*  info, void*  pixels)
{
    waterFallRaw-=1;
    if (waterFallRaw < barsHeight)
        waterFallRaw=info->height;

    drawWaterFallLine(info, waterFallRaw, pixels, &scaleLog);
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

    AndroidBitmapInfo  info;
    void*              pixels;
    pthread_mutex_t    lock;
    bool exit = false;
} context;

void ProcessChunk()
{
    while(chunker.getAudioChunk()) {

        double oldTime = now_ms();
        int iterationsPerChunk = 0;

        //if we have enough data queued process the fft
        while (chunker.Process(&spectrum, decay, timeOverlap)) {
            pthread_mutex_lock(&context.lock);

            scaleLog.Build(0, 0);

            // advance waterfall
            waterFallRaw -= 1;
            if (waterFallRaw < barsHeight)
                waterFallRaw = context.info.height;

            // draw line
            drawWaterFallLine(&context.info, waterFallRaw, context.pixels, &scaleLog);

            if (fftLength != spectrum.getFFTLength()) {
                spectrum.setFFTLength(fftLength);
            }

            pthread_mutex_unlock(&context.lock);

            iterationsPerChunk++;
        }

        double currTime = now_ms();
        LOGI("Process, iterations: %i (%fms) hop: %i", iterationsPerChunk, (currTime - oldTime), (int)(spectrum.getFFTLength() * timeOverlap));
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
        LOGI("waiting: %f  process: %f    fifo:(%i / %i)", (waitTime - oldTime), (currTime - waitTime), context.recQueue.size(), context.freeQueue.size());
        oldTime = currTime;
    }
    return nullptr;
}
#endif

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetFftLength(JNIEnv * env, jclass obj, jint  fftLength_)
{
    fftLength = fftLength_;
}

extern "C" JNIEXPORT jint JNICALL Java_com_example_plasma_Spectrogram_GetFftLength(JNIEnv * env, jclass obj)
{
    return spectrum.getFFTLength();
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

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetFrequencyLogarithmicAxis(JNIEnv * env, jclass obj, jboolean bLogarithmic)
{
    scaleLog.SetFrequencyLogarithmicAxis(bLogarithmic);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_example_plasma_Spectrogram_GetFrequencyLogarithmicAxis(JNIEnv * env, jclass obj, jboolean bLogarithmic)
{
    return scaleLog.GetFrequencyLogarithmicAxis();
}

extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_FreqToX(JNIEnv * env, jclass obj, double freq)
{
    return scaleLog.FreqToX(freq);
}

extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_XToFreq(JNIEnv * env, jclass obj, double x)
{
    return scaleLog.XtoFreq(x);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetMinMaxFreqs(JNIEnv * env, jclass obj, double min, double max)
{
    //scaleLog.SetMinMax(min, max);
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
    spectrum.setFFTLength(fftLength);
    scaleLog.SetSamplerate(sampleRate);
    scaleLog.SetFFT(&spectrum);

    if (scaleLog.GetLength() != context.info.width) {
        scaleLog.SetWidth(context.info.width);
    }

#ifdef MULTITHREADING
    SetRecorderCallback([](void* pCTX, uint32_t msg, void* pData) ->bool {

        {
            std::lock_guard<std::mutex> lock(chunker.getMutex());

            AudioQueue* pFreeQueue = nullptr;       // user
            AudioQueue* pRecQueue = nullptr;        // user
            GetBufferQueues(&sampleRate, &pFreeQueue, &pRecQueue);

            sample_buf *buf = nullptr;
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
    int split = waterFallRaw;
    pthread_mutex_lock(&context.lock);

    drawSpectrumBars(&context.info, context.pixels, barsHeight, &scaleLog);

    AndroidBitmap_unlockPixels(env, bitmap);

    return split;
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

