
#pragma once

#include "audio_common.h"

void Audio_init(unsigned int sampleRate, int framesPerBuf);
void Audio_deinit();
float Audio_getSampleRate();
void Audio_getBufferQueues(AudioQueue **pFreeQ, AudioQueue **pRecQ);
void Audio_startPlay();




