#ifndef WIN32

#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>

#endif

#include "common.h"
#include <signal.h>
#include "ipcserver.h"

IpcServer::IpcServer()
{
#ifndef _WIN32
	evthread_use_pthreads();
#else
	evthread_use_windows_threads();
#endif

	m_evbase = event_base_new();
	if (!m_evbase)
		throw "Could not initialize libevent\n";

	m_listener = NULL;
	m_dispatchThread = 0;
	m_running = false;
	m_connectionFactory = NULL;
}

IpcServer::~IpcServer()
{
	Stop();

	if (m_evbase)
	{
		event_base_free(m_evbase);
		m_evbase = NULL;
	}
}

void IpcServer::OnAccept(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen)
{
	struct bufferevent *bev = bufferevent_socket_new(m_evbase, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	if (!bev)
	{
		LOGI("IpcServer: Error constructing bufferevent");
		event_base_loopbreak(m_evbase);
		return;
	}

	LOGI("IpcServer: New client connection\n");
	IpcConnection *conn = m_connectionFactory->NewConnection();
	if (!conn)
	{
		LOGI("IpcServer: NewConnection() failed in ConnectionFactory\n");
		bufferevent_free(bev);
		return;
	}

	m_connList.push_back(conn);
	IpcConnectionManager::OpenConnection(conn, bev);
}

void IpcServer::CloseConnection(IpcConnection *conn)
{
	std::vector<IpcConnection *>::iterator iter = m_connList.begin();
	while (iter != m_connList.end())
	{
		if (conn == *iter)
		{
			m_connList.erase(iter);
			break;
		}
		iter++;
	}
	m_connectionFactory->DeleteConnection(conn);
}

void IpcServer::accept_cb(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen, void *arg)
{
	((IpcServer *) arg)->OnAccept(listener, fd, sock, socklen);
}

void IpcServer::signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	IpcServer *pThis = (IpcServer *) user_data;
	struct timeval delay = {2, 0};
	LOGI("IpcServer: Caught an interrupt signal; exiting cleanly in two seconds\n");
	event_base_loopexit(pThis->m_evbase, &delay);
}

THREAD_FN IpcServer::DispatchThread(void *arg)
{
	IpcServer *pThis = (IpcServer *) arg;
	event_base_dispatch(pThis->m_evbase);

	// 关闭监听端口
	if (pThis->m_listener)
	{
		evconnlistener_free(pThis->m_listener);
		pThis->m_listener = NULL;
	}

	LOGI("IpcServer: Close all connection\n");
	// 线程退出前得把连接全部关闭掉
	while (pThis->m_connList.size() > 0)
	{
		pThis->m_connList[0]->Disconnect();
	}

	// 执行最后清理操作
	event_base_dispatch(pThis->m_evbase);
	LOGI("IpcServer: Worker thread exit\n");
	THREAD_RETURN();
}

bool IpcServer::Start(uint16_t port, IpcConnectionFactory *connectionFactory)
{
	struct sockaddr_in addr;
	int addr_size;
	memset(&addr, 0, sizeof(addr));

	addr_size = sizeof(struct sockaddr_in);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (!StartListen((struct sockaddr *) &addr, addr_size))
		return false;

	m_connectionFactory = connectionFactory;
	return true;
}

#ifndef _WIN32

bool IpcServer::StartByPipeMode(const char *name, IpcConnectionFactory *connectionFactory)
{
	struct sockaddr_un addr;
	int addr_size;
	memset(&addr, 0, sizeof(addr));

	addr_size = sizeof(struct sockaddr_un);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, name);

	if (addr.sun_path[0] == '@')        // 命名以@开头表示创建非命名socket
		addr.sun_path[0] = 0;

	if (!StartListen((struct sockaddr *) &addr, addr_size))
		return false;

	m_connectionFactory = connectionFactory;
	return true;
}

#endif

bool IpcServer::StartListen(const struct sockaddr *sa, int socklen)
{
	if (m_dispatchThread)
	{
		LOGI("IpcServer: Service already running\n");
		return true;
	}

	if (m_listener)
		evconnlistener_free(m_listener);

	m_listener = evconnlistener_new_bind(m_evbase, accept_cb, this,
	                                     LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
	                                     10, sa, socklen);
	if (!m_listener)
	{
		LOGI("IpcServer: Could not create a listener\n");
		return false;
	}

	THREAD_START(m_dispatchThread, DispatchThread, this);
	if (!m_dispatchThread)
	{
		LOGI("IpcServer: Could not create a worker thread\n");
		return false;
	}
	m_running = true;
	return true;
}

void IpcServer::Stop()
{
	event_base_loopbreak(m_evbase);
	if (m_dispatchThread)
	{
		THREAD_JOIN(m_dispatchThread);
		m_dispatchThread = 0;
	}
	if (m_listener)
	{
		evconnlistener_free(m_listener);
		m_listener = NULL;
	}
	m_connectionFactory = NULL;
	m_running = false;
}

bool IpcServer::IsRunning()
{
	return m_running;
}

void IpcServer::Wait()
{
	struct event *signal_event = evsignal_new(m_evbase, SIGINT, signal_cb, this);
	event_add(signal_event, NULL);
	if (m_dispatchThread)
	{
		THREAD_JOIN(m_dispatchThread);
		m_dispatchThread = 0;
	}
	event_del(signal_event);
	event_free(signal_event);
}
