//
// Created by Jack on 2019/8/11.
//

#ifndef ASF_MSGQUEUE_H
#define ASF_MSGQUEUE_H


#include <queue>
#include "spinmutex.h"

class MsgQueue
{
public:
	MsgQueue() = default;

	void PushMsg(const std::string &msg)
	{
		std::lock_guard<spin_mutex> lock(m_mutex);
		m_queue.push(msg);
	}

	bool PopMsg(std::string &msg)
	{
		std::lock_guard<spin_mutex> lock(m_mutex);
		if (!m_queue.empty())
		{
			msg = m_queue.front();
			m_queue.pop();
			return true;
		}
		return false;
	}

	void ClearAll()
	{
		std::lock_guard<spin_mutex> lock(m_mutex);
		while (!m_queue.empty())
			m_queue.pop();
	}

private:
	spin_mutex m_mutex;
	std::queue<std::string> m_queue;
};

#endif //ASF_MSGQUEUE_H
