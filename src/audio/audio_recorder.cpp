#include "audio_recorder.h"
#include <cstring>

static void ConvertToSLSampleFormat(SLAndroidDataFormat_PCM_EX *pFormat,
                                    SampleFormat *info)
{
  memset(pFormat, 0, sizeof(*pFormat));
  pFormat->formatType = SL_DATAFORMAT_PCM;
  pFormat->numChannels = 1;
  pFormat->channelMask = SL_SPEAKER_FRONT_LEFT;
  pFormat->sampleRate = info->sampleRate_ * 1000;
  pFormat->endianness = SL_BYTEORDER_LITTLEENDIAN;
  pFormat->bitsPerSample = info->pcmFormat_;
  pFormat->containerSize = info->pcmFormat_;
}

static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *rec)
{
  (static_cast<AudioRecorder *>(rec))->ProcessSLCallback(bq);
}

void AudioRecorder::ProcessSLCallback(SLAndroidSimpleBufferQueueItf bq)
{
  assert(bq == recBufQueueItf_);
  sample_buf *dataBuf = nullptr;
  bool res = devShadowQueue_->front(&dataBuf);
  assert(res);
  devShadowQueue_->pop();
  dataBuf->size_ = dataBuf->cap_;
  recQueue_->push(dataBuf);

  sample_buf *freeBuf;
  while (freeQueue_->front(&freeBuf) && devShadowQueue_->push(freeBuf))
  {
    freeQueue_->pop();
    SLresult result = (*bq)->Enqueue(bq, freeBuf->buf_, freeBuf->cap_);
    SLASSERT(result);
  }

  if (devShadowQueue_->size() == 0)
    (*recItf_)->SetRecordState(recItf_, SL_RECORDSTATE_STOPPED);
}

AudioRecorder::AudioRecorder(SampleFormat *sampleFormat, SLEngineItf slEngine)
{
  SLresult result;
  sampleInfo_ = *sampleFormat;
  SLAndroidDataFormat_PCM_EX format_pcm;
  ConvertToSLSampleFormat(&format_pcm, &sampleInfo_);

  SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE,
                                    SL_IODEVICE_AUDIOINPUT,
                                    SL_DEFAULTDEVICEID_AUDIOINPUT, nullptr};
  SLDataSource audioSrc = {&loc_dev, nullptr};
  SLDataLocator_AndroidSimpleBufferQueue loc_bq = {
      SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, DEVICE_SHADOW_BUFFER_QUEUE_LEN};
  SLDataSink audioSnk = {&loc_bq, &format_pcm};

  const SLInterfaceID id[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                               SL_IID_ANDROIDCONFIGURATION};
  const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
  result = (*slEngine)->CreateAudioRecorder(slEngine, &recObjectItf_, &audioSrc,
                                            &audioSnk, 2, id, req);
  SLASSERT(result);

  SLAndroidConfigurationItf inputConfig;
  result = (*recObjectItf_)->GetInterface(recObjectItf_, SL_IID_ANDROIDCONFIGURATION, &inputConfig);
  if (result == SL_RESULT_SUCCESS)
  {
    SLuint32 presetValue = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
    (*inputConfig)->SetConfiguration(inputConfig, SL_ANDROID_KEY_RECORDING_PRESET,
                                     &presetValue, sizeof(SLuint32));
  }

  result = (*recObjectItf_)->Realize(recObjectItf_, SL_BOOLEAN_FALSE);
  SLASSERT(result);
  result = (*recObjectItf_)->GetInterface(recObjectItf_, SL_IID_RECORD, &recItf_);
  SLASSERT(result);
  result = (*recObjectItf_)->GetInterface(recObjectItf_, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                          &recBufQueueItf_);
  SLASSERT(result);
  result = (*recBufQueueItf_)->RegisterCallback(recBufQueueItf_, bqRecorderCallback, this);
  SLASSERT(result);

  devShadowQueue_ = new AudioQueue(DEVICE_SHADOW_BUFFER_QUEUE_LEN);
  assert(devShadowQueue_);
}

SLboolean AudioRecorder::Start()
{
  if (!freeQueue_ || !recQueue_ || !devShadowQueue_)
    return SL_BOOLEAN_FALSE;

  SLresult result = (*recItf_)->SetRecordState(recItf_, SL_RECORDSTATE_STOPPED);
  SLASSERT(result);
  result = (*recBufQueueItf_)->Clear(recBufQueueItf_);
  SLASSERT(result);

  for (int i = 0; i < RECORD_DEVICE_KICKSTART_BUF_COUNT; i++)
  {
    sample_buf *buf = nullptr;
    if (!freeQueue_->front(&buf))
      break;
    freeQueue_->pop();
    result = (*recBufQueueItf_)->Enqueue(recBufQueueItf_, buf->buf_, buf->cap_);
    SLASSERT(result);
    devShadowQueue_->push(buf);
  }

  result = (*recItf_)->SetRecordState(recItf_, SL_RECORDSTATE_RECORDING);
  SLASSERT(result);
  return SL_BOOLEAN_TRUE;
}

SLboolean AudioRecorder::Pause()
{
  SLresult result = (*recItf_)->SetRecordState(recItf_, SL_RECORDSTATE_PAUSED);
  SLASSERT(result);
  return SL_BOOLEAN_TRUE;
}

SLboolean AudioRecorder::Stop()
{
  SLresult result = (*recItf_)->SetRecordState(recItf_, SL_RECORDSTATE_STOPPED);
  SLASSERT(result);
  result = (*recBufQueueItf_)->Clear(recBufQueueItf_);
  SLASSERT(result);
  return SL_BOOLEAN_TRUE;
}

AudioRecorder::~AudioRecorder()
{
  if (recObjectItf_ != nullptr)
    (*recObjectItf_)->Destroy(recObjectItf_);

  if (devShadowQueue_)
  {
    sample_buf *buf = nullptr;
    while (devShadowQueue_->front(&buf))
    {
      devShadowQueue_->pop();
      if (freeQueue_)
        freeQueue_->push(buf);
    }
    delete devShadowQueue_;
  }
}

void AudioRecorder::SetBufQueues(AudioQueue *freeQ, AudioQueue *recQ)
{
  assert(freeQ && recQ);
  freeQueue_ = freeQ;
  recQueue_ = recQ;
}
