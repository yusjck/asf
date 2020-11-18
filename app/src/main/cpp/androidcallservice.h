//
// Created by Jack on 2019/7/22.
//

#ifndef ASF_ANDROIDCALLSERVICE_H
#define ASF_ANDROIDCALLSERVICE_H


#include <ipcserver.h>

class AndroidCallService : public IpcConnectionFactory
{
public:
	bool Start();
	void Stop();
	void Wait();
	virtual IpcConnection *NewConnection();
	virtual void DeleteConnection(IpcConnection *conn);

	AndroidCallService();
	virtual ~AndroidCallService();

private:
	IpcServer m_ipcServer;
};


#endif //ASF_ANDROIDCALLSERVICE_H
