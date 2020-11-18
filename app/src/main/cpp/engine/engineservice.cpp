//
// Created by Jack on 2019/7/22.
//

#include <unistd.h>
#include "engineservice.h"
#include "enginecmd.h"
#include "luamain.h"

EngineService::EngineService()
= default;

EngineService::~EngineService()
= default;

bool EngineService::Start()
{
	if (getuid() == 0)
		return m_msgChannel.Listen("@asf_engine_by_root", this);
	else
		return m_msgChannel.Listen("@asf_engine", this);
}

void EngineService::Stop()
{
	m_msgChannel.Close();
}

void EngineService::OnDisconnect()
{
	if (getuid() == 0)
		exit(0);

	m_cmdServer.Stop();
}

void EngineService::ReportScriptState(int state, int flags)
{
	BufferWriter msg;
	msg.WriteInt(ENGREPORT_SCRIPTSTATE);
	msg.WriteInt(state);
	msg.WriteInt(flags);
	m_msgChannel.Send(msg);
}

void EngineService::ReportScriptLog(uint32_t time, uint32_t color, const char *text)
{
	BufferWriter msg;
	msg.WriteInt(ENGREPORT_SCRIPTLOG);
	msg.WriteUint(time);
	msg.WriteUint(color);
	msg.WriteStr(text);
	m_msgChannel.Send(msg);
}

void EngineService::OnRunScript(BufferReader &msg)
{
	const char *scriptCfg = msg.ReadStr();
	const char *userVar = msg.ReadStr();
	LuaRunScript(scriptCfg, userVar);
}

void EngineService::OnAbort(BufferReader &msg)
{
	LuaAbort();
}

void EngineService::OnCleanup(BufferReader &msg)
{
	LuaCleanup([](void *ud) {
		auto *msgChannel = (MsgChannel *)ud;
		BufferWriter reply;
		reply.WriteInt(ENGREPORT_CLEANUPFINISHED);
		msgChannel->Send(reply);
	}, &m_msgChannel);
}

void EngineService::OnSetDisplayInfo(BufferReader &msg)
{
	int width = msg.ReadInt();
	int height = msg.ReadInt();
	int rotation = msg.ReadInt();
	DeviceInfo::UpdateDisplayInfo(width, height, rotation);
}

void EngineService::OnEnableCmdServer(BufferReader &msg)
{
	static bool cmdServiceEnabled = false;
	bool enable = msg.ReadBool();

	// 判断要指定的状态是否等同当前状态，是就不处理
	if (enable == cmdServiceEnabled)
		return;

	if (enable)
	{
		bool success = m_cmdServer.Start();
		BufferWriter reply;
		reply.WriteInt(ENGREPORT_CMDSERVERSTATE);
		reply.WriteInt(success ? CMDSERVER_STARTED : CMDSERVER_START_FAILED);
		m_msgChannel.Send(reply);

		cmdServiceEnabled = success;
	}
	else
	{
		m_cmdServer.Stop();
		BufferWriter reply;
		reply.WriteInt(ENGREPORT_CMDSERVERSTATE);
		reply.WriteInt(CMDSERVER_STOPPED);
		m_msgChannel.Send(reply);

		cmdServiceEnabled = false;
	}
}

void EngineService::OnEventNotify(BufferReader &msg)
{
	const char *event = msg.ReadStr();
	LuaPushEvent(event);
}

void EngineService::OnSetPluginDir(BufferReader &msg)
{
	const char *pluginDir = msg.ReadStr();
	PluginManager::SetDefaultPluginDir(pluginDir);
}

#define COMMAND_MAP_BEGIN switch (cmd) {
#define COMMAND_MAP_END default:\
		break;\
	}
#define ON_COMMAND(cmd, handler)\
		case cmd:\
			handler(reader);\
			break;

void EngineService::OnReceive(CBuffer &msg)
{
	BufferReader reader(msg);
	int cmd = reader.ReadInt();

	COMMAND_MAP_BEGIN
		ON_COMMAND(ENGCTRL_RUNSCRIPT, OnRunScript)
		ON_COMMAND(ENGCTRL_ABORT, OnAbort)
		ON_COMMAND(ENGCTRL_CLEANUP, OnCleanup)
		ON_COMMAND(ENGCTRL_SETDISPLAYINFO, OnSetDisplayInfo)
		ON_COMMAND(ENGCTRL_ENABLECMDSERVER, OnEnableCmdServer)
		ON_COMMAND(ENGCTRL_EVENTNOTIFY, OnEventNotify)
		ON_COMMAND(ENGCTRL_SETPLUGINDIR, OnSetPluginDir)
	COMMAND_MAP_END
}
