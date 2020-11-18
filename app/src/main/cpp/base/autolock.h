#ifndef _AUTOLOCK_H
#define _AUTOLOCK_H

#ifdef WIN32
	#include <windows.h>
	#define mutex_object		CRITICAL_SECTION
	#define mutex_init(obj)		InitializeCriticalSection(obj)
	#define mutex_lock(obj)		EnterCriticalSection(obj)
	#define mutex_unlock(obj)	LeaveCriticalSection(obj)
	#define mutex_destroy(obj)	DeleteCriticalSection(obj)
	#define atomic_inc(v)		InterlockedIncrement(v)
	#define atomic_dec(v)		InterlockedDecrement(v)
#else
	#include <pthread.h>
	#define mutex_object		pthread_mutex_t
	#define mutex_init(obj)		pthread_mutex_init(obj, NULL)
	#define mutex_lock(obj)		pthread_mutex_lock(obj)
	#define mutex_unlock(obj)	pthread_mutex_unlock(obj)
	#define mutex_destroy(obj)	pthread_mutex_destroy(obj)
	#define atomic_inc(v)		__sync_add_and_fetch(v, 1)
	#define atomic_dec(v)		__sync_sub_and_fetch(v, 1)
#endif

class CAutoLock
{
private:
	typedef struct _LOCK_DATA {
		long RefCount;
		mutex_object Mutex;
	} LOCK_DATA, *PLOCK_DATA;

public:
	CAutoLock GetLock()
	{
		return *this;
	}
	void ReleaseLock()
	{
		if (m_bUnlocked == false)
		{
			mutex_unlock(&m_pLockData->Mutex);
			m_bUnlocked = true;
		}
	}
	CAutoLock()
	{
		m_pLockData = new LOCK_DATA;
		m_pLockData->RefCount = 1;
		mutex_init(&m_pLockData->Mutex);
		m_bUnlocked = true;
	}
	CAutoLock(const CAutoLock &C)
	{
		m_pLockData = C.m_pLockData;
		atomic_inc(&m_pLockData->RefCount);
		mutex_lock(&m_pLockData->Mutex);
		m_bUnlocked = false;
	}
	~CAutoLock()
	{
		if (m_bUnlocked == false)
			mutex_unlock(&m_pLockData->Mutex);
		if (atomic_dec(&m_pLockData->RefCount) == 0)
		{
			mutex_destroy(&m_pLockData->Mutex);
			delete m_pLockData;
		}
	}

private:
	PLOCK_DATA m_pLockData;
	bool m_bUnlocked;
};

#endif