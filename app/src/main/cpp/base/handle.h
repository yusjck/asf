#ifndef _HANDLE_H
#define _HANDLE_H

#include <map>

typedef const void * handle_t;

template <class T>
class HandleStore
{
public:
	handle_t Insert(T *obj)
	{
		if (obj == NULL)
			return NULL;

		handle_t handle = GenerateHandle();
		while (m_handles.find(handle) != m_handles.end())
			handle = GenerateHandle();

		_handle h = {handle, obj};
		obj->AddRef();
		m_handles[handle] = h;
		return handle;
	}

	T *Query(handle_t handle)
	{
		_handle_iter iter = m_handles.find(handle);
		if (iter != m_handles.end())
		{
			_handle &h = iter->second;
			h.obj->AddRef();
			return h.obj;
		}
		return NULL;
	}

	bool Delete(handle_t handle)
	{
		_handle_iter iter = m_handles.find(handle);
		if (iter != m_handles.end())
		{
			_handle &h = iter->second;
			h.obj->Release();
			m_handles.erase(iter);
			return true;
		}
		return false;
	}

	void DeleteAll()
	{
		_handle_iter iter = m_handles.begin();
		while (iter != m_handles.end())
		{
			_handle &h = iter->second;
			h.obj->Release();
			iter = m_handles.erase(iter);
		}
	}

	handle_t FindNext(handle_t handle)
	{
		_handle_iter iter = m_handles.begin();
		while (iter != m_handles.end())
		{
			if (handle == NULL || iter->first == handle)
			{
				iter++;
				if (iter != m_handles.end())
					return iter->first;
				break;
			}
			iter++;
		}
		return NULL;
	}

	HandleStore()
	{
		m_handleIndex = 1;
	}

	~HandleStore()
	{
		DeleteAll();
	}

private:
	typedef struct {
		handle_t handle;
		T *obj;
	} _handle;
	typedef typename std::map<handle_t, _handle>::iterator _handle_iter;
	uint32_t m_handleIndex;
	std::map<handle_t, _handle> m_handles;

	handle_t GenerateHandle()
	{
		return (handle_t)(uintptr_t)m_handleIndex++;
	}
};

#endif