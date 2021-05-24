#ifndef CUMULATIVEMOVING_H
#define CUMULATIVEMOVING_H
#include "BufferIO.h"

template <class type>
struct tuple
{
    type m_average;
    type m_variance;
};

template <class type>
class CumulativeMoving : public BufferIO<tuple<type>>
{
    int     m_n = 0;
public:
    CumulativeMoving(int size) : BufferIO<tuple<type>>(size)
    {
    }

    void Resize(int size)
    {
        BufferIO::Resize(size);
        clear();
    }

    void clear()
    {
        for(int i=0;i<m_size;i++) {
            m_rout[i].m_average = 0.0;
            m_rout[i].m_variance = 0.0;
        }
    }

    void Average(BufferIO<type> *pInputBuf)
    {
        assert(pInput->getSize()==getSize());

        m_n++;
        if (m_n == 1)
        {
            for(int i=0;i<m_size;i++) {
                m_rout[i].m_average = x;
                m_rout[i].m_variance = 0.0;
            }
        }
        else
        {
        type *pInput = pInputBuf->GetData();

            type invNextN = 1/ m_n;
            for(int i=0;i<m_size;i++)
            {
                type average = m_rout[i].m_average[i] + (pInput[i] - m_rout[i].m_average) * invNextN;
                type variance = m_rout[i].m_variance[i] + (pInput[i] - m_rout[i].m_average) * (pInput[i] - average);

                m_rout[i].m_average = average;
                m_rout[i].m_variance = variance;
            }
        }
    }
};

typedef CumulativeMoving<float> CumulativeMovingFloat;
typedef CumulativeMoving<int> CumulativeMovingInt;

#endif
