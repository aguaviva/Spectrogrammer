#include "utils.h"
#ifdef ANDROID
#include <android/log.h>
#endif
#include <sys/time.h>
#include <time.h>


void ALog(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef ANDROID    
    __android_log_vprint(ANDROID_LOG_INFO, "Spec", fmt, args);
#endif    
    va_end(args);
}

uint64_t getTickCount() 
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
}