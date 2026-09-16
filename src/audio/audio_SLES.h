#ifndef NATIVE_AUDIO_SLES_H
#define NATIVE_AUDIO_SLES_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <cassert>

#define SLASSERT(x)                   \
  do {                                \
    assert(SL_RESULT_SUCCESS == (x)); \
    (void)(x);                        \
  } while (0)

#endif
