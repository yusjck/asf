//
// Created by Jack on 2019/7/21.
//

#ifndef ASF_EVENT_H
#define ASF_EVENT_H

#include <thread>

class ThreadEvent
{
public:
	void Wait()
	{
		std::unique_lock<std::mutex> lock(m_cvMutex);
		m_cv.wait(lock, [this]{ return m_flag != 0; });
	}

	bool Wait(uint32_t timeout)
	{
		std::unique_lock<std::mutex> lock(m_cvMutex);
		return m_cv.wait_for(lock, std::chrono::milliseconds(timeout)) != std::cv_status::timeout;
	}

	void Set()
	{
		std::unique_lock<std::mutex> lock(m_cvMutex);
		m_flag = 1;
		m_cv.notify_all();
	}

	void Reset()
	{
		std::unique_lock<std::mutex> lock(m_cvMutex);
		m_flag = 0;
	}

	ThreadEvent()
	{
		m_flag = 0;
	}

	~ThreadEvent()
	{

	}

private:
	std::mutex m_cvMutex;
	std::condition_variable m_cv;
	int m_flag;
};

#endif //ASF_EVENT_H
