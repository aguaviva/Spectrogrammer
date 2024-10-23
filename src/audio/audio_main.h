
#include "audio_common.h"

void Audio_createSLEngine(int sampleRate, int framesPerBuf);
void GetBufferQueues(float *pSampleRate, AudioQueue **pFreeQ, AudioQueue **pRecQ);
void SetRecorderCallback(ENGINE_CALLBACK callback);
bool Audio_createAudioRecorder();
void Audio_deleteAudioRecorder();
void Audio_deleteSLEngine();
void Audio_startPlay();




