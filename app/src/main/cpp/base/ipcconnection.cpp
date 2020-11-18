//
// Created by Jack on 2019/6/17.
//

#include "common.h"
#include "ipcconnection.h"

void IpcConnectionManager::OpenConnection(IpcConnection *conn, struct bufferevent *bev)
{
	conn->m_connManager = this;
	conn->OnConnectInternal(bev);
}

IpcConnection::IpcConnection()
{
	m_bevSocket = NULL;
	m_connManager = NULL;
}

IpcConnection::~IpcConnection()
{
	Disconnect();
}

void IpcConnection::OnConnectInternal(struct bufferevent *bev)
{
	m_bevSocket = bev;

	bufferevent_setcb(bev, read_cb, NULL, event_cb, this);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);

	OnConnect();
}

void IpcConnection::OnRead(struct bufferevent *bev)
{
	struct evbuffer *buf = bufferevent_get_input(bev);

	do
	{
		uint32_t packSize;
		if (evbuffer_copyout(buf, &packSize, sizeof(packSize)) < sizeof(packSize))
			return;

		if (evbuffer_get_length(buf) < packSize + sizeof(packSize))
			return;

		evbuffer_remove(buf, &packSize, sizeof(packSize));
		CBuffer pack;

		if (packSize > 0)
		{
			void *p = pack.GetBuffer(packSize);
			if (p == NULL)
				throw "no memory";

			evbuffer_remove(buf, p, packSize);
		}

		OnReceive(pack);        // 将收到的完整数据包交给下层处理
	} while (1);
}

void IpcConnection::OnEvent(struct bufferevent *bev, short event)
{
	if (event & BEV_EVENT_EOF)
	{
		LOGI("IpcConnection: Connection closed\n");
	}
	else if (event & BEV_EVENT_ERROR)
	{
		LOGI("IpcConnection: Got an error on the connection: %s\n", strerror(errno));
	}
	else if (event & BEV_EVENT_TIMEOUT)
	{
		LOGI("IpcConnection: Time out\n");
	}

	bufferevent_free(m_bevSocket);
	m_bevSocket = NULL;
	OnDisconnect();               // 通知使用者连接已经断开
	m_connManager->CloseConnection(this);
}

bool IpcConnection::Send(const CBuffer &pack)
{
	if (m_bevSocket == NULL)
	{
		LOGI("IpcConnection: Connection is closed\n");
		return false;
	}

	uint32_t packSize = pack.GetSize();
	int result = bufferevent_write(m_bevSocket, &packSize, sizeof(packSize));
	if (result != 0)
	{
		LOGI("IpcConnection: Write data failure\n");
		return false;
	}

	result = bufferevent_write(m_bevSocket, pack.GetBuffer(), pack.GetSize());
	if (result != 0)
	{
		LOGI("IpcConnection: Write data failure\n");
		return false;
	}

	bufferevent_flush(m_bevSocket, EV_WRITE, BEV_FLUSH);
	return true;
}

void IpcConnection::Disconnect()
{
	if (m_bevSocket)
	{
		bufferevent_free(m_bevSocket);
		m_bevSocket = NULL;
		OnDisconnect();
		m_connManager->CloseConnection(this);
	}
}

bool IpcConnection::IsConnected()
{
	return m_bevSocket != NULL;
}

void IpcConnection::read_cb(struct bufferevent *bev, void *arg)
{
	((IpcConnection *) arg)->OnRead(bev);
}

void IpcConnection::event_cb(struct bufferevent *bev, short event, void *arg)
{
	((IpcConnection *) arg)->OnEvent(bev, event);
}
