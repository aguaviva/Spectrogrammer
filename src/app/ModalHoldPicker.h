#include <stdint.h>
#include "BufferIO.h"

void RefreshFiles(const char *pWorkingDirectory);
bool HoldPicker(const char *pWorkingDirectory, bool bCanSave, BufferIODouble *pBuffer, float *sample_rate, uint32_t *fft_size);