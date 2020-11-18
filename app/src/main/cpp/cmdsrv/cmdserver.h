//
// Created by Jack on 2019/3/2.
//

#ifndef ASF_CMDSERVER_H
#define ASF_CMDSERVER_H

#include <ipcserver.h>

class CmdServer : public IpcConnectionFactory
{
public:
	bool Start();
	void Stop();
	void Wait();
	virtual IpcConnection *NewConnection();
	virtual void DeleteConnection(IpcConnection *conn);

	CmdServer();
	virtual ~CmdServer();

private:
	IpcServer m_ipcServer;
};

#endif //ASF_CMDSERVER_H
