#include <stdio.h>
#include "SpectrumFile.h"

bool LoadSpectrum(const char *filename, BufferIODouble *pBuffer, float *sample_rate, uint32_t *fft_size)
{
    FILE *f = fopen(filename, "rb");
    if (f != NULL)
    {
        uint32_t version = 0;
        fread(&version, 1, sizeof(uint32_t), f);
        if (version!=1)
            return false;

        fread(sample_rate, 1, sizeof(float), f);
        fread(fft_size, 1, sizeof(uint32_t), f);

        uint32_t size = 0;
        fread(&size, 1, sizeof(uint32_t), f);

        pBuffer->Resize(size);
        fread(pBuffer->GetData(), size, sizeof(float), f);

        fclose(f);
        return true;
    }

    return false;
}


bool SaveSpectrum(const char *filename, BufferIODouble *pBuffer, float sample_rate, uint32_t fft_size)
{
    FILE *f = fopen(filename, "wb");
    if (f!=NULL)
    {
        uint32_t version = 1;
        fwrite(&version, 1, sizeof(uint32_t), f);

        fwrite(&sample_rate, 1, sizeof(float), f);
        fwrite(&fft_size, 1, sizeof(uint32_t), f);
        
        uint32_t size = pBuffer->GetSize();
        fwrite(&size, 1, sizeof(uint32_t), f);
        
        fwrite(pBuffer->GetData(), size, sizeof(float), f);

        fclose(f);
        return true;
    }
    return false;
}