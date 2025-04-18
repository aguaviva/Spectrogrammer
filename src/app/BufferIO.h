#ifndef BUFFERIO_H
#define BUFFERIO_H
#include <cstring>
#include <stdlib.h>
#include <assert.h>

template <class type>
class BufferIO
{
    type *m_rout = nullptr;
    int   m_size = 0;
public:
    BufferIO()
    {
        
    }
    BufferIO(int size)
    {
        Resize(size);
    }

    BufferIO(BufferIO<type> *pIn)
    {
        copy(pIn);
    }

    ~BufferIO()
    {
        free(m_rout);
    }

    void Resize(int size)
    {
        if (m_size != size)
        {
            free(m_rout);
            m_size = size;
            m_rout = (type *) malloc(sizeof(type) * m_size);
        }
    }

    void clear()
    {
        for(int i=0;i<m_size;i++)
            m_rout[i]=0;
    }

    void copy(BufferIO<type> *pIn)
    {
        Resize(pIn->GetSize());
        memcpy(m_rout, pIn->GetData(), m_size*sizeof(type));
    }

    void add(BufferIO<type> *pIn)
    {
        assert(pIn->GetSize()==GetSize());

        float *pDataIn = pIn->GetData();
        for(int i=0;i<pIn->GetSize();i++)
        {
            m_rout[i] += pDataIn[i];
        }
    }

    void add(float v)
    {
        for(int i=0;i<GetSize();i++)
        {
            m_rout[i] += v;
        }
    }

    void sub(BufferIO<type> *pIn)
    {
        assert(pIn->GetSize()==GetSize());

        float *pDataIn = pIn->GetData();
        for(int i=0;i<pIn->GetSize();i++)
        {
            m_rout[i] -= pDataIn[i];
        }
    }


    void mul(float k)
    {
        for(int i=0;i<GetSize();i++)
        {
            m_rout[i] *= k;
        }
    }

    type *GetData() const { return m_rout; }
    int GetSize() const { return m_size; }
};

typedef BufferIO<float> BufferIODouble;
typedef BufferIO<int> BufferIOInt;

#endif