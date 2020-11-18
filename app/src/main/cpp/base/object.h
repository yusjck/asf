#ifndef _OBJECT_H
#define _OBJECT_H

#ifdef WIN32
#include <windows.h>
	#define atomic_inc(v)		InterlockedIncrement(v)
	#define atomic_dec(v)		InterlockedDecrement(v)
#else
#define atomic_inc(v)		__sync_add_and_fetch(v, 1)
#define atomic_dec(v)		__sync_sub_and_fetch(v, 1)
#endif

#include <stdint.h>

class CRefObject
{
protected:
    CRefObject()
    {
        m_RefCount = 1;
    }

    virtual ~CRefObject()
    {
    }

public:
    uint32_t AddRef()
    {
        return atomic_inc(&m_RefCount);
    }

    uint32_t Release()
    {
        uint32_t RefCount = atomic_dec(&m_RefCount);
        if (RefCount == 0)
            delete this;
        return RefCount;
    }

private:
    uint32_t m_RefCount;
};

template <class T>
class CAutoRef
{
public:
    CAutoRef(T *RefObject)
    {
        m_RefObject = RefObject;
    }

    ~CAutoRef()
    {
        if (m_RefObject)
            m_RefObject->Release();
    }

    CAutoRef(const CAutoRef<T> &C)
    {
        m_RefObject = C.m_RefObject;
        if (m_RefObject)
            m_RefObject->AddRef();
    }

    CAutoRef<T> &operator=(const CAutoRef<T> &C)
    {
        if (this != &C)
        {
            if (m_RefObject)
                m_RefObject->Release();
            m_RefObject = C.m_RefObject;
            if (m_RefObject)
                m_RefObject->AddRef();
        }
        return *this;
    }

    CAutoRef<T> &operator=(T *RefObject)
    {
        if (m_RefObject != RefObject)
        {
            if (m_RefObject)
                m_RefObject->Release();
            m_RefObject = RefObject;
        }
        return *this;
    }

    T *Get()
    {
        return m_RefObject;
    }

    operator T *() const
    {
        return m_RefObject;
    }

    T *operator->() const
    {
        return m_RefObject;
    }

private:
    T *m_RefObject;
};

#endif