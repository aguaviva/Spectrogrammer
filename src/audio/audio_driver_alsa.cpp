#ifdef ALSA_DRIVER

#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic>
#include <unistd.h>
#include <alsa/asoundlib.h>
#define LOGW printf
#include "buf_manager.h"

static AudioQueue *freeQueue_;
static AudioQueue *recQueue_; 

#define BUF_COUNT 64
static uint32_t bufCount_ = BUF_COUNT;
static sample_buf *bufs_;
static int32_t buffer_frames = 1024;
static snd_pcm_t *capture_handle;
static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
static std::atomic<bool> recording(false);
static unsigned int actual_sample_rate = 0;
static pthread_t capture_thread;
static bool thread_running = false;
static int wake_pipe[2] = {-1, -1};

static void close_wake_pipe()
{
    if (wake_pipe[0] >= 0) close(wake_pipe[0]);
    if (wake_pipe[1] >= 0) close(wake_pipe[1]);
    wake_pipe[0] = wake_pipe[1] = -1;
}

void Audio_init(unsigned int sampleRate, int framesPerBuf)
{
    int err;
    unsigned int rate = sampleRate;
    buffer_frames = framesPerBuf;
    snd_pcm_hw_params_t *hw_params;
    const char *device_name = "default";

    if ((err = snd_pcm_open (&capture_handle, device_name, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) 
    {
        fprintf (stderr, "cannot open audio device %s (%s)\n", 
                    device_name,
                    snd_strerror (err));
        return;
    }

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0 ||
        (err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0 ||
        (err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0 ||
        (err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0 ||
        (err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0 ||
        (err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0 ||
        (err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) 
    {
        fprintf (stderr, "cannot configure audio device (%s)\n", snd_strerror (err));
        snd_pcm_close(capture_handle);
        capture_handle = nullptr;
        return;
    }

    snd_pcm_hw_params_free (hw_params);
    actual_sample_rate = rate;

    if ((err = snd_pcm_prepare (capture_handle)) < 0) 
    {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
        snd_pcm_close(capture_handle);
        capture_handle = nullptr;
        return;
    }

    uint32_t bufSize = buffer_frames * sizeof(int16_t);
    bufs_ = allocateSampleBufs(bufCount_, bufSize);
    assert(bufs_);

    freeQueue_ = new AudioQueue(bufCount_);
    recQueue_ = new AudioQueue(bufCount_);
    assert(freeQueue_ && recQueue_);
    for (uint32_t i = 0; i < bufCount_; i++) 
    {
        freeQueue_->push(&bufs_[i]);
    }

    if (pipe(wake_pipe) != 0)
        wake_pipe[0] = wake_pipe[1] = -1;
}

void Audio_getBufferQueues(AudioQueue **pFreeQ, AudioQueue **pRecQ)
{
    *pFreeQ = freeQueue_;
    *pRecQ = recQueue_; 
}

float Audio_getSampleRate()
{
    return (float)actual_sample_rate;
}

static void * debug_capture_thread_fn( void * v )
{
    while (recording.load())
    {
        sample_buf *buffer;
        if (!freeQueue_->front(&buffer))
        {
            usleep(1000);
            continue;
        }
        freeQueue_->pop();

        struct pollfd pfds[2];
        int n = 0;
        if (wake_pipe[0] >= 0)
        {
            pfds[n].fd = wake_pipe[0];
            pfds[n].events = POLLIN;
            n++;
        }
        int pcm_index = -1;
        struct pollfd pcmfd;
        if (snd_pcm_poll_descriptors(capture_handle, &pcmfd, 1) == 1)
        {
            pcm_index = n;
            pfds[n] = pcmfd;
            n++;
        }
        if (n == 0 || poll(pfds, n, 50) <= 0)
        {
            freeQueue_->push(buffer);
            continue;
        }
        if (wake_pipe[0] >= 0 && (pfds[0].revents & POLLIN))
        {
            char dummy;
            ssize_t ignored = read(wake_pipe[0], &dummy, 1);
            (void)ignored;
            freeQueue_->push(buffer);
            break;
        }
        if (pcm_index < 0 || !(pfds[pcm_index].revents & (POLLIN | POLLERR | POLLHUP)))
        {
            freeQueue_->push(buffer);
            continue;
        }

        int err = snd_pcm_readi(capture_handle, buffer->buf_, buffer_frames);
        if (err == -EAGAIN)
        {
            freeQueue_->push(buffer);
            continue;
        }
        if (err < 0)
        {
            if (recording.load())
                fprintf(stderr, "audio capture recovered from %s\n", snd_strerror(err));
            snd_pcm_recover(capture_handle, err, 1);
            freeQueue_->push(buffer);
            continue;
        }
        if (err != buffer_frames)
        {
            freeQueue_->push(buffer);
            continue;
        }
        buffer->size_ = buffer->cap_;
        recQueue_->push(buffer);
    }

    return NULL;
}

static void stop_capture_thread()
{
    recording.store(false);
    if (wake_pipe[1] >= 0)
    {
        char dummy = 1;
        ssize_t ignored = write(wake_pipe[1], &dummy, 1);
        (void)ignored;
    }
    if (capture_handle)
        snd_pcm_drop(capture_handle);
    if (thread_running)
    {
        pthread_join(capture_thread, nullptr);
        thread_running = false;
    }
}

void Audio_startPlay()
{
    if (capture_handle == nullptr || thread_running)
        return;
    snd_pcm_prepare(capture_handle);
    recording.store(true);
    pthread_create(&capture_thread, NULL, debug_capture_thread_fn, NULL);
    thread_running = true;
}

void Audio_pausePlay()
{
    stop_capture_thread();
}

void Audio_deinit()
{
    stop_capture_thread();

    if (capture_handle)
        snd_pcm_close (capture_handle);
    capture_handle = nullptr;
    close_wake_pipe();

    delete recQueue_;
    delete freeQueue_;
    releaseSampleBufs(bufs_, bufCount_);

    fprintf(stdout, "audio interface closed\n");
}
#endif