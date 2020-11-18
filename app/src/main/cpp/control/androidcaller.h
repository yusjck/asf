//
// Created by Jack on 2019/7/22.
//

#ifndef ASF_ANDROIDCALLER_H
#define ASF_ANDROIDCALLER_H


#include <ipcclient.h>
#include <threadevent.h>
#include "androidcallercmd.h"

class CallerContext : public BufferReader, public BufferWriter
{
public:
	const CBuffer &GetRequestBuffer()
	{
		return BufferWriter::GetBuf();
	}

	void SetResponseBuf(CBuffer &buf)
	{
		return BufferReader::SetBuf(buf);
	}

	CallerContext(int cmd)
	{
		WriteInt(cmd);
	}

	virtual ~CallerContext()
	{

	}
};

class AndroidCaller : private IpcClient
{
public:
	int MessageBox(const char *text, uint32_t timeout);
	int ShowMessage(const char *text);
	int Vibrate(uint32_t duration);
	int GetDisplayInfo(int &width, int &height, int &rotation);
	int RequestControl(const char *sourceDeviceName, const char *deviceSignature);

	bool TryConnect();
	int Call(CallerContext &ctx);
	void Disconnect();

	AndroidCaller();
	virtual ~AndroidCaller();

protected:
	virtual void OnConnectFail();
	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual void OnReceive(CBuffer &pack);

private:
	CBuffer m_replyPack;
	ThreadEvent m_connEvent;
};


#endif //ASF_ANDROIDCALLER_H
