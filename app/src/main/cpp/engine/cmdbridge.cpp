#include "cmdbridge.h"

CmdBridge::CmdBridge()
{

}

CmdBridge::~CmdBridge()
{

}

CmdBridge *CmdBridge::GetInstance()
{
	static CmdBridge *cmdbridge = NULL;
	if (cmdbridge == NULL)
		cmdbridge = new CmdBridge;
	return cmdbridge;
}

void CmdBridge::OnConnectFail()
{
	m_connEvent.Set();
}

void CmdBridge::OnConnect()
{
	m_connEvent.Set();
}

void CmdBridge::OnReceive(CBuffer &pack)
{
	*m_replyPack = pack;
	m_connEvent.Set();
}

void CmdBridge::OnDisconnect()
{
	m_connEvent.Set();
}

int CmdBridge::Call(CCmdContext &_ctx)
{
	CmdBridge *pThis = GetInstance();
	CBuffer replyPack;
	pThis->m_replyPack = &replyPack;
	pThis->m_connEvent.Reset();

	if (!pThis->Send(_ctx.GetCmdCtx()))
		return ERR_SLAVE_DISCONNECTED;

	pThis->m_connEvent.Wait();

	if (!pThis->IsConnected())
		return ERR_SLAVE_DISCONNECTED;

	return  _ctx.ParseRetContext(replyPack) ? _ctx.GetRetCode() : ERR_INVOKE_FAILED;
}

bool CmdBridge::TryConnect()
{
	CmdBridge *pThis = GetInstance();
	pThis->m_connEvent.Reset();

	if (!pThis->Connect("127.0.0.1:18656"))
		return false;

	pThis->m_connEvent.Wait();
	return pThis->IsConnected();
}

void CmdBridge::Disconnect()
{
	IpcClient *cmdbridge = GetInstance();
	cmdbridge->Disconnect();
}
