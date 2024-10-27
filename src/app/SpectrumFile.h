#ifndef SPECTRUMFILE_H
#define SPECTRUMFILE_H

#include <stdint.h>
#include "BufferIO.h"

bool LoadSpectrum(const char *filename, BufferIODouble *pBuffer, float *sample_rate, uint32_t *fft_size);
bool SaveSpectrum(const char *filename, BufferIODouble *pBuffer, float sample_rate, uint32_t fft_size);

#endif