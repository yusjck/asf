#include "cmdserver.h"
#include "cmddispatcher.h"

CmdServer::CmdServer()
{

}

CmdServer::~CmdServer()
{

}

IpcConnection *CmdServer::NewConnection()
{
	return new CmdDispatcher;
}

void CmdServer::DeleteConnection(IpcConnection *conn)
{
	delete conn;
}

bool CmdServer::Start()
{
	return m_ipcServer.Start(18656, this);
}

void CmdServer::Stop()
{
	m_ipcServer.Stop();
}

void CmdServer::Wait()
{
	m_ipcServer.Wait();
}
