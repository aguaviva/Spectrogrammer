/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "fftw3.h"
#include "colormaps.h"
#include "scale.h"
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

#define AU_FORMAT int16_t
#define AU_LEN(l) (l/2)
#define AU_CONVERT2DOUBLE(v) ((double)v)

double  convertToDecibels(double v, double ref)
{
    if (v<=0.001)
        return -120;

    return 20 * log10(v / ref);
}

double Uint16ToDouble(const AU_FORMAT *v)
{
    return AU_CONVERT2DOUBLE(*v);
}

AU_FORMAT DoubleToUint16(double v)
{
    v = v + 32767;
    return (AU_FORMAT)v;
}

double maxx(double a, double b)
{
    return (a>b)? a:b;
}

//////////////////////////////////////////////////////

#define REAL 0
#define IMAG 1

class myFFT {
    fftw_complex *m_in, *m_out;
    double *m_rout;
    int m_length;
    double m_fftScaling;
    fftw_plan m_plan;
public:

    myFFT(int length)
    {
        m_length = length;

        m_fftScaling = 0;
        for(int i=0;i<m_length;i++)
            m_fftScaling+=hamming(i);

        m_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_length);
        m_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_length);

        m_rout = (double*) fftw_malloc(sizeof(double) * m_length/2);
        for(int i=0;i<m_length/2;i++)
            m_rout[i]=0;

        m_plan = fftw_plan_dft_1d(m_length, m_in, m_out, FFTW_FORWARD, FFTW_ESTIMATE);
    }

    ~myFFT()
    {
        fftw_destroy_plan(m_plan);
        free(m_rout);
        fftw_free(m_in);
        fftw_free(m_out);
    }

    int getBins() const { return m_length/2; }

    double hanning(int i) const {
        return .50 - .50 * cos((2 * M_PI * i) / (m_length - 1.0));
    }

    double hamming(int i) const {
        return .54 - .46 * cos((2 * M_PI * i) / (m_length - 1.0));
    }

    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length)
    {
        for (int i = 0; i < length; i++)
        {
            double val = Uint16ToDouble(&input[i]);

            int ii= i + offsetDest;
            val *= hamming(ii);

            m_in[ii][REAL] = val;
            m_in[ii][IMAG] = 0;
        }
    }

    void doFFT()
    {
        fftw_execute(m_plan);
    }

    void computePowerFFT(float decay)
    {
        double totalPower = 0;

        for (int i = 0; i < getBins(); i++)
        {
            double power = sqrt(m_out[i][REAL] * m_out[i][REAL] + m_out[i][IMAG] * m_out[i][IMAG]);
            power *= (2 / m_fftScaling);
            m_rout[i] = m_rout[i] *decay + power*(1.0f-decay);

            totalPower += power;
        }
    }

    double getData(int i) const
    {
        if (i<getBins())
            return m_rout[i];
        else
            return 0;
    }

    int getFFTLength() const
    {
        return m_length;
    }

    double bin2Freq(float sampleRate, int bin) const
    {
        if (bin==0)
            return 0;

        return (sampleRate * (double)bin) / (double)getFFTLength();
    }

    double freq2Bin(float sampleRate, double freq) const
    {
        if (freq==0)
            return 0;

        return (freq * (double)getFFTLength()) / sampleRate;
    }
};

////////////////////////////////////////////////////////////////////

class ScaleBuffer
{
    myFFT *m_pFFT;
    int m_width;
    double *m_data;
    double m_sampleRate;
    int m_binCount;
    bool m_useLogY = true;

    Scale scaleXtoFreq;

public:
    ScaleBuffer(myFFT *pFFT, int width, double sampleRate)
    {
        m_pFFT = pFFT;
        m_width = width;
        m_sampleRate = sampleRate;
        m_data = (double *)malloc(sizeof(double) * m_width);
        m_binCount = pFFT->getBins();

        initScale(width);
    }

    ~ScaleBuffer()
    {
        free(m_data);
    }

    void initScale(int width)
    {
        scaleXtoFreq.init(width,100,  m_sampleRate/2);
    }

    void SetFrequencyLogarithmicAxis(bool bLogarithmic)
    {
        scaleXtoFreq.setLogarithmic(bLogarithmic);
    }

    bool GetFrequencyLogarithmicAxis() const
    {
        return scaleXtoFreq.getLogarithmic();
    }

    int FreqToBin(double freq) const
    {
        double t = unlerp( 1, m_sampleRate/2.0f, freq);
        return lerp( t, 1, m_binCount);
    }

    double XtoFreq(double x) const
    {
        return scaleXtoFreq.forward(x);
    }

    double FreqToX(double freq) const
    {
        return scaleXtoFreq.backward(freq);
    }

