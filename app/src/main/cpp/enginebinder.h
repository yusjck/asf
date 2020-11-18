//
// Created by Jack on 2019/7/22.
//

#ifndef ASF_ENGINEBINDER_H
#define ASF_ENGINEBINDER_H


#include "base/msgchannel.h"

class EngineBinder : private MsgWatcher
{
public:
	bool ConnectEngine(const char *serviceName);
	void DisconnectEngine();
	bool RunScript(const char *scriptCfg, const char *userVar);
	bool Abort();
	bool Cleanup();
	bool SetDisplayInfo(int width, int height, int rotation);
	bool EnableCmdServer(bool enable);
	bool SendEvent(const char *event);
	bool SetPluginDir(const char *pluginDir);
	EngineBinder();
	virtual ~EngineBinder();

private:
	MsgChannel m_msgChannel;
	bool m_connecting;

	void OnReportScriptState(BufferReader &msg);
	void OnReportScriptLog(BufferReader &msg);
	void OnReportCleanupFinished(BufferReader &msg);
	void OnReportCmdServerState(BufferReader &msg);

	virtual void OnReceive(CBuffer &msg);
	virtual void OnDisconnect();
};


#endif //ASF_ENGINEBINDER_H
