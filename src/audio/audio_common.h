#ifndef NATIVE_AUDIO_AUDIO_COMMON_H
#define NATIVE_AUDIO_AUDIO_COMMON_H

#include "buf_manager.h"

#define AUDIO_SAMPLE_CHANNELS 1
#define RECORD_DEVICE_KICKSTART_BUF_COUNT 2
#define DEVICE_SHADOW_BUFFER_QUEUE_LEN 4
#define BUF_COUNT 64

struct SampleFormat {
  uint32_t sampleRate_;
  uint32_t framesPerBuf_;
  uint16_t channels_;
  uint16_t pcmFormat_;
};

#endif
