#include "audio_recorder.h"
#include "audio_common.h"
#include "audio_SLES.h"
#include <cassert>
#include <cstring>

struct CaptureEngine {
  SLmilliHertz sampleRate_;
  uint32_t framesPerBuf_;
  SLObjectItf slEngineObj_;
  SLEngineItf slEngineItf_;
  AudioRecorder *recorder_;
  AudioQueue *freeBufQueue_;
  AudioQueue *recBufQueue_;
  sample_buf *bufs_;
  uint32_t bufCount_;
};
static CaptureEngine engine;

void Audio_init(unsigned int sampleRate, int framesPerBuf)
{
  memset(&engine, 0, sizeof(engine));
  engine.sampleRate_ = static_cast<SLmilliHertz>(sampleRate);
  engine.framesPerBuf_ = static_cast<uint32_t>(framesPerBuf);

  SLresult result = slCreateEngine(&engine.slEngineObj_, 0, NULL, 0, NULL, NULL);
  SLASSERT(result);
  result = (*engine.slEngineObj_)->Realize(engine.slEngineObj_, SL_BOOLEAN_FALSE);
  SLASSERT(result);
  result = (*engine.slEngineObj_)->GetInterface(engine.slEngineObj_, SL_IID_ENGINE,
                                                &engine.slEngineItf_);
  SLASSERT(result);

  uint32_t bufSize = engine.framesPerBuf_ * AUDIO_SAMPLE_CHANNELS * 16;
  bufSize = (bufSize + 7) >> 3;
  engine.bufCount_ = BUF_COUNT;
  engine.bufs_ = allocateSampleBufs(engine.bufCount_, bufSize);
  assert(engine.bufs_);

  engine.freeBufQueue_ = new AudioQueue(engine.bufCount_);
  engine.recBufQueue_ = new AudioQueue(engine.bufCount_);
  for (uint32_t i = 0; i < engine.bufCount_; i++)
    engine.freeBufQueue_->push(&engine.bufs_[i]);

  SampleFormat sampleFormat{};
  sampleFormat.pcmFormat_ = SL_PCMSAMPLEFORMAT_FIXED_16;
  sampleFormat.channels_ = AUDIO_SAMPLE_CHANNELS;
  sampleFormat.sampleRate_ = engine.sampleRate_;
  sampleFormat.framesPerBuf_ = engine.framesPerBuf_;
  engine.recorder_ = new AudioRecorder(&sampleFormat, engine.slEngineItf_);
  engine.recorder_->SetBufQueues(engine.freeBufQueue_, engine.recBufQueue_);
}

void Audio_getBufferQueues(AudioQueue **pFreeQ, AudioQueue **pRecQ)
{
  *pFreeQ = engine.freeBufQueue_;
  *pRecQ = engine.recBufQueue_;
}

float Audio_getSampleRate()
{
  return (float)engine.sampleRate_;
}

void Audio_startPlay()
{
  engine.recorder_->Start();
}

void Audio_pausePlay()
{
  engine.recorder_->Pause();
}

void Audio_deinit()
{
  if (engine.recorder_)
  {
    engine.recorder_->Stop();
    delete engine.recorder_;
    engine.recorder_ = nullptr;
  }
  delete engine.recBufQueue_;
  delete engine.freeBufQueue_;
  engine.recBufQueue_ = nullptr;
  engine.freeBufQueue_ = nullptr;
  releaseSampleBufs(engine.bufs_, engine.bufCount_);
  engine.bufs_ = nullptr;

  if (engine.slEngineObj_ != nullptr)
  {
    (*engine.slEngineObj_)->Destroy(engine.slEngineObj_);
    engine.slEngineObj_ = nullptr;
    engine.slEngineItf_ = nullptr;
  }
}
