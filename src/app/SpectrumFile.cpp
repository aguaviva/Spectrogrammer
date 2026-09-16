#include <stdio.h>
#include "SpectrumFile.h"

bool LoadSpectrum(const char *filename, BufferIODouble *pBuffer, float *sample_rate, uint32_t *fft_size)
{
    FILE *f = fopen(filename, "rb");
    if (f != NULL)
    {
        uint32_t version = 0;
        if (fread(&version, sizeof(version), 1, f) != 1)
        {
            fclose(f);
            return false;
        }
        if (version!=1)
        {
            fclose(f);
            return false;
        }

        uint32_t size = 0;
        if (fread(sample_rate, sizeof(*sample_rate), 1, f) != 1 ||
            fread(fft_size, sizeof(*fft_size), 1, f) != 1 ||
            fread(&size, sizeof(size), 1, f) != 1 ||
            size == 0 || size > 1024 * 1024)
        {
            fclose(f);
            return false;
        }

        pBuffer->Resize(size);
        bool ok = fread(pBuffer->GetData(), sizeof(float), size, f) == size;
        fclose(f);
        return ok;
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