    void Build(double min, double max)
    {
        //set bins to min value
        for(int i=0; i < m_width; i++)
            m_data[i]=0;

        // set X axis
        int x=0;
        int bin = FreqToBin(XtoFreq(x));
        m_data[x]=m_pFFT->getData(bin);

        for(; x < m_width; x++)
        {
            int binNext = FreqToBin(XtoFreq(x+1));

            if (binNext==bin)
            {
                m_data[x]=m_data[x-1];
            }
            else
            {
                double v = 0;
                for (int i = bin; i < binNext; i++)
                {
                    v = maxx(m_pFFT->getData(i), v);
                }
                m_data[x]=v;
            }

            bin = binNext;
        }

        // set Y axis
        if (m_useLogY)
        {
            // log Y axis
            for (int i = 0; i < m_width; i++)
            {
                const double ref = 32768;
                m_data[i] = convertToDecibels(m_data[i], ref);
            }
        }
    }

    double GetNormalizedData(int i) const
    {
        if (m_useLogY)
        {
            double t = unlerp( -120, -20, m_data[i]);
            t = clamp(t, 0, 1);
            return t;
        }
        else
        {
            double t = unlerp( 0, 32768/20, m_data[i]);
            t = clamp(t, 0, 1);
            return t;
        }
    }

    int GetLength() const
    {
        return m_width;
    }
};

#ifdef ANDROID

myFFT* pSpectrum = nullptr;
ScaleBuffer *pScaleLog = nullptr;
//--------------------------------Draw spectrum--------------------------------

uint16_t GetColor(int x)
{
    return GetMagma(pScaleLog->GetNormalizedData(x)*255);
}

void drawWaterFallLine(AndroidBitmapInfo*  info, int yy, void*  pixels)
{
    uint16_t* line = (uint16_t*)((char*)pixels + info->stride*yy);

    /* optimize memory writes by generating one aligned 32-bit store
     * for every pair of pixels.
     */
    uint16_t*  line_end = line + info->width;
    uint32_t x = 0;

    if (line < line_end)
    {
        if (((uint32_t)(uintptr_t)line & 3) != 0)
        {
            line[0] = GetColor(x++);
            line++;
        }

        while (line + 2 <= line_end)
        {
            uint32_t  pixel;
            pixel = (uint32_t)GetColor(x++);
            pixel <<=16;
            pixel |= (uint32_t)GetColor(x++);
            ((uint32_t*)line)[0] = pixel;
            line += 2;
        }

        if (line < line_end)
        {
            line[0] = GetColor(x++);
            line++;
        }
    }
}

void drawSpectrumBar(AndroidBitmapInfo*  info, uint16_t *line, double val, int height)
{
    int y = 0;

    //background
    for (int yy=0;yy<val;yy++)
    {
        line[y] = 0x00ff;
        y += info->stride/2;
    }

    // bar
    for (int yy=val;yy<height;yy++)
    {
        line[y] = 0xffff;
        y += info->stride/2;
    }
}

void drawSpectrumBars(AndroidBitmapInfo*  info, void*  pixels, int height)
{
    uint16_t *line = (uint16_t *)pixels;
    for (int x=0;x<info->width;x++)
    {
        double power = pScaleLog->GetNormalizedData(x);
        drawSpectrumBar(info, line, (1.0-power) * height, height);
        line+=1;
    }
}

AU_FORMAT *GetSampleData(sample_buf *b0)
{
    return (AU_FORMAT *)b0->buf_;
}

float sampleRate = 48000.0f;
float timeOverlap = .5f; //in seconds
float decay = .5f; //in seconds

AudioQueue* freeQueue = nullptr;       // user
AudioQueue* recQueue = nullptr;        // user

void GetBufferQueues(float *pSampleRate, AudioQueue **pFreeQ, AudioQueue **pRecQ);

void SetBufQueues(float sampleRate_, AudioQueue* freeQ, AudioQueue* recQ) {
    assert(freeQ && recQ);
    freeQueue = freeQ;
    recQueue = recQ;
    sampleRate = sampleRate_;
}

int offset = 0;
sample_buf *q[2] = { nullptr, nullptr};
int  barsHeight = 500;
int  waterFallRaw = barsHeight;
bool bRedoScale = false;

AudioQueue audioFttQueue(32);
int audioFttQueueTotalSize=0;

static void PrepareBuffer(const int chunkOffset)
{
    int dataToWrite = pSpectrum->getFFTLength();

    int i=0;
    int destOffset = 0;
    sample_buf *buf = nullptr;

    audioFttQueue.peek(&buf, i++);
    assert(buf);
    AU_FORMAT *ptrB0 = GetSampleData(buf) + chunkOffset;
    int toWrite = std::min(dataToWrite, (int)(AU_LEN(buf->cap_) - chunkOffset));
    assert(toWrite>0);

    pSpectrum->convertShortToFFT(ptrB0, destOffset, toWrite);
    destOffset += toWrite;
    dataToWrite -= toWrite;

    while(dataToWrite>0) {
        buf = nullptr;
        audioFttQueue.peek(&buf, i++);
        assert(buf);
        ptrB0 = GetSampleData(buf);
        toWrite = std::min(dataToWrite, (int)(AU_LEN(buf->cap_)));

        pSpectrum->convertShortToFFT(ptrB0, destOffset, toWrite);
        destOffset += toWrite;
        dataToWrite -= toWrite;
    }
}

