#include "fft.h"
#include <cmath>

float hanning(int i, int window_size)
{
    return .50f - .50f * cos((2.0f * M_PI * (float)i) / (float)(window_size - 1));
}
