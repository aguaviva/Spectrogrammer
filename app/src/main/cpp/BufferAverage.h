//
// Created by raul on 12/29/2021.
//

#ifndef SPECTROGRAMMER_BUFFERAVERAGE_H
#define SPECTROGRAMMER_BUFFERAVERAGE_H

class BufferAverage
{
    BufferIODouble bufferIO;
    int m_averageCount;
    int m_i;

public:
    BufferAverage():bufferIO(0)
    {
        m_averageCount = 0;
        m_i = 0;
    }

    void setAverageCount(int av)
    {
        m_i = 0;
        m_averageCount = av;
    }

    int getAverageCount()
    {
        return m_averageCount;
    }

    void reset()
    {
        bufferIO.clear();
        m_i = 0;
    }

    int getProgress()
    {
        return (m_i*100)/m_averageCount;
    }

    BufferIODouble *Do(BufferIODouble *pIn)
    {
        if (pIn->GetSize()!=bufferIO.GetSize())
        {
            bufferIO.Resize(pIn->GetSize());
            reset();
        }

        if (m_averageCount<=1)
            return pIn;

        if (m_i==0)
        {
            bufferIO.copy(pIn);
        }
        else
        {
            bufferIO.add(pIn);
        }

        m_i++;
        if (m_i==m_averageCount)
        {
            m_i=0;

            float invCount = (float)1.0f/(float)m_averageCount;

            bufferIO.mul(invCount);

            return &bufferIO;
        }

        return nullptr;
    }

};

#endif //SPECTROGRAMMER_BUFFERAVERAGE_H
