#ifndef NATIVE_AUDIO_AUDIO_RECORDER_H
#define NATIVE_AUDIO_AUDIO_RECORDER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "audio_SLES.h"
#include "audio_common.h"

class AudioRecorder {
  SLObjectItf recObjectItf_ = nullptr;
  SLRecordItf recItf_ = nullptr;
  SLAndroidSimpleBufferQueueItf recBufQueueItf_ = nullptr;

  SampleFormat sampleInfo_{};
  AudioQueue *freeQueue_ = nullptr;
  AudioQueue *recQueue_ = nullptr;
  AudioQueue *devShadowQueue_ = nullptr;

 public:
  AudioRecorder(SampleFormat *, SLEngineItf engineEngine);
  ~AudioRecorder();
  SLboolean Start(void);
  SLboolean Stop(void);
  SLboolean Pause(void);
  void SetBufQueues(AudioQueue *freeQ, AudioQueue *recQ);
  void ProcessSLCallback(SLAndroidSimpleBufferQueueItf bq);
};

#endif
