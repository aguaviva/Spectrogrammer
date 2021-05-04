#include <pthread.h>
#include <semaphore.h>
#include "audio_common.h"
#include <jni.h>
#include <ctime>
#include <algorithm>
#include <android/log.h>
#include <android/bitmap.h>

pthread_attr_t attr;
pthread_t worker;
sem_t headwriteprotect;
sem_t semaphoreUI;
static bool s_run=false;

extern "C" JNIEXPORT void JNICALL Java_com_example_plasma_Spectrogram_ConnectWithAudioMT(JNIEnv * env, jclass obj, jobject bitmap)
{
    //GetBufferQueues(&sampleRate, &freeQueue, &recQueue);
/*
    SetRecorderCallback([](void* pCTX, uint32_t msg, void* pData) ->bool {
        //LOGI("data");
        sem_post(&headwriteprotect);
        return true;
    });


    sem_init(&headwriteprotect, 0, 0);
    pthread_attr_init(&attr);
    pthread_create(&worker, &attr, [env, bitmap](void *p)->void* {
        s_run = true;

        while(s_run)
        {
            sem_wait(&headwriteprotect);

            //Java_com_example_plasma_Spectrogram_Update(env,nullptr, bitmap,  0);
        }

    }, nullptr);
*/
}
