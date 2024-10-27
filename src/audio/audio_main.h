
#include "audio_common.h"

void Audio_createSLEngine(int sampleRate, int framesPerBuf);
float Audio_getSampleRate();
void Audio_getBufferQueues(AudioQueue **pFreeQ, AudioQueue **pRecQ);
void Audio_getRecorderCallback(ENGINE_CALLBACK callback);
bool Audio_createAudioRecorder();
void Audio_deleteAudioRecorder();
void Audio_deleteSLEngine();
void Audio_startPlay();




