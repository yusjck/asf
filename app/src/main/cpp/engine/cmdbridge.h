#ifndef _CMDBRIDGE_H
#define _CMDBRIDGE_H

#include "ipcclient.h"
#include "cmdcode.h"
#include "errcode.h"
#include "threadevent.h"
#include "cmdcontext.h"

class CmdBridge : private IpcClient
{
public:
	static bool TryConnect();
	static int Call(CCmdContext &_ctx);
	static void Disconnect();
	CmdBridge();
	virtual ~CmdBridge();

protected:
	virtual void OnConnectFail();
	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual void OnReceive(CBuffer &pack);

private:
	CBuffer *m_replyPack;
	ThreadEvent m_connEvent;
	static CmdBridge *GetInstance();
};

#endif