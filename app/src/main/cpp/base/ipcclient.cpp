#ifndef WIN32

#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#endif

#include "common.h"
#include "ipcclient.h"

IpcClient::IpcClient()
{
#ifndef _WIN32
	evthread_use_pthreads();
#else
	evthread_use_windows_threads();
#endif

	m_evbase = event_base_new();
	if (!m_evbase)
		throw "Could not initialize libevent\n";

	m_bevSocket = NULL;
	m_connectTimeout = { 0 };
	m_dispatchThread = 0;
}

IpcClient::~IpcClient()
{
	Disconnect();

	if (m_dispatchThread)
	{
		THREAD_JOIN(m_dispatchThread);
		m_dispatchThread = 0;
	}

	if (m_evbase)
	{
		event_base_free(m_evbase);
		m_evbase = NULL;
	}
}

void IpcClient::OnEvent(struct bufferevent *bev, short event)
{
	// 连接完成或失败，删除超时检测定时器
	event_del(&m_connectTimeout);

	if (event & BEV_EVENT_CONNECTED)
	{
		LOGI("IpcClient: The client has connected to server\n");
		IpcConnectionManager::OpenConnection(this, bev);
		return;
	}

	LOGI("IpcClient: Unable connect to server\n");
	OnConnectFail();
	bufferevent_free(bev);
}

void IpcClient::CloseConnection(IpcConnection *conn)
{

}

void IpcClient::connect_event_cb(struct bufferevent *bev, short event, void *arg)
{
	IpcClient *pThis = (IpcClient *) arg;
	pThis->OnEvent(bev, event);
}

void IpcClient::connect_timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	LOGI("IpcClient: Connect timeout\n");
	// 连接超时了，关掉Socket退出
	IpcClient *pThis = (IpcClient *) arg;
	pThis->OnConnectFail();
	bufferevent_free(pThis->m_bevSocket);
}

THREAD_FN IpcClient::DispatchThread(void *arg)
{
	IpcClient *pThis = (IpcClient *) arg;
	event_base_dispatch(pThis->m_evbase);
	THREAD_RETURN();
}

bool IpcClient::Connect(const char *name)
{
	struct sockaddr_in addr;
	int addr_size;
	memset(&addr, 0, sizeof(addr));

	addr_size = sizeof(struct sockaddr_in);
	if (evutil_parse_sockaddr_port(name, (struct sockaddr *) &addr, &addr_size) != 0)
	{
		LOGI("IpcClient: Could not resolve server address\n");
		return false;
	}

	return StartConnect((struct sockaddr *) &addr, addr_size);
}

#ifndef _WIN32

bool IpcClient::ConnectByPipeMode(const char *name)
{
	struct sockaddr_un addr;
	int addr_size;
	memset(&addr, 0, sizeof(addr));

	addr_size = sizeof(struct sockaddr_un);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, name);

	if (addr.sun_path[0] == '@')        // 命名以@开头表示创建非命名socket
		addr.sun_path[0] = 0;

	return StartConnect((struct sockaddr *) &addr, addr_size);
}

#endif

bool IpcClient::StartConnect(const struct sockaddr *sa, int socklen)
{
	if (IsConnected())
	{
		LOGI("IpcClient: Connection already exists\n");
		return true;
	}

	if (m_dispatchThread)
	{
		THREAD_JOIN(m_dispatchThread);
		m_dispatchThread = 0;
	}

	m_bevSocket = bufferevent_socket_new(m_evbase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	bufferevent_socket_connect(m_bevSocket, sa, socklen);
	bufferevent_setcb(m_bevSocket, NULL, NULL, connect_event_cb, this);

	event_assign(&m_connectTimeout, m_evbase, -1, 0, connect_timeout_cb, this);

	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = 2;
	event_add(&m_connectTimeout, &tv);

	THREAD_START(m_dispatchThread, DispatchThread, this);
	if (!m_dispatchThread)
	{
		bufferevent_free(m_bevSocket);
		event_del(&m_connectTimeout);
		LOGI("IpcClient: Could not create a worker thread\n");
		return false;
	}

	return true;
}
