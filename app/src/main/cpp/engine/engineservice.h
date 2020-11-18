//
// Created by Jack on 2019/7/22.
//

#ifndef ASF_ENGINESERVICE_H
#define ASF_ENGINESERVICE_H


#include "../base/msgchannel.h"
#include <cmdserver.h>

class EngineService : public MsgWatcher
{
public:
	bool Start();
	void Stop();
	void ReportScriptState(int state, int flags);
	void ReportScriptLog(uint32_t time, uint32_t color, const char *text);

	EngineService();
	virtual ~EngineService();

private:
	MsgChannel m_msgChannel;
	CmdServer m_cmdServer;

	void OnRunScript(BufferReader &msg);
	void OnAbort(BufferReader &msg);
	void OnCleanup(BufferReader &msg);
	void OnSetDisplayInfo(BufferReader &msg);
	void OnEnableCmdServer(BufferReader &msg);
	void OnEventNotify(BufferReader &msg);
	void OnSetPluginDir(BufferReader &msg);

	virtual void OnReceive(CBuffer &msg);
	virtual void OnDisconnect();
};


#endif //ASF_ENGINESERVICE_H
