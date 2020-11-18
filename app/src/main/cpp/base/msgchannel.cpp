#include <sys/un.h>
#include "common.h"
#include "msgchannel.h"

MsgChannel::MsgChannel()
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
	m_bev_client = NULL;
	m_bev_server = NULL;
	m_connected = false;
	m_work_mode = MODE_NONE;
	m_dispatch_thread = NULL;
	m_watcher = NULL;
}

MsgChannel::~MsgChannel()
{
	Close();

	if (m_evbase)
	{
		event_base_free(m_evbase);
		m_evbase = NULL;
	}
}

void MsgChannel::OnAccept(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen)
{
	struct bufferevent *bev = bufferevent_socket_new(m_evbase, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	if (!bev)
	{
		LOGI("MsgChannel: Error constructing bufferevent");
		event_base_loopbreak(m_evbase);
		return;
	}

	// 只允许一个连接，当连接建立完成后就把监听器关掉，断开后再重新开启
	if (m_connected)
		evconnlistener_disable(m_listener);

	LOGI("MsgChannel: Channel connected\n");
	bufferevent_setcb(bev, read_cb, NULL, event_cb, this);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);

	m_bev_client = bev;
	m_connected = true;
	m_watcher->OnConnect();
}

void MsgChannel::OnRead(struct bufferevent *bev)
{
	struct evbuffer *buf = bufferevent_get_input(bev);

	do
	{
		uint32_t msg_len;
		if (evbuffer_copyout(buf, &msg_len, sizeof(msg_len)) < sizeof(msg_len))
			return;

		if (evbuffer_get_length(buf) < msg_len + sizeof(msg_len))
			return;

		evbuffer_remove(buf, &msg_len, sizeof(msg_len));
		CBuffer msg;

		if (msg_len > 0)
		{
			void *p = msg.GetBuffer(msg_len);
			if (p == NULL)
				throw "No memory";

			evbuffer_remove(buf, p, msg_len);
		}

		m_watcher->OnReceive(msg);        // 将收到的完整数据包交给下层处理
	} while (1);
}

void MsgChannel::OnEvent(struct bufferevent *bev, short event)
{
	if (event & BEV_EVENT_CONNECTED)
	{
		LOGI("MsgChannel: The client has connected to server\n");
		m_connected = true;
		m_watcher->OnConnect();
		event_base_loopbreak(m_evbase);     // 客户端连接为阻塞模式，连上后要退出事件循环
		return;
	}

	if (event & BEV_EVENT_EOF)
	{
		LOGI("MsgChannel: Connection closed\n");
	}
	else if (event & BEV_EVENT_ERROR)
	{
		LOGI("MsgChannel: Got an error on the connection: %s\n", strerror(errno));
	}
	else if (event & BEV_EVENT_TIMEOUT)
	{
		LOGI("MsgChannel: Time out\n");
	}

	// 处理连接被动断开
	m_connected = false;
	m_watcher->OnDisconnect();             // 通知MsgWatcher连接已经断开

	// 处理连接断开后的后续操作
	if (m_work_mode == MODE_SERVER)
	{
		bufferevent_free(m_bev_client);
		m_bev_client = NULL;
		evconnlistener_enable(m_listener);  // 打开监听器，等待客户端再次连接
	}
	else if (m_work_mode == MODE_CLIENT)
	{
		bufferevent_free(m_bev_server);     // 客户端断开后事件循环会自动退出
		m_bev_server = NULL;
	}
}

bool MsgChannel::Send(const CBuffer &msg)
{
	if (m_work_mode == MODE_SERVER)
	{
		if (m_bev_client == NULL)
			return false;

		uint32_t msg_len = msg.GetSize();
		bufferevent_write(m_bev_client, &msg_len, sizeof(msg_len));
		bufferevent_write(m_bev_client, msg.GetBuffer(), msg.GetSize());
		bufferevent_flush(m_bev_client, EV_WRITE, BEV_FLUSH);
		return true;
	}

	if (m_work_mode == MODE_CLIENT)
	{
		if (m_bev_server == NULL)
			return false;

		uint32_t msg_len = msg.GetSize();
		bufferevent_write(m_bev_server, &msg_len, sizeof(msg_len));
		bufferevent_write(m_bev_server, msg.GetBuffer(), msg.GetSize());
		bufferevent_flush(m_bev_server, EV_WRITE, BEV_FLUSH);
		return true;
	}

	return false;
}

void MsgChannel::DispatchThread()
{
	event_base_dispatch(m_evbase);
	LOGI("MsgChannel: Worker thread exit\n");
}