static void fill_plasma( AndroidBitmapInfo*  info, void*  pixels) {
    if ((pScaleLog == nullptr) || (pScaleLog->GetLength() != info->width) || (bRedoScale == true)) {
        bool log;
        if (pScaleLog!=nullptr) {
            log = pScaleLog->GetFrequencyLogarithmicAxis();
            delete pScaleLog;
        }
        pScaleLog = new ScaleBuffer(pSpectrum, info->width, sampleRate);
        pScaleLog->SetFrequencyLogarithmicAxis(log);
        bRedoScale = false;
    }

    bool bUpdateBars = false;

    sample_buf *buf = nullptr;
    while(recQueue->front(&buf))
    {
        recQueue->pop();

        //queue audio chunks
        audioFttQueue.push(buf);
        audioFttQueueTotalSize += AU_LEN(buf->cap_);

        //if we have enough data queued process the fft
        while (audioFttQueueTotalSize >= pSpectrum->getFFTLength() + offset)
        {
            PrepareBuffer(offset);

            // advance waterfall
            waterFallRaw-=1;
            if (waterFallRaw < barsHeight)
                waterFallRaw=info->height;

            pSpectrum->doFFT();
            pSpectrum->computePowerFFT(decay);
            pScaleLog->Build(0,0);
            drawWaterFallLine(info, waterFallRaw, pixels);
            bUpdateBars = true;


            offset += pSpectrum->getFFTLength()*timeOverlap;

            {
                // return processed chunks
                sample_buf *front = nullptr;
                audioFttQueue.front(&front);
                assert(front);
                int frontSize = AU_LEN(front->cap_);
                while (offset >= frontSize) {
                    audioFttQueue.pop();
                    freeQueue->push(front);
                    offset -= frontSize;
                    audioFttQueueTotalSize -= frontSize;
                }
            }
        }
    }

    if (bUpdateBars) {
        drawSpectrumBars(info, pixels, barsHeight);
    }
}


extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ConnectWithAudio(JNIEnv * env, jclass obj)
{
    GetBufferQueues(&sampleRate, &freeQueue, &recQueue);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetFftLength(JNIEnv * env, jclass obj, jint  fftLength)
{
    delete pSpectrum;
    pSpectrum = new myFFT(fftLength);
    bRedoScale = true;
}

extern "C" JNIEXPORT jint JNICALL Java_com_example_plasma_Spectrogram_GetFftLength(JNIEnv * env, jclass obj)
{
    return pSpectrum->getFFTLength();
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_SetBarsHeight(JNIEnv * env, jclass obj, jint  barsHeight_)
{
    barsHeight=barsHeight_;
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
    if (pScaleLog==nullptr)
        return;

    pScaleLog->SetFrequencyLogarithmicAxis(bLogarithmic);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_example_plasma_Spectrogram_GetFrequencyLogarithmicAxis(JNIEnv * env, jclass obj, jboolean bLogarithmic)
{
    if (pScaleLog==nullptr)
        return false;

    return pScaleLog->GetFrequencyLogarithmicAxis();
}

extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_FreqToX(JNIEnv * env, jclass obj, double freq)
{
    if (pScaleLog)
        return pScaleLog->FreqToX(freq);
    return 0;
}


extern "C" JNIEXPORT float JNICALL Java_com_example_plasma_Spectrogram_XToFreq(JNIEnv * env, jclass obj, double x)
{
    if (pScaleLog)
        return pScaleLog->XtoFreq(x);

    return 0;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_example_plasma_Spectrogram_SetMinMaxFreqs(JNIEnv * env, jclass obj, double min, double max)
{
    if (pScaleLog) {
        //pScaleLog->SetMinMax(min, max);
    }

    return pScaleLog!=nullptr;
}


extern "C" JNIEXPORT int JNICALL Java_com_example_plasma_Spectrogram_Update(JNIEnv * env, jclass  obj, jobject bitmap,  jlong  time_ms)
{
    AndroidBitmapInfo  info;
    void*              pixels;
    int                ret;
    static int         init;

    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return -1;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGB_565) {
        LOGE("Bitmap format is not RGB_565 !");
        return -1;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    /* Now fill the values with a nice little plasma */
    fill_plasma(&info, pixels);

    AndroidBitmap_unlockPixels(env, bitmap);

    return waterFallRaw;
}
#endif