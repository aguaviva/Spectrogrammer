/* 
  A Minimal Capture Program

  This program opens an audio interface for capture, configures it for
  stereo, 16 bit, 44.1kHz, interleaved conventional read/write
  access. Then its reads a chunk of random data from it, and exits. It
  isn't meant to be a real program.

  From on Paul David's tutorial : http://equalarea.com/paul/alsa-audio.html

  Fixes rate and buffer problems

  sudo apt-get install libasound2-dev
  gcc -o alsa-record-example -lasound alsa-record-example.c && ./alsa-record-example hw:0
*/
#ifndef ANDROID

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#define LOGW printf
#include "buf_manager.h"

static AudioQueue *freeQueue_;
static AudioQueue *recQueue_; 

#define BUF_COUNT 64
static int32_t buffer_frames = 1024;
static snd_pcm_t *capture_handle;
static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
static bool recording = false;
static pthread_t debug_capture_thread;

void Audio_init(unsigned int sampleRate, int framesPerBuf)
{
    int err;
    unsigned int rate = sampleRate;
    snd_pcm_hw_params_t *hw_params;
    const char *device_name = "default";

    if ((err = snd_pcm_open (&capture_handle, device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0) 
    {
        fprintf (stderr, "cannot open audio device %s (%s)\n", 
                    device_name,
                    snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) 
    {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) 
    {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
                    snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) 
    {
        fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) 
    {
        fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) 
    {
        fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) 
    {
        fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) 
    {
        fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
        exit (1);
    }

    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (capture_handle)) < 0) 
    {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
        exit (1);
    }

    uint32_t bufSize = buffer_frames * 1 * 16;//engine.fastPathFramesPerBuf_ * engine.sampleChannels_ * engine.bitsPerSample_;
    bufSize = (bufSize + 7) >> 3;  // bits --> byte
    uint32_t bufCount_ = BUF_COUNT;
    sample_buf *bufs_ = allocateSampleBufs(bufCount_, bufSize);
    assert(bufs_);

    freeQueue_ = new AudioQueue(bufCount_);
    recQueue_ = new AudioQueue(bufCount_);
    assert(freeQueue_ && recQueue_);
    for (uint32_t i = 0; i < bufCount_; i++) 
    {
        freeQueue_->push(&bufs_[i]);
    }
}

void Audio_getBufferQueues(AudioQueue **pFreeQ, AudioQueue **pRecQ)
{
    *pFreeQ = freeQueue_;
    *pRecQ = recQueue_; 
}

static void * debug_capture_thread_fn( void * v )
{
    int err;
    while (recording)
    {
        sample_buf *bufs_;
        if (freeQueue_->front(&bufs_))
        {
            freeQueue_->pop();

            if ((err = snd_pcm_readi (capture_handle, bufs_->buf_, buffer_frames)) != buffer_frames) 
            {
                fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
                break;
            }
            recQueue_->push(bufs_);
        }
    }

    return NULL;
}

void Audio_startPlay()
{
    recording = true;

    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    pthread_create(&debug_capture_thread, &attr, debug_capture_thread_fn, NULL);
}

void Audio_deinit()
{
    recording = false;
    void *retval;
    pthread_join(debug_capture_thread, &retval);

    snd_pcm_close (capture_handle);
    fprintf(stdout, "audio interface closed\n");
}
#endif