bool MsgChannel::Listen(const char *name, MsgWatcher *watcher)
{
	if (m_work_mode != MODE_NONE)
		return false;

	struct sockaddr_un addr;
	int addr_size;
	memset(&addr, 0, sizeof(addr));

	addr_size = sizeof(struct sockaddr_un);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, name);

	if (addr.sun_path[0] == '@')        // 命名以@开头表示创建非命名socket
		addr.sun_path[0] = 0;

	if (m_listener)
		evconnlistener_free(m_listener);

	m_listener = evconnlistener_new_bind(m_evbase, accept_cb, this,
	                                     LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
	                                     1, (struct sockaddr *) &addr, addr_size);
	if (!m_listener)
	{
		LOGI("MsgChannel: Could not create a listener\n");
		return false;
	}

	m_work_mode = MODE_SERVER;
	m_watcher = watcher;

	m_dispatch_thread = new std::thread(&MsgChannel::DispatchThread, this);
	if (!m_dispatch_thread)
	{
		Close();
		LOGI("MsgChannel: Could not create a worker thread\n");
		return false;
	}

	return true;
}

bool MsgChannel::Connect(const char *name, MsgWatcher *watcher)
{
	if (IsConnected())
		return true;

	// 当前模式不是NONE说明上次调用后没有关闭，再次使用要先清理资源
	if (m_work_mode != MODE_NONE)
		Close();

	struct sockaddr_un addr;
	int addr_size;
	memset(&addr, 0, sizeof(addr));

	addr_size = sizeof(struct sockaddr_un);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, name);

	if (addr.sun_path[0] == '@')
		addr.sun_path[0] = 0;

	struct event timeout;
	event_assign(&timeout, m_evbase, -1, 0, connect_timeout_cb, this);

	// 设置连接超时时长为2秒
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = 2;
	event_add(&timeout, &tv);

	m_bev_server = bufferevent_socket_new(m_evbase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	bufferevent_socket_connect(m_bev_server, (struct sockaddr *) &addr, addr_size);

	bufferevent_setcb(m_bev_server, read_cb, NULL, event_cb, this);
	bufferevent_enable(m_bev_server, EV_READ | EV_PERSIST);

	m_work_mode = MODE_CLIENT;
	m_watcher = watcher;

	// 执行连接服务端接口，这里将阻塞直到连接完成或超时
	event_base_loop(m_evbase, EVLOOP_ONCE);

	event_del(&timeout);
	if (!m_connected)
	{
		Close();
		LOGI("MsgChannel: Could not connect to server\n");
		return false;
	}

	m_dispatch_thread = new std::thread(&MsgChannel::DispatchThread, this);
	if (!m_dispatch_thread)
	{
		Close();
		LOGI("MsgChannel: Could not create a worker thread\n");
		return false;
	}

	return true;
}

void MsgChannel::Close()
{
	if (m_dispatch_thread)
	{
		// 不允许从内部(MsgWatcher回调函数)调用Close关闭连接
		if (std::this_thread::get_id() == m_dispatch_thread->get_id())
			throw "Invalid operation";

		// 退出事件循环线程
		event_base_loopbreak(m_evbase);
		m_dispatch_thread->join();
		delete m_dispatch_thread;
		m_dispatch_thread = NULL;
	}

	if (m_listener)
	{
		evconnlistener_free(m_listener);
		m_listener = NULL;
	}

	if (m_bev_client)
	{
		bufferevent_free(m_bev_client);
		m_bev_client = NULL;
	}

	if (m_bev_server)
	{
		bufferevent_free(m_bev_server);
		m_bev_server = NULL;
	}

	// 执行最终的清理工作
	event_base_dispatch(m_evbase);

	m_connected = false;
	m_work_mode = MODE_NONE;
	m_watcher = NULL;
}

void MsgChannel::accept_cb(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen, void *arg)
{
	((MsgChannel *) arg)->OnAccept(listener, fd, sock, socklen);
}

void MsgChannel::read_cb(struct bufferevent *bev, void *arg)
{
	((MsgChannel *) arg)->OnRead(bev);
}

void MsgChannel::event_cb(struct bufferevent *bev, short event, void *arg)
{
	((MsgChannel *) arg)->OnEvent(bev, event);
}

void MsgChannel::connect_timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	MsgChannel *pThis = (MsgChannel *) arg;
	LOGI("MsgChannel: Connect timeout\n");
	event_base_loopbreak(pThis->m_evbase);
}
