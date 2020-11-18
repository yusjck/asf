//
// Created by Jack on 2019/7/22.
//

#include "androidcallservice.h"

void *createAndroidCaller();
void destroyAndroidCaller(void *androidCaller);
void forwardAndroidCall(void *androidCaller, CBuffer &request, CBuffer &response);

class AndroidCallConnection : public IpcConnection
{
public:
	AndroidCallConnection()
	{
		m_androidCaller = nullptr;
	}

	virtual ~AndroidCallConnection()
	{
		if (m_androidCaller)
			destroyAndroidCaller(m_androidCaller);
	}

private:
	void *m_androidCaller;

	virtual void OnConnect()
	{
		m_androidCaller = createAndroidCaller();
	}

	virtual void OnDisconnect()
	{
		destroyAndroidCaller(m_androidCaller);
		m_androidCaller = nullptr;
	}

	virtual void OnReceive(CBuffer &request)
	{
		CBuffer response;
		forwardAndroidCall(m_androidCaller, request, response);
		Send(response);
	}
};

AndroidCallService::AndroidCallService()
{

}

AndroidCallService::~AndroidCallService()
{

}

IpcConnection *AndroidCallService::NewConnection()
{
	return new AndroidCallConnection;
}

void AndroidCallService::DeleteConnection(IpcConnection *conn)
{
	delete conn;
}

bool AndroidCallService::Start()
{
	return m_ipcServer.StartByPipeMode("@asf_android_call", this);
}

void AndroidCallService::Stop()
{
	m_ipcServer.Stop();
}
