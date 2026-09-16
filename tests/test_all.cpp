#include <cassert>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <vector>

#include "audio_main.h"
#include "auformat.h"
#include "BufferAverage.h"
#include "BufferIO.h"
#include "buf_manager.h"
#include "ChunkerProcessor.h"
#include "fft.h"
#include "Processor.h"
#include "scale.h"
#include "ScaleBufferY.h"
#include "SpectrumFile.h"

static int g_failed = 0;
static int g_passed = 0;

#define RUN(fn)                                                                \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, "RUN %s\n", #fn);                                      \
        fflush(stderr);                                                        \
        fn();                                                                  \
    } while (0)

#define CHECK(cond)                                                            \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            g_failed++;                                                        \
        }                                                                      \
        else                                                                   \
            g_passed++;                                                        \
    } while (0)

static void test_audio_queue()
{
    AudioQueue q(4);
    sample_buf a{}, b{}, c{};
    sample_buf *out = nullptr;

    CHECK(q.size() == 0);
    CHECK(!q.front(&out));
    CHECK(q.push(&a));
    CHECK(q.push(&b));
    CHECK(q.size() == 2);
    CHECK(q.front(&out) && out == &a);
    CHECK(q.peek(&out, 1) && out == &b);
    q.pop();
    CHECK(q.front(&out) && out == &b);
    CHECK(q.push(&c));
    CHECK(q.size() == 2);
}

static void test_sample_bufs()
{
    uint32_t count = 4;
    sample_buf *bufs = allocateSampleBufs(count, 64);
    CHECK(bufs != nullptr);
    CHECK(bufs[0].cap_ == 64);
    CHECK(bufs[0].buf_ != nullptr);
    releaseSampleBufs(bufs, count);
}

static void test_spectrum_file()
{
    BufferIODouble src(8);
    for (int i = 0; i < 8; i++)
        src.GetData()[i] = (float)i * 0.5f;

    const char *path = "/tmp/spectrogrammer_test.spec";
    CHECK(SaveSpectrum(path, &src, 48000.0f, 1024));

    BufferIODouble dst;
    float rate = 0;
    uint32_t fft = 0;
    CHECK(LoadSpectrum(path, &dst, &rate, &fft));
    CHECK(rate == 48000.0f);
    CHECK(fft == 1024);
    CHECK(dst.GetSize() == 8);
    CHECK(dst.GetData()[7] == 3.5f);

    CHECK(!LoadSpectrum("/tmp/spectrogrammer_missing.spec", &dst, &rate, &fft));

    FILE *f = fopen(path, "wb");
    uint32_t bad = 99;
    fwrite(&bad, sizeof(bad), 1, f);
    fclose(f);
    CHECK(!LoadSpectrum(path, &dst, &rate, &fft));
    remove(path);
}

class FakeProcessor : public Processor
{
public:
    int n = 256;
    std::vector<float> samples;
    BufferIODouble bins;
    int compute_count = 0;

    void init(int length, float) override
    {
        n = length;
        samples.assign(n, 0);
        bins.Resize(n / 2 + 1);
    }
    const char *GetName() const override { return "fake"; }
    int getProcessedLength() const override { return n; }
    int getBinCount() const override { return bins.GetSize(); }
    void convertShortToFFT(const AU_FORMAT *input, int offsetDest, int length) override
    {
        for (int i = 0; i < length; i++)
            samples[offsetDest + i] = (float)input[i];
    }
    void computePower(float) override { compute_count++; }
    float bin2Freq(int bin) const override { return (float)bin; }
    float freq2Bin(float freq) const override { return freq; }
    BufferIODouble *getBufferIO() override { return &bins; }
};

static sample_buf *filled_buf(int16_t value, uint32_t frames)
{
    static int next = 0;
    static sample_buf store[8];
    static std::vector<int16_t> data[8];
    int i = next++;
    data[i].assign(frames, value);
    store[i].buf_ = (uint8_t *)data[i].data();
    store[i].cap_ = frames * sizeof(int16_t);
    store[i].size_ = store[i].cap_;
    return &store[i];
}

static void test_chunker()
{
    AudioQueue rec(8);
    AudioQueue freeq(8);
    FakeProcessor proc;
    proc.init(256, 48000);

    rec.push(filled_buf(1, 128));
    rec.push(filled_buf(2, 128));

    ChunkerProcessor chunker;
    chunker.SetQueues(&rec, &freeq);
    chunker.begin();
    CHECK(chunker.Process(&proc, 0, 0.0));
    CHECK(proc.compute_count == 1);
    CHECK(proc.samples[0] == 1);
    CHECK(proc.samples[127] == 1);
    CHECK(proc.samples[128] == 2);
    CHECK(proc.samples[255] == 2);
    CHECK(!chunker.Process(&proc, 0, 0.0));
    chunker.end();
}

static void test_fft_sine()
{
    const int nfft = 1024;
    const float sr = 48000.0f;
    const float freq = 468.75f; // exactly bin 10: 10 * sr / nfft
    myFFT fft;
    fft.init(nfft, sr);

    std::vector<int16_t> samples(nfft);
    for (int i = 0; i < nfft; i++)
        samples[i] = (int16_t)(16000.0f * sinf(2.0f * (float)M_PI * freq * i / sr));

    fft.convertShortToFFT(samples.data(), 0, nfft);
    fft.computePower(0.0f);

    float *bins = fft.getBufferIO()->GetData();
    int peak = 1;
    for (int i = 2; i < fft.getBinCount(); i++)
    {
        if (bins[i] > bins[peak])
            peak = i;
    }
    CHECK(peak == 10);
    CHECK(fabsf(fft.bin2Freq(10) - freq) < 0.01f);
}

static void test_buffer_average()
{
    BufferAverage avg;
    avg.setAverageCount(2);
    BufferIODouble a(2), b(2);
    a.GetData()[0] = 2;
    a.GetData()[1] = 4;
    b.GetData()[0] = 6;
    b.GetData()[1] = 8;

    CHECK(avg.Do(&a) == nullptr);
    BufferIODouble *out = avg.Do(&b);
    CHECK(out != nullptr);
    CHECK(out->GetData()[0] == 4);
    CHECK(out->GetData()[1] == 6);
}

static void test_scale_y()
{
    CHECK(linearToDecibels(1, 1) == 0);
    CHECK(fabsf(linearToDecibels(0.001f, 1) - (-60.0f)) < 0.01f);

    BufferIODouble in(1), out;
    in.GetData()[0] = 32768;
    applyScaleY(&in, -130, 0, true, &out);
    CHECK(out.GetSize() == 1);
    CHECK(out.GetData()[0] >= 0.99f);

    CHECK(clamp(5, 0, 1) == 1);
    CHECK(clamp(-1, 0, 1) == 0);
    CHECK(fabsf(lerp(0.5f, 0, 10) - 5) < 1e-5f);
}

static void test_alsa_pause_does_not_hang()
{
    Audio_init(24000, 1024);
    Audio_startPlay();
    usleep(200000);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    Audio_pausePlay();
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    CHECK(ms < 500.0);

    Audio_deinit();
}

int main()
{
    RUN(test_audio_queue);
    RUN(test_sample_bufs);
    RUN(test_spectrum_file);
    RUN(test_chunker);
    RUN(test_fft_sine);
    RUN(test_buffer_average);
    RUN(test_scale_y);
    RUN(test_alsa_pause_does_not_hang);

    printf("%d passed, %d failed\n", g_passed, g_failed);
    return g_failed ? 1 : 0;
}
