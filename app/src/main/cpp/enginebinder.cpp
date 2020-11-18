//
// Created by Jack on 2019/7/22.
//

#include "enginebinder.h"
#include "engine/enginecmd.h"

void updateScriptState(int state, int flags);
void outputScriptLog(uint32_t time, uint32_t color, const char *text);
void processServiceExit();
void updateCmdServerState(int state);
void engineDisconnectNotify();

EngineBinder::EngineBinder()
= default;

EngineBinder::~EngineBinder()
= default;

bool EngineBinder::ConnectEngine(const char *serviceName)
{
	m_connecting = true;
	bool result = m_msgChannel.Connect(serviceName, this);
	m_connecting = false;
	return result;
}

void EngineBinder::DisconnectEngine()
{
	m_msgChannel.Close();
}

bool EngineBinder::RunScript(const char *scriptCfg, const char *userVar)
{
	BufferWriter msg;
	msg.WriteInt(ENGCTRL_RUNSCRIPT);
	msg.WriteStr(scriptCfg);
	msg.WriteStr(userVar);
	return m_msgChannel.Send(msg);
}

bool EngineBinder::Abort()
{
	BufferWriter msg;
	msg.WriteInt(ENGCTRL_ABORT);
	return m_msgChannel.Send(msg);
}

bool EngineBinder::Cleanup()
{
	BufferWriter msg;
	msg.WriteInt(ENGCTRL_CLEANUP);
	return m_msgChannel.Send(msg);
}

bool EngineBinder::SetDisplayInfo(int width, int height, int rotation)
{
	BufferWriter msg;
	msg.WriteInt(ENGCTRL_SETDISPLAYINFO);
	msg.WriteInt(width);
	msg.WriteInt(height);
	msg.WriteInt(rotation);
	return m_msgChannel.Send(msg);
}

bool EngineBinder::EnableCmdServer(bool enable)
{
	BufferWriter msg;
	msg.WriteInt(ENGCTRL_ENABLECMDSERVER);
	msg.WriteBool(enable);
	return m_msgChannel.Send(msg);
}

bool EngineBinder::SendEvent(const char *event)
{
	BufferWriter msg;
	msg.WriteInt(ENGCTRL_EVENTNOTIFY);
	msg.WriteStr(event);
	return m_msgChannel.Send(msg);
}

bool EngineBinder::SetPluginDir(const char *pluginDir)
{
	BufferWriter msg;
	msg.WriteInt(ENGCTRL_SETPLUGINDIR);
	msg.WriteStr(pluginDir);
	return m_msgChannel.Send(msg);
}

void EngineBinder::OnReportScriptState(BufferReader &msg)
{
	int state = msg.ReadInt();
	int flags = msg.ReadInt();
	updateScriptState(state, flags);
}

void EngineBinder::OnReportScriptLog(BufferReader &msg)
{
	uint32_t time = msg.ReadUint();
	uint32_t color = msg.ReadUint();
	const char *text = msg.ReadStr();
	outputScriptLog(time, color, text);
}

void EngineBinder::OnReportCleanupFinished(BufferReader &msg)
{
	processServiceExit();
}

void EngineBinder::OnReportCmdServerState(BufferReader &msg)
{
	int state = msg.ReadInt();
	updateCmdServerState(state);
}

void EngineBinder::OnReceive(CBuffer &msg)
{
	BufferReader reader(msg);
	int cmd = reader.ReadInt();

	switch (cmd)
	{
		case ENGREPORT_SCRIPTSTATE:
			OnReportScriptState(reader);
			break;
		case ENGREPORT_SCRIPTLOG:
			OnReportScriptLog(reader);
			break;
		case ENGREPORT_CLEANUPFINISHED:
			OnReportCleanupFinished(reader);
			break;
		case ENGREPORT_CMDSERVERSTATE:
			OnReportCmdServerState(reader);
			break;
		default:
			break;
	}
}

void EngineBinder::OnDisconnect()
{
	if (!m_connecting)
		engineDisconnectNotify();
}
