//
// Created by Jack on 2019/6/15.
//

#ifndef ASF_MSGCHANNEL_H
#define ASF_MSGCHANNEL_H

#include <thread>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/thread.h>
#include <event2/event_struct.h>
#include "buffer.h"

class MsgWatcher
{
public:
	MsgWatcher() {};
	virtual ~MsgWatcher() {};

protected:
	virtual void OnConnect() {};
	virtual void OnReceive(CBuffer &msg) = 0;
	virtual void OnDisconnect() {};

	friend class MsgChannel;
};

class MsgChannel
{
public:
	MsgChannel();
	~MsgChannel();

	bool Listen(const char *name, MsgWatcher *watcher);
	bool Connect(const char *name, MsgWatcher *watcher);
	void Close();
	bool IsConnected() { return m_connected; }
	bool Send(const CBuffer &msg);

private:
	struct event_base *m_evbase;
	struct evconnlistener *m_listener;
	struct bufferevent *m_bev_client;
	struct bufferevent *m_bev_server;
	bool m_connected;
	enum {
		MODE_NONE,
		MODE_SERVER,
		MODE_CLIENT,
	} m_work_mode;
	std::thread *m_dispatch_thread;
	MsgWatcher *m_watcher;

	static void accept_cb(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen, void *arg);
	static void read_cb(struct bufferevent *bev, void *arg);
	static void event_cb(struct bufferevent *bev, short event, void *arg);
	static void connect_timeout_cb(evutil_socket_t fd, short event, void *arg);

	void DispatchThread();
	void OnAccept(evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sock, int socklen);
	void OnRead(struct bufferevent *bev);
	void OnEvent(struct bufferevent *bev, short event);
};

#endif //ASF_MSGCHANNEL_H
