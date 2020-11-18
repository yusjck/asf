//
// Created by Jack on 2019/6/17.
//

#ifndef ASF_IPCCONNECTION_H
#define ASF_IPCCONNECTION_H


#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/thread.h>
#include "buffer.h"

class IpcConnection;

class IpcConnectionManager
{
public:
	virtual void CloseConnection(IpcConnection *conn) = 0;
	virtual ~IpcConnectionManager() {};

protected:
	virtual void OpenConnection(IpcConnection *conn, struct bufferevent *bev);
};

class IpcConnectionFactory
{
public:
	virtual IpcConnection *NewConnection() = 0;
	virtual void DeleteConnection(IpcConnection *conn) = 0;
	virtual ~IpcConnectionFactory() {};
};

class IpcConnection
{
public:
	bool Send(const CBuffer &pack);
	void Disconnect();
	bool IsConnected();

	IpcConnection();
	virtual ~IpcConnection();

protected:
	virtual void OnConnect() {};
	virtual void OnDisconnect() {};
	virtual void OnReceive(CBuffer &pack) = 0;

private:
	struct bufferevent *m_bevSocket;
	IpcConnectionManager *m_connManager;
	friend class IpcConnectionManager;

	void OnConnectInternal(struct bufferevent *bev);
	void OnRead(struct bufferevent *bev);
	void OnEvent(struct bufferevent *bev, short event);

	static void read_cb(struct bufferevent *bev, void *arg);
	static void event_cb(struct bufferevent *bev, short event, void *arg);
};


#endif //ASF_IPCCONNECTION_H
