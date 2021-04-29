/*
 * Copyright 2015 The Android Open Source Project
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
//#include "jni_interface.h"
#include "audio_recorder.h"
#include "audio_common.h"
#include <jni.h>
#include <SLES/OpenSLES_Android.h>
#include <sys/types.h>
#include <cassert>
#include <cstring>

struct EchoAudioEngine {
  SLmilliHertz fastPathSampleRate_;
  uint32_t fastPathFramesPerBuf_;
  uint16_t sampleChannels_;
  uint16_t bitsPerSample_;

  SLObjectItf slEngineObj_;
  SLEngineItf slEngineItf_;

  AudioRecorder *recorder_;
  AudioQueue *freeBufQueue_;  // Owner of the queue
  AudioQueue *recBufQueue_;   // Owner of the queue

  sample_buf *bufs_;
  uint32_t bufCount_;
  uint32_t frameCount_;
};
static EchoAudioEngine engine;

bool EngineService(void *ctx, uint32_t msg, void *data);
void SetBufQueues(float sampleRate, AudioQueue *freeQ, AudioQueue *recQ);

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Audio_createSLEngine(JNIEnv *env, jclass type, jint sampleRate, jint framesPerBuf) {
  SLresult result;
  memset(&engine, 0, sizeof(engine));

  engine.fastPathSampleRate_ = static_cast<SLmilliHertz>(sampleRate);
  engine.fastPathFramesPerBuf_ = static_cast<uint32_t>(framesPerBuf);
  engine.sampleChannels_ = AUDIO_SAMPLE_CHANNELS;
  engine.bitsPerSample_ = SL_PCMSAMPLEFORMAT_FIXED_16;

  result = slCreateEngine(&engine.slEngineObj_, 0, NULL, 0, NULL, NULL);
  SLASSERT(result);

  result = (*engine.slEngineObj_)->Realize(engine.slEngineObj_, SL_BOOLEAN_FALSE);
  SLASSERT(result);

  result = (*engine.slEngineObj_)->GetInterface(engine.slEngineObj_, SL_IID_ENGINE, &engine.slEngineItf_);
  SLASSERT(result);

  // compute the RECOMMENDED fast audio buffer size:
  //   the lower latency required
  //     *) the smaller the buffer should be (adjust it here) AND
  //     *) the less buffering should be before starting player AFTER
  //        receiving the recorder buffer
  //   Adjust the bufSize here to fit your bill [before it busts]
  uint32_t bufSize = engine.fastPathFramesPerBuf_ * engine.sampleChannels_ * engine.bitsPerSample_;
  bufSize = (bufSize + 7) >> 3;  // bits --> byte
  engine.bufCount_ = BUF_COUNT;
  engine.bufs_ = allocateSampleBufs(engine.bufCount_, bufSize);
  assert(engine.bufs_);

  engine.freeBufQueue_ = new AudioQueue(engine.bufCount_);
  engine.recBufQueue_ = new AudioQueue(engine.bufCount_);
  assert(engine.freeBufQueue_ && engine.recBufQueue_);
  for (uint32_t i = 0; i < engine.bufCount_; i++) {
    engine.freeBufQueue_->push(&engine.bufs_[i]);
  }
}

void GetBufferQueues(float *pSampleRate, AudioQueue **pFreeQ, AudioQueue **pRecQ)
{
  *pSampleRate = engine.fastPathSampleRate_;
  *pFreeQ = engine.freeBufQueue_;
  *pRecQ = engine.recBufQueue_;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_example_plasma_Audio_createAudioRecorder(JNIEnv *env, jclass type) {
  SampleFormat sampleFormat;
  memset(&sampleFormat, 0, sizeof(sampleFormat));
  sampleFormat.pcmFormat_ = static_cast<uint16_t>(engine.bitsPerSample_);

  // SampleFormat.representation_ = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
  sampleFormat.channels_ = engine.sampleChannels_;
  sampleFormat.sampleRate_ = engine.fastPathSampleRate_;
  sampleFormat.framesPerBuf_ = engine.fastPathFramesPerBuf_;
  engine.recorder_ = new AudioRecorder(&sampleFormat, engine.slEngineItf_);
  if (!engine.recorder_) {
    return JNI_FALSE;
  }
  engine.recorder_->SetBufQueues(engine.freeBufQueue_, engine.recBufQueue_);
  engine.recorder_->RegisterCallback(EngineService, (void *)&engine);

  //SetBufQueues(engine.fastPathSampleRate_, engine.freeBufQueue_, engine.recBufQueue_);
  return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Audio_deleteAudioRecorder(JNIEnv *env, jclass type) {
  if (engine.recorder_) delete engine.recorder_;

  engine.recorder_ = nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Audio_startPlay(JNIEnv *env, jclass type) {
  engine.frameCount_ = 0;
  /*
   * start player: make it into waitForData state
   */
  engine.recorder_->Start();
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Audio_stopPlay(JNIEnv *env, jclass type) {
  engine.recorder_->Stop();
  delete engine.recorder_;
  engine.recorder_ = nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Audio_pausePlay(JNIEnv *env, jclass type) {
  engine.recorder_->Pause();
}

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Audio_deleteSLEngine( JNIEnv *env, jclass type) {
  delete engine.recBufQueue_;
  delete engine.freeBufQueue_;
  releaseSampleBufs(engine.bufs_, engine.bufCount_);

  if (engine.slEngineObj_ != nullptr) {
    (*engine.slEngineObj_)->Destroy(engine.slEngineObj_);
    engine.slEngineObj_ = nullptr;
    engine.slEngineItf_ = nullptr;
  }
}

uint32_t dbgEngineGetBufCount() {
  uint32_t count = 0;//engine.player_->dbgGetDevBufCount();
  count += engine.recorder_->dbgGetDevBufCount();
  count += engine.freeBufQueue_->size();
  count += engine.recBufQueue_->size();

  LOGE(
      "Buf Disrtibutions: PlayerDev=%d, RecDev=%d, FreeQ=%d, "
      "RecQ=%d",
      0,//engine.player_->dbgGetDevBufCount(),
      engine.recorder_->dbgGetDevBufCount(), engine.freeBufQueue_->size(),
      engine.recBufQueue_->size());
  if (count != engine.bufCount_) {
    LOGE("====Lost Bufs among the queue(supposed = %d, found = %d)", BUF_COUNT, count);
  }
  return count;
}

/*
 * simple message passing for player/recorder to communicate with engine
 */
bool EngineService(void *ctx, uint32_t msg, void *data) {
  assert(ctx == &engine);
  switch (msg) {
    case ENGINE_SERVICE_MSG_RETRIEVE_DUMP_BUFS: {
      *(static_cast<uint32_t *>(data)) = dbgEngineGetBufCount();
      break;
    }
    case ENGINE_SERVICE_MSG_RECORDED_AUDIO_AVAILABLE: {
      // adding audio delay effect
      sample_buf *buf = static_cast<sample_buf *>(data);
      assert(engine.fastPathFramesPerBuf_ == buf->size_ / engine.sampleChannels_ / (engine.bitsPerSample_ / 8));

      break;
    }
    default:
      assert(false);
      return false;
  }

  return true;
}


