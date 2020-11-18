//
// Created by Jack on 2019/7/22.
//

#include <errcode.h>
#include "androidcaller.h"

AndroidCaller::AndroidCaller()
= default;

AndroidCaller::~AndroidCaller()
= default;

int AndroidCaller::MessageBox(const char *text, uint32_t timeout)
{
	CallerContext ctx(ACCMD_MESSAGEBOX);
	ctx.WriteStr(text);
	ctx.WriteInt(timeout);
	return Call(ctx);
}

int AndroidCaller::ShowMessage(const char *text)
{
	CallerContext ctx(ACCMD_SHOWMESSAGE);
	ctx.WriteStr(text);
	return Call(ctx);
}

int AndroidCaller::Vibrate(uint32_t duration)
{
	CallerContext ctx(ACCMD_VIBRATE);
	ctx.WriteInt(duration);
	return Call(ctx);
}

int AndroidCaller::GetDisplayInfo(int &width, int &height, int &rotation)
{
	CallerContext ctx(ACCMD_GETDISPLAYINFO);

	int ret = Call(ctx);
	if (!IS_SUCCESS(ret))
		return ret;

	width = ctx.ReadInt();
	height = ctx.ReadInt();
	rotation = ctx.ReadInt();
	return ERR_NONE;
}

int AndroidCaller::RequestControl(const char *sourceDeviceName, const char *deviceSignature)
{
	CallerContext ctx(ACCMD_REQUESTCONTROL);
	ctx.WriteStr(sourceDeviceName);
	ctx.WriteStr(deviceSignature);
	return Call(ctx);
}

bool AndroidCaller::TryConnect()
{
	m_connEvent.Reset();

	if (!ConnectByPipeMode("@asf_android_call"))
		return false;

	m_connEvent.Wait();
	return IsConnected();
}

void AndroidCaller::Disconnect()
{
	IpcClient::Disconnect();
}

int AndroidCaller::Call(CallerContext &ctx)
{
	m_connEvent.Reset();
	if (!Send(ctx.GetRequestBuffer()))
		return ERR_SLAVE_DISCONNECTED;

	m_connEvent.Wait();

	if (!IsConnected())
		return ERR_SLAVE_DISCONNECTED;

	ctx.SetResponseBuf(m_replyPack);
	m_replyPack.FreeBuffer();

	return ctx.ReadInt();
}

void AndroidCaller::OnConnectFail()
{
	m_connEvent.Set();
}

void AndroidCaller::OnConnect()
{
	m_connEvent.Set();
}

void AndroidCaller::OnReceive(CBuffer &pack)
{
	m_replyPack = pack;
	m_connEvent.Set();
}

void AndroidCaller::OnDisconnect()
{
	m_connEvent.Set();
